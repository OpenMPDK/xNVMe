import ctypes
import os

import conftest
import numpy as np
import pytest
import xnvme.ctypes_bindings.api as xnvme

pytest.skip("Needs to be ported from Cython to Ctypes", allow_module_level=True)

NULL = None
UINT16_MAX = 0xFFFF


@pytest.fixture
def boilerplate(dev):
    geo = xnvme.xnvme_dev_get_geo(dev)
    nsid = xnvme.xnvme_dev_get_nsid(dev)

    rng_slba = 0
    rng_elba = (1 << 26) // geo.lba_nbytes  # About 64MB
    # rng_elba = (1 << 28) // geo.lba_nbytes # About 256MB

    mdts_naddr = min(geo.mdts_nbytes // geo.lba_nbytes, 256)
    buf_nbytes = mdts_naddr * geo.lba_nbytes

    # Verify range
    assert rng_elba >= rng_slba, "Invalid range: [rng_slba,rng_elba]"

    # TODO: verify that the range is sufficiently large

    wbuf = xnvme.xnvme_buf_alloc(dev, buf_nbytes)
    rbuf = xnvme.xnvme_buf_alloc(dev, buf_nbytes)

    yield (dev, geo, wbuf, rbuf, buf_nbytes, mdts_naddr, nsid, rng_slba, rng_elba)

    xnvme.xnvme_buf_free(dev, wbuf)
    xnvme.xnvme_buf_free(dev, rbuf)


def fill_lba_range_and_write_buffer_with_character(
    wbuf,
    buf_nbytes,
    rng_slba,
    rng_elba,
    mdts_naddr,
    dev,
    geo,
    nsid,
    character,
):
    ctypes.memset(ctypes.c_void_p(wbuf.void_pointer), ord(character), buf_nbytes)

    written_bytes = 0
    for slba in range(rng_slba, rng_elba, mdts_naddr):
        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        nlb = min(rng_elba - slba, mdts_naddr - 1)

        written_bytes += (1 + nlb) * geo.lba_nbytes

        err = xnvme.xnvme_nvm_write(ctx, nsid, slba, nlb, wbuf, NULL)
        assert not (
            err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)
        ), f"xnvme_nvm_write(): {{err: 0x{err:x}, slba: 0x{slba:016x}}}"
    return written_bytes


