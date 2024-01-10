def get_combinations():
    """Return all sensible backend configurations"""

    combos = {
        "linux": [],
        "freebsd": [],
        "windows": [],
    }

    combos["linux"] = [
        # Ramdisk
        {
            "be": ["ramdisk"],
            "admin": ["ramdisk"],
            "async": ["emu", "thrpool"],
            "mem": ["posix", "hugepage"],
            "dev": ["nvme"],
            "sync": ["ramdisk"],
            "label": ["bdev", "ramdisk"],
        },
        # File-based I/O
        {
            "be": ["linux"],
            "admin": ["file_as_ns"],
            "async": ["emu", "thrpool"],
            "mem": ["posix", "hugepage"],
            "dev": ["nvme"],
            "sync": ["psync"],
            "label": ["file"],
        },
        # Pseudo-async I/O to block-devices e.g. (/dev/nvme0n1)
        {
            "be": ["linux"],
            "admin": ["nvme", "block"],
            "async": ["emu", "thrpool"],
            "mem": ["posix", "hugepage"],
            "dev": ["nvme"],
            "sync": ["nvme", "psync", "block"],
            "label": ["bdev"],
        },
        # Actual async I/O to block-devices e.g. (/dev/nvme0n1)
        {
            "be": ["linux"],
            "admin": ["nvme", "block"],
            "async": ["posix", "libaio", "io_uring"],
            "mem": ["posix", "hugepage"],
            "dev": ["nvme"],
            "sync": ["nvme"],
            "label": ["bdev"],
        },
        # Pseudo-async I/O to char-devices e.g. (/dev/ng0n1)
        {
            "be": ["linux"],
            "admin": ["nvme"],
            "async": ["emu", "thrpool"],
            "mem": ["posix", "hugepage"],
            "dev": ["nvme"],
            "sync": ["nvme"],
            "label": ["cdev"],
        },
        # Actual async I/O to via char-devices e.g. (/dev/ng0n1)
        {
            "be": ["linux"],
            "mem": ["posix", "hugepage"],
            "dev": ["nvme"],
            "sync": ["nvme"],
            "async": ["io_uring_cmd"],
            "admin": ["nvme"],
            "label": ["cdev"],
        },
        # User-space NVMe-driver
        {
            "be": ["vfio"],
            "mem": ["vfio"],
            "dev": ["nvme"],
            "async": ["vfio"],
            "sync": ["vfio"],
            "admin": ["vfio"],
            "label": ["pcie"],
        },
        {
            "be": ["spdk"],
            "mem": ["spdk"],
            "dev": ["nvme"],
            "async": ["nvme"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["pcie"],
        },
        {
            "be": ["spdk"],
            "mem": ["spdk"],
            "dev": ["nvme"],
            "async": ["nvme"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["fabrics"],
        },
    ]

    combos["freebsd"].append(
        {
            "be": ["spdk"],
            "mem": ["spdk"],
            "dev": ["nvme"],
            "async": ["nvme"],
            "sync": ["nvme"],
            "admin": ["nvme"],
            "label": ["pcie"],
        },
    )
    combos["freebsd"].append(
        {
            "be": ["fbsd"],
            "mem": ["posix"],
            "dev": ["nvme"],
            "async": ["emu", "posix", "thrpool", "kqueue"],
            "sync": ["psync", "nvme"],
            "admin": ["nvme"],
            "label": ["bdev"],
        },
    )

    return combos
