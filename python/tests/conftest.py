"""
    This is the test-configuration for xNVMe

    xnvme_be_opts(): helper-function used by xnvme_setup()

    xnvme_setup(): generates a list of sensible backend configurations and matches them
    up with devices from the CIJOE configuration. Emitting (device, be_opts, cli_args)
    and pytest.skip when a valid device is not found in the configuration.

    XnvmeDriver: provides a "functor" for controlling NVMe driver attachment.

    xnvme_driver: a fixture for parametrize, invoking XnvmeDriver as needed by the
    specific testcase, e.g. specific to the parametrization. This relies on the pytest
    feature "indirect parametrization".
"""
import errno
import logging as log
import os.path
import re
from functools import wraps
from pathlib import Path
from time import sleep

import pytest
from xnvme_be_combinations import get_combinations


def xnvme_be_opts(options=None, only_labels=[]):
    """Produce a list of "sensible" backend configurations"""

    osname = pytest.cijoe_instance.config.options.get("os", {}).get("name", "linux")
    if osname == "debian":
        osname = "linux"

    if options is None:
        options = ["be", "mem", "sync", "async", "admin", "label"]
    if "label" not in options:
        options.append("label")

    combinations = get_combinations()

    all_configs = []
    for opts in combinations[osname]:
        for be in opts["be"]:
            for be_mem in opts["mem"]:
                for be_admin in opts["admin"]:
                    for be_sync in opts["sync"]:
                        for be_async in opts["async"]:
                            for label in opts["label"]:
                                if only_labels and label not in only_labels:
                                    continue

                            all_configs.append(
                                {
                                    "be": be,
                                    "mem": be_mem,
                                    "admin": be_admin,
                                    "sync": be_sync,
                                    "async": be_async,
                                    "label": opts["label"],
                                }
                            )

    filtered = []
    for cfg in all_configs:
        item = [(key, val) for (key, val) in cfg.items() if key in options]
        if item not in filtered:
            filtered.append(item)
    filtered.sort()

    return [dict(item) for item in filtered]


def cijoe_config_get_device(labels):
    """Returns the 'device-dict' from 'devices' in 'cijoe_cfg' with the given 'label'"""

    for device in pytest.cijoe_instance.config.options.get("devices", []):
        if not (set(labels) - set(device["labels"])):
            return device

    return None


def xnvme_setup_device(labels):
    """Get a device, without a backend-configuration to match"""

    device = cijoe_config_get_device(labels)
    if device:
        return [device]

    return [
        pytest.param(
            device,
            marks=pytest.mark.skip(
                f"xnvme_setup_device(): Configuration has no device labelled: {labels}"
            ),
        )
    ]


def xnvme_cli_args(device, be_opts):
    """Construct cli-arguments for the given device and backend options"""

    args = []

    if device:
        args += [f"{device['uri']}"]
        args += [f"--dev-nsid {device['nsid']}"]

    if be_opts:
        args += [f"--{arg} {val}" for arg, val in be_opts.items() if arg != "label"]

    return " ".join(args)


def xnvme_setup(labels=[], opts=[]):
    """Produces a config, yields (device, be_opts, cli_args)"""

    parametrization = []

    combinations = xnvme_be_opts(
        opts, ["file"] if "file" in labels else ["bdev", "cdev", "pcie", "fabrics"]
    )

    for be_opts in combinations:
        search = labels + be_opts["label"]
        device = cijoe_config_get_device(search)

        dstr = device["uri"] if device else "None"
        bstr = ",".join([f"{k}={v}" for k, v in be_opts.items()])

        paramid = f"uri={dstr},{bstr}"

        cli_args = xnvme_cli_args(device, be_opts)

        if device is None:
            parametrization.append(
                pytest.param(
                    device,
                    be_opts,
                    cli_args,
                    marks=pytest.mark.skip(
                        f"Configuration has no device labelled: {search}"
                    ),
                    id=paramid,
                )
            )
        else:
            parametrization.append(
                pytest.param(
                    device,
                    be_opts,
                    cli_args,
                    id=paramid,
                )
            )

    return parametrization


