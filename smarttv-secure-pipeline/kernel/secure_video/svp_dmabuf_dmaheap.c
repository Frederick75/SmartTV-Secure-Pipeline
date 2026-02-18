/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/err.h>

#include "svp_uapi.h"

/*
 * Vendor heap names vary. Common examples:
 *  - "secure"
 *  - "svp"
 *  - "mtk_svp"
 * Inspect /sys/kernel/dma_heap/ or /dev/dma_heap/ on the target.
 */
static char *svp_heap_name = "secure";
module_param(svp_heap_name, charp, 0444);
MODULE_PARM_DESC(svp_heap_name, "DMA-HEAP name used for secure video buffers");

static size_t svp_calc_size(u32 w, u32 h, u32 fourcc)
{
    (void)fourcc;
    /* Demo: NV12 size approximation. Production: align stride/height to HW constraints. */
    return (size_t)w * (size_t)h * 3 / 2;
}

int svp_dmabuf_alloc_export_fd(u32 w, u32 h, u32 fourcc, u32 flags, int *out_fd)
{
    struct dma_heap *heap;
    struct dma_buf *dbuf;
    size_t size;
    int fd;

    if (!out_fd || w == 0 || h == 0)
        return -EINVAL;

    size = svp_calc_size(w, h, fourcc);

    heap = dma_heap_find(svp_heap_name);
    if (!heap) {
        pr_err("svp: dma_heap '%s' not found\n", svp_heap_name);
        return -ENODEV;
    }

    /* heap_flags is vendor-defined; 0 is typically OK */
    dbuf = dma_heap_buffer_alloc(heap, size, O_RDWR | O_CLOEXEC, 0);
    dma_heap_put(heap);

    if (IS_ERR(dbuf)) {
        pr_err("svp: dma_heap alloc failed (%ld)\n", PTR_ERR(dbuf));
        return PTR_ERR(dbuf);
    }

    fd = dma_buf_fd(dbuf, O_CLOEXEC);
    if (fd < 0) {
        dma_buf_put(dbuf);
        return fd;
    }

    /* flags are policy hints; secure enforcement is heap/platform-level. */
    (void)flags;
    *out_fd = fd;
    return 0;
}
EXPORT_SYMBOL_GPL(svp_dmabuf_alloc_export_fd);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SVP DMA-HEAP secure allocator (scaffold)");
