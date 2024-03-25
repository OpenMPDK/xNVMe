import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync"])
def test_compare(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["sync"] in ["block", "psync"]:
        pytest.skip(reason=f"Not supported: compare via { be_opts['sync'] }")

    err, _ = cijoe.run(f"xnvme_tests_compare compare {cli_args}")
    assert not err
