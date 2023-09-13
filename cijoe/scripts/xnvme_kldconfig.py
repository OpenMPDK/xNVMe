"""
Add the SPDK kmod directory to FreeBSD kernel module search path
================================================================

This is needed by the ``xnvme-driver`` script.

Retargetable: True
------------------
"""


def main(args, cijoe, step):
    osname = cijoe.config.options.get("os", {}).get("name", None)
    if not osname:
        return 0
    if osname != "freebsd":
        return 0

    xnvme_source = step.get("with", {}).get("xnvme_source", "/tmp/xnvme_source")

    cijoe.run(f"kldconfig -i -m -vv {xnvme_source}/dpdk/build/kmod")
    return 0