def fabrics_setup(cijoe):
    """Setup fabrics"""
    xnvme_conf = cijoe.config.options.get("xnvme", None)
    if not xnvme_conf:
        return errno.EINVAL

    xnvme_path = Path(xnvme_conf["source"]["path"])
    rpc = xnvme_path / "spdk" / "scripts" / "rpc.py"
    ip = "127.0.0.1"
    port = "4420"
    trtype = "tcp"
    adrfam = "ipv4"

    modules = ["nvme", "nvmet", "nvmet_tcp", "nvme_fabrics", "nvme_tcp"]

    # Load modules
    for module in modules:
        err, _ = cijoe.run(f"modprobe {module}")
        assert not err

    # Get pcie ids and subnqn
    err, state = cijoe.run("xnvme enum")
    assert not err

    # Get pairs of pcie id and subnqn -> might contain dublicates hence we cast to a set
    devices = set(re.findall(r"{uri: '(.*)'.* subnqn: '(.*)'}", state.output()))

    if not os.path.exists(xnvme_path / "spdk" / "build" / "bin" / "nvmf_tgt"):
        # Build the SPDK NVMe-oF target-app (nvmf_tgt)
        err, _ = cijoe.run("make", cwd=xnvme_path / "spdk" / "app" / "nvmf_tgt")
        assert not err
    else:
        # Kill nvmf_tgt if it is already running
        # We don't care if this fails, probably just means it was not running.
        cijoe.run("pkill -f nvmf_tgt")

    # Start 'nvmf_tgt'
    err, _ = cijoe.run(
        "(nohup ./nvmf_tgt > foo.out 2> foo.err < /dev/null &)",
        cwd=xnvme_path / "spdk" / "build" / "bin",
    )
    assert not err

    # ... and give it two seconds to settle
    sleep(2)

    # Create transport
    err, _ = cijoe.run(
        f"{rpc} nvmf_create_transport -t {trtype} -u 131072 -m 8 -c 8192 -i 524288"
    )
    assert not err

    count = 1
    # Create subsystems, attach controllers and add listener
    for pcie_id, subnqn in devices:
        err, _ = cijoe.run(
            f"{rpc} nvmf_create_subsystem {subnqn} -a -s SPDK0000000000000{count} -d Controller{count}"
        )
        assert not err

        err, state = cijoe.run(
            f"{rpc} bdev_nvme_attach_controller -b nvme{count} -t PCIe -a {pcie_id}"
        )
        assert not err

        for namespace in state.output().split():
            err, _ = cijoe.run(f"{rpc} nvmf_subsystem_add_ns {subnqn} {namespace}")
            assert not err

        err, _ = cijoe.run(
            f"{rpc} nvmf_subsystem_add_listener {subnqn} -t {trtype} -a {ip} -s {port} -f {adrfam}"
        )
        assert not err
        count += 1

    return 0


def fabrics_teardown(cijoe):
    """Teardown fabrics"""
    err, _ = cijoe.run("pkill -f nvmf_tgt")
    return err