class TestLBLK:
    @pytest.mark.skipif(
        conftest.BACKEND == b"spdk",
        reason="SPDK cannot handle a dev open/close followed by enumerate",
    )
    @pytest.mark.skipif(
        conftest.BACKEND == b"ramdisk",
        reason="ramdisk doesn't support enumeration",
    )
    def test_enum(self, opts):
        def callback_func(dev, fd):
            ident = xnvme.xnvme_dev_get_ident(dev)
            if ident.csi != xnvme.XNVME_SPEC_CSI_NVM:
                return xnvme.XNVME_ENUMERATE_DEV_CLOSE

            xnvme.xnvme_ident_yaml(fd, ident, 0, b", ", 0)

            return xnvme.XNVME_ENUMERATE_DEV_CLOSE

        fd = xnvme.FILE()
        fd.tmpfile()
        xnvme.xnvme_enumerate(None, opts, callback_func, fd)
        data = fd.getvalue()
        print(data.decode())
        assert len(data) != 0

    # /**
    #  * 0) Fill wbuf with '!'
    #  * 1) Write the entire LBA range [slba, elba] using wbuf
    #  * 2) Fill wbuf with a repeating sequence of letters A to Z
    #  * 3) Scatter the content of wbuf within [slba,elba]
    #  * 4) Read, with exponential stride, within [slba,elba] using rbuf
    #  * 5) Verify that the content of rbuf is the same as wbuf
    #  */
    def test_verify_io(self, boilerplate):
        (
            dev,
            geo,
            wbuf,
            rbuf,
            buf_nbytes,
            mdts_naddr,
            nsid,
            rng_slba,
            rng_elba,
        ) = boilerplate

        print("Writing '!' to LBA range [slba,elba]")
        fill_lba_range_and_write_buffer_with_character(
            wbuf, buf_nbytes, rng_slba, rng_elba, mdts_naddr, dev, geo, nsid, "!"
        )

        print("Writing payload scattered within LBA range [slba,elba]")
        wbuf_mem_view = np.ctypeslib.as_array(
            ctypes.cast(wbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
            shape=(buf_nbytes,),
        )
        wbuf_mem_view[:] = np.arange(len(wbuf_mem_view))
        # xnvme.xnvmec_buf_fill(wbuf, buf_nbytes, "anum")

        for count in range(mdts_naddr):
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)  # TODO: Reuse from before?
            wbuf_ofz = count * geo.lba_nbytes
            slba = rng_slba + count * 4

            err = xnvme.xnvme_nvm_write(
                ctx,
                nsid,
                slba,
                0,
                xnvme.xnvme_void_p(wbuf.void_pointer + wbuf_ofz),
                NULL,
            )
            assert not (
                err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)
            ), f"xnvme_nvm_write(): {{err: 0x{err:x}, slba: 0x{slba:016x}}}"
            # xnvme.xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF)

        print("Read scattered payload within LBA range [slba,elba]")

        rbuf_mem_view = np.ctypeslib.as_array(
            ctypes.cast(rbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
            shape=(buf_nbytes,),
        )
        rbuf_mem_view[:] = 0
        assert not np.all(rbuf_mem_view == wbuf_mem_view)
        # xnvme.xnvmec_buf_clear(rbuf, buf_nbytes)
        for count in range(mdts_naddr):
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
            rbuf_ofz = count * geo.lba_nbytes
            slba = rng_slba + count * 4

            err = xnvme.xnvme_nvm_read(
                ctx,
                nsid,
                slba,
                0,
                xnvme.xnvme_void_p(rbuf.void_pointer + rbuf_ofz),
                NULL,
            )
            assert not (
                err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)
            ), "xnvme_nvm_read(): {{err: 0x{err:x}, slba: 0x{slba:016x}}}"

        assert np.all(rbuf_mem_view == wbuf_mem_view), xnvme.xnvmec_buf_diff_pr(
            wbuf, rbuf, buf_nbytes, xnvme.XNVME_PR_DEF
        )
        # assert not xnvme.xnvmec_buf_diff(wbuf, rbuf, buf_nbytes)

    # /**
    #  * 0) Fill wbuf with '!'
    #  *
    #  * 1) Write the entire LBA range [slba, elba] using wbuf
    #  *
    #  * 2) Fill wbuf with a repeating sequence of letters A to Z
    #  *
    #  * 3) Scatter the content of wbuf within [slba,elba]
    #  *
    #  * 4) Read, with exponential stride, within [slba,elba] using rbuf
    #  *
    #  * 5) Verify that the content of rbuf is the same as wbuf
    #  */
    @pytest.mark.skipif(True, reason="Unfinished")
    def test_scopy(self, boilerplate):
        # struct xnvme_spec_nvm_scopy_source_range *sranges = NULL # For the copy-payload
        sranges = []

        (
            dev,
            geo,
            wbuf,
            rbuf,
            buf_nbytes,
            xfer_naddr,
            nsid,
            rng_slba,
            rng_elba,
        ) = boilerplate

        # Force void* casting to xnvme_spec_nvm_idfy_ns*
        # xnvme_spec_nvm_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev)
        _nvm = xnvme.xnvme_dev_get_ns(dev)
        _void_p = xnvme.xnvme_void_p(_nvm.void_pointer)
        nvm = xnvme.xnvme_spec_nvm_idfy_ns(__void_p=_void_p)
        xnvme.xnvme_spec_nvm_idfy_ns_pr(nvm, xnvme.XNVME_PR_DEF)

        if nvm.msrc:
            xfer_naddr = min(min(nvm.msrc + 1, xfer_naddr), nvm.mcl)
        buf_nbytes = xfer_naddr * geo.nbytes

        # sranges = xnvme_buf_alloc(dev, xnvme_spec_nvm_scopy_source_range().sizeof)
        # memset(sranges, 0, sizeof(*sranges))

        # Copy to the end of [slba,elba]
        sdlba = rng_elba - xfer_naddr

        # NVMe-struct copy format
        copy_fmt = xnvme.XNVME_NVM_SCOPY_FMT_ZERO

        xnvme.xnvme_spec_idfy_ctrlr_pr(
            xnvme.xnvme_spec_idfy_ctrlr(
                __void_p=xnvme.xnvme_void_p(xnvme.xnvme_dev_get_ctrlr(dev).void_pointer)
            ),
            xnvme.XNVME_PR_DEF,
        )
        xnvme.xnvme_spec_nvm_idfy_ns_pr(
            xnvme.xnvme_spec_nvm_idfy_ns(
                __void_p=xnvme.xnvme_void_p(xnvme.xnvme_dev_get_ns(dev).void_pointer)
            ),
            xnvme.XNVME_PR_DEF,
        )

        fill_lba_range_and_write_buffer_with_character(
            wbuf, buf_nbytes, rng_slba, rng_elba, xfer_naddr, dev, geo, nsid, "!"
        )

        print("Writing payload scattered within LBA range [slba,elba]")
        wbuf_mem_view = np.ctypeslib.as_array(
            ctypes.cast(wbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
            shape=(buf_nbytes,),
        )
        wbuf_mem_view[:] = np.arange(len(wbuf_mem_view))
        # xnvmec_buf_fill(wbuf, buf_nbytes, "anum")
        for count in range(xfer_naddr):
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
            wbuf_ofz = count * geo.lba_nbytes
            slba = rng_slba + count * 4

            sranges.append((slba, 0))
            # sranges.entry[count].slba = slba
            # sranges.entry[count].nlb = 0

            err = xnvme.xnvme_nvm_write(
                ctx,
                nsid,
                slba,
                0,
                xnvme.xnvme_void_p(wbuf.void_pointer + wbuf_ofz),
                NULL,
            )
            assert not (
                err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)
            ), xnvme.xnvme_cmd_ctx_pr(ctx, xnvme.XNVME_PR_DEF)

        ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
        nr = xfer_naddr - 1

        print("scopy sranges to sdlba: 0x{sdlba:016x}")
        print(sranges)
        # xnvme_spec_nvm_scopy_source_range_pr(sranges, nr, XNVME_PR_DEF)

        err = xnvme.xnvme_nvm_scopy(ctx, nsid, sdlba, sranges.entry, nr, copy_fmt)
        assert not (err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)), xnvme.xnvme_cmd_ctx_pr(
            ctx, xnvme.XNVME_PR_DEF
        )

        print("read sdlba: 0x{sdlba:016x}")
        assert False, "Unimplemented"
        # memset(rbuf, 0, buf_nbytes)
        # err = xnvme.xnvme_nvm_read(ctx, nsid, sdlba, nr, rbuf, NULL)
        # assert not (err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)), xnvme.xnvme_cmd_ctx_pr(
        #     ctx, xnvme.XNVME_PR_DEF
        # )

        # print("Comparing wbuf and rbuf")
        # assert np.all(rbuf_mem_view == wbuf_mem_view), xnvme.xnvmec_buf_diff_pr(
        #     wbuf, rbuf, buf_nbytes, xnvme.XNVME_PR_DEF
        # )

        # xnvme.xnvme_buf_free(dev, sranges)

    def read_and_compare_lba_range(
        self, rbuf, cbuf, rng_slba, nlb, mdts_naddr, geo, dev, nsid
    ):
        compared_bytes = 0
        print("Reading and comparing in LBA range [%ld,%ld]", rng_slba, rng_slba + nlb)

        read_lbs = 0
        r_nlb = 0
        while read_lbs < nlb:
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
            r_nlb = min(mdts_naddr, nlb - read_lbs)

            err = xnvme.xnvme_nvm_read(
                ctx, nsid, rng_slba + read_lbs, r_nlb - 1, rbuf, NULL
            )
            assert not (
                err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)
            ), "xnvme_nvm_read(): {err: 0x{err:0x}, slba: 0x{rng_slba + read_lbs:016x}}"
            # xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF)

            read_bytes = r_nlb * geo.lba_nbytes

            cbuf_mem_view = np.ctypeslib.as_array(
                ctypes.cast(cbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
                shape=(read_bytes,),
            )
            rbuf_mem_view = np.ctypeslib.as_array(
                ctypes.cast(rbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
                shape=(read_bytes,),
            )
            assert np.all(cbuf_mem_view == rbuf_mem_view), xnvme.xnvmec_buf_diff_pr(
                cbuf, rbuf, read_bytes, xnvme.XNVME_PR_DEF
            )
            compared_bytes += read_bytes

            read_lbs += r_nlb
        return compared_bytes

    # /**
    #  * 0) Fill wbuf with '!'
    #  * 1) Write the entire LBA range [slba, elba] using wbuf
    #  * 2) Make sure that we wrote '!'
    #  * 3) Execute the write zeroes command
    #  * 4) Fill wbuf with 0
    #  * 5) Read, with exponential stride, within [slba,elba] using rbuf
    #  * 6) Verify that the content of rbuf is the same as wbuf
    #  */
    @pytest.mark.skipif(True, reason="Unfinished. Backend has to support write-zero")
    def test_write_zeroes(self, boilerplate):
        (
            dev,
            geo,
            wbuf,
            rbuf,
            buf_nbytes,
            mdts_naddr,
            nsid,
            rng_slba,
            rng_elba,
        ) = boilerplate
        nlb = rng_elba - rng_slba

        written_bytes = fill_lba_range_and_write_buffer_with_character(
            wbuf, buf_nbytes, rng_slba, rng_elba, mdts_naddr, dev, geo, nsid, "!"
        )

        print(f"Written bytes {written_bytes} with !")

        rbuf_mem_view = np.ctypeslib.as_array(
            ctypes.cast(rbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
            shape=(buf_nbytes,),
        )
        rbuf_mem_view[:] = 0
        compared_bytes = self.read_and_compare_lba_range(
            rbuf, wbuf, rng_slba, nlb, mdts_naddr, geo, dev, nsid
        )

        print(f"Compared {compared_bytes} bytes to !")

        for slba in range(rng_slba, rng_elba, mdts_naddr):
            ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
            nlb = min((rng_elba - slba) * geo.lba_nbytes, UINT16_MAX)

            err = xnvme.xnvme_nvm_write_zeroes(ctx, nsid, slba, nlb)
            assert not (err or xnvme.xnvme_cmd_ctx_cpl_status(ctx)), os.strerror(
                abs(err)
            )

        print("Wrote zeroes to LBA range [{rng_slba},{rng_elba}]")

        # Set the rbuf to != 0 so we know that we read zeroes
        rbuf_mem_view = np.ctypeslib.as_array(
            ctypes.cast(rbuf.void_pointer, ctypes.POINTER(ctypes.c_uint8)),
            shape=(buf_nbytes,),
        )
        rbuf_mem_view[:] = b"a"

        xnvme.xnvmec_buf_clear(wbuf, buf_nbytes)
        compared_bytes = self.read_and_compare_lba_range(
            rbuf, wbuf, rng_slba, nlb, mdts_naddr, geo, dev, nsid
        )
        print(f"Compared {compared_bytes} bytes to zero")
