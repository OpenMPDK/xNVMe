import os

DEVICE_PATH = os.environ.get("XNVME_URI", "").encode()
BACKEND = os.environ.get("XNVME_BE", "").encode()
DEVICE_NSID = int(os.environ.get("DEVICE_NSID", "1"))