class XnvmeDriver(object):
    """
    The driver managing for an NVMe device is in Linux attached to one of:

    * nvme (Kernel managed NVMe driver)
    * vfio-pci (User space)
    * uio-generic (User space)

    Switching attachment is done via the 'xnvme-driver' cli-tool.

    This class encapsulates the cli-tool via static helpers (kernel_detach,
    kernal_attach). Whether or not a device needs one or the other is handled
    explicitly by the key: "driver_attachment" on the device in the configuration.

        driver_attachment: "kernel"

    or

        driver_attachment: "userspace"

    When no "driver_attachment" key is provided, "kernel" is assumed.

    Additionally, since switching device-driver-attachment takes a non-trivial amount of
    time. Especially when doing it repeatedly, as done when executing the testrunner.

    To reduce that processing time, then the XnvmeDriver.prep() function checks whether
    it needs to call the cli-tool tool, by examining XnvmeDriver.ATTACHED.
    """

    IS_KERNEL_ATTACHED = None
    IS_FABRICS_UP = None

    @staticmethod
    def kernel_detach(cijoe):
        """Detach from kernel"""

        if XnvmeDriver.IS_FABRICS_UP in [True, None]:
            err = fabrics_teardown(cijoe)
            if err:
                log.error(f"fabrics teardown failed with error: {err}")
            else:
                XnvmeDriver.IS_FABRICS_UP = False

        cijoe.run("xnvme-driver")
        XnvmeDriver.IS_KERNEL_ATTACHED = False

    @staticmethod
    def kernel_attach(cijoe):
        """Attach to kernel"""

        if XnvmeDriver.IS_FABRICS_UP in [True, None]:
            err = fabrics_teardown(cijoe)
            if err:
                log.error(f"fabrics teardown failed with error: {err}")
            else:
                XnvmeDriver.IS_FABRICS_UP = False

        cijoe.run("xnvme-driver reset")
        XnvmeDriver.IS_KERNEL_ATTACHED = True

    @staticmethod
    def attach(cijoe, device):
        """Attach device driver according to the 'needs' of the given 'device'"""

        needs_kernel = device.get("driver_attachment", "kernel") == "kernel"
        needs_userspace = not needs_kernel
        needs_fabrics = "fabrics" in device["labels"]

        if needs_userspace and XnvmeDriver.IS_KERNEL_ATTACHED in [True, None]:
            XnvmeDriver.kernel_detach(cijoe)
            cijoe.run("xnvme enum")
        elif needs_kernel and XnvmeDriver.IS_KERNEL_ATTACHED in [False, None]:
            XnvmeDriver.kernel_attach(cijoe)
            cijoe.run("xnvme enum")
        else:
            cijoe.run('echo "Skipping XnvmeDriver.attach()."')

        if needs_fabrics and XnvmeDriver.IS_FABRICS_UP in [False, None]:
            err = fabrics_setup(cijoe)
            if err:
                log.error(f"fabrics setup failed with error: {err}")
            else:
                XnvmeDriver.IS_FABRICS_UP = True
        elif not needs_fabrics and XnvmeDriver.IS_FABRICS_UP in [True, None]:
            err = fabrics_teardown(cijoe)
            if err:
                log.error(f"fabrics teardown failed with error: {err}")
            else:
                XnvmeDriver.IS_FABRICS_UP = False


@pytest.fixture
def device(cijoe, request):
    """
    This handles NVMe-device-driver-attachment per parametrized testcase.

    Usage example:

      from cijoe.xnvme.tests.config import xnvme_device_driver as device

      pytest.mark.parametrize("device,be_opts", {data}, indirect=["device"])
      test_foo(cijoe, device, be_opts):
        ...

    By doing so, then each generated parameter for the test-case is sent by this
    fixture. This allows the fixture to invoke device-driver attachment according to
    the given device. This avoids having to call:

      XnvmeDriver.attach(cijoe, request.param)

    from within the testcase body itself.
    """
    if request.param:
        XnvmeDriver.attach(cijoe, request.param)

    return request.param


def xnvme_parametrize(labels, opts):
    """
    This decorator provides all the pytest-parametrization magic in one.

    Instead of decorating tests with, e.g.:

        @pytest.mark.parametrize(
            "device,be_opts,cli_args",
            xnvme_setup(labels=["bdev"], opts=["be", "admin"]),
            indirect=["device"],
        )

    Then they can be decorated with, e.g.:

        @xnvme_parametrize(labels=["bdev"]), opts=["be", "admin"])

    This encapsulates all the pytest-voodoo into the conftest
    """

    def decorator(fun):
        @pytest.mark.parametrize(
            "device,be_opts,cli_args",
            xnvme_setup(labels=labels, opts=opts),
            indirect=["device"],
        )
        @wraps(fun)
        def inner(*args, **kwargs):
            return fun(*args, **kwargs)

        return inner

    return decorator
