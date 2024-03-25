import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


def test_enum(cijoe):
    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("lblk enum")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("lblk enum")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_info(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"lblk info {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"lblk idfy {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_read(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"lblk read {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_write(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"lblk write {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_compare(cijoe, device, be_opts, cli_args):
    src = "/tmp/file.bin"

    err, _ = cijoe.run(f"lblk read {cli_args} --slba 0x0 --nlb 0 --data-output {src}")
    err, _ = cijoe.run(f"lblk compare {cli_args} --slba 0x0 --nlb 0 --data-input {src}")
    assert not err


@xnvme_parametrize(labels=["write_uncor"], opts=["be", "admin"])
def test_write_uncor(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"lblk write-uncor {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["write_zeroes"], opts=["be", "admin"])
def test_write_zeroes(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"lblk write-zeros {cli_args} --slba 0x0 --nlb 0")
    assert not err


@xnvme_parametrize(labels=["pi1"], opts=["be", "admin"])
def test_pi_type1(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support metadata")

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 0 --pract 1 --prchk 0x5"
    )
    assert not err

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 4 --prchk 0x7 --apptag 0x1234 --apptag_mask 0xFFFF"
    )
    assert not err


@xnvme_parametrize(labels=["pi2"], opts=["be", "admin"])
def test_pi_type2(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support metadata")

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 0 --pract 1 --prchk 0x5"
    )
    assert not err

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 4 --prchk 0x7 --apptag 0x1234 --apptag_mask 0xFFFF"
    )
    assert not err


@xnvme_parametrize(labels=["pi3"], opts=["be", "admin"])
def test_pi_type3(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support metadata")

    # For PI type 3 prchk 0bxx1 is invalid
    # Thus using prchk 0b100
    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 0 --pract 1 --prchk 0x4"
    )
    assert not err

    # And prchk 0b110
    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 4 --prchk 0x6 --apptag 0x1234 --apptag_mask 0xFFFF"
    )
    assert not err


@xnvme_parametrize(labels=["pi1_ex"], opts=["be", "admin"])
def test_pi_type1_extended_lba(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support metadata")

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 0 --pract 1 --prchk 0x5"
    )
    assert not err

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 4 --prchk 0x7 --apptag 0x1234 --apptag_mask 0xFFFF"
    )
    assert not err


@xnvme_parametrize(labels=["pi1_pif2"], opts=["be", "admin"])
def test_pi1_pif2(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support metadata")

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 0 --pract 1 --prchk 0x5"
    )
    assert not err

    err, _ = cijoe.run(
        f"lblk write-read-pi {cli_args} --slba 0x0 --nlb 4 --prchk 0x7 --apptag 0x1234 --apptag_mask 0xFFFF"
    )
    assert not err
