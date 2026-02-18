/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/dmaengine.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include <linux/mutex.h>
#include <linux/completion.h>

#include "rdma_stub_uapi.h"

static dev_t rdma_dev;
static struct cdev rdma_cdev;
static struct class *rdma_class;
static DEFINE_MUTEX(rdma_lock);

static struct dma_chan *chan;

static void rdma_dma_cb(void *param)
{
    complete(param);
}

static int map_dmabuf_sg(struct device *dev,
                         struct dma_buf *dbuf,
                         struct dma_buf_attachment **out_att,
                         struct sg_table **out_sgt,
                         enum dma_data_direction dir)
{
    struct dma_buf_attachment *att;
    struct sg_table *sgt;

    att = dma_buf_attach(dbuf, dev);
    if (IS_ERR(att))
        return PTR_ERR(att);

    sgt = dma_buf_map_attachment(att, dir);
    if (IS_ERR(sgt)) {
        dma_buf_detach(dbuf, att);
        return PTR_ERR(sgt);
    }

    *out_att = att;
    *out_sgt = sgt;
    return 0;
}

static void unmap_dmabuf_sg(struct dma_buf *dbuf,
                           struct dma_buf_attachment *att,
                           struct sg_table *sgt,
                           enum dma_data_direction dir)
{
    dma_buf_unmap_attachment(att, sgt, dir);
    dma_buf_detach(dbuf, att);
}

static long rdma_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct rdma_copy_req req;
    struct dma_buf *src = NULL, *dst = NULL;
    struct dma_buf_attachment *src_att = NULL, *dst_att = NULL;
    struct sg_table *src_sgt = NULL, *dst_sgt = NULL;
    dma_addr_t src_dma, dst_dma;
    struct dma_async_tx_descriptor *tx;
    dma_cookie_t cookie;
    DECLARE_COMPLETION_ONSTACK(done);
    int ret = 0;

    (void)f;

    if (_IOC_TYPE(cmd) != RDMA_IOC_MAGIC)
        return -ENOTTY;
    if (cmd != RDMA_IOC_COPY)
        return -ENOTTY;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
        return -EFAULT;

    if (req.size == 0)
        return -EINVAL;

    mutex_lock(&rdma_lock);

    if (!chan) { ret = -ENODEV; goto out; }

    /*
     * Secure-policy hook:
     * For true secure-copy you typically need vendor secure DMA channel / secure IOMMU domain.
     * Add vendor integration here if required.
     */
    if (req.flags & 0x1) {
        /* TODO: vendor_secure_dma_prepare(chan, ...); */
    }

    src = dma_buf_get(req.src_dmabuf_fd);
    if (IS_ERR(src)) { ret = PTR_ERR(src); src = NULL; goto out; }

    dst = dma_buf_get(req.dst_dmabuf_fd);
    if (IS_ERR(dst)) { ret = PTR_ERR(dst); dst = NULL; goto out; }

    ret = map_dmabuf_sg(chan->device->dev, src, &src_att, &src_sgt, DMA_TO_DEVICE);
    if (ret) goto out;

    ret = map_dmabuf_sg(chan->device->dev, dst, &dst_att, &dst_sgt, DMA_FROM_DEVICE);
    if (ret) goto out;

    if (!src_sgt->sgl || !dst_sgt->sgl) { ret = -EINVAL; goto out; }

    /*
     * Simplification: use first SG entry only.
     * Production: walk SG and handle offsets / chunking properly.
     */
    src_dma = sg_dma_address(src_sgt->sgl) + req.src_offset;
    dst_dma = sg_dma_address(dst_sgt->sgl) + req.dst_offset;

    tx = dmaengine_prep_dma_memcpy(chan, dst_dma, src_dma, req.size,
                                   DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
    if (!tx) { ret = -EIO; goto out; }

    tx->callback = rdma_dma_cb;
    tx->callback_param = &done;

    cookie = dmaengine_submit(tx);
    ret = dma_submit_error(cookie);
    if (ret) goto out;

    dma_async_issue_pending(chan);

    if (!wait_for_completion_timeout(&done, msecs_to_jiffies(2000))) {
        dmaengine_terminate_sync(chan);
        ret = -ETIMEDOUT;
        goto out;
    }

out:
    if (src_sgt && src_att) unmap_dmabuf_sg(src, src_att, src_sgt, DMA_TO_DEVICE);
    if (dst_sgt && dst_att) unmap_dmabuf_sg(dst, dst_att, dst_sgt, DMA_FROM_DEVICE);
    if (src) dma_buf_put(src);
    if (dst) dma_buf_put(dst);

    mutex_unlock(&rdma_lock);
    return ret;
}

static const struct file_operations rdma_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = rdma_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = rdma_ioctl,
#endif
};

static int __init rdma_init(void)
{
    int ret;
    dma_cap_mask_t mask;

    dma_cap_zero(mask);
    dma_cap_set(DMA_MEMCPY, mask);

    chan = dma_request_channel(mask, NULL, NULL);
    if (!chan)
        pr_warn("rdma_stub: no DMA_MEMCPY channel; /dev/rdma_stub0 will exist but COPY will fail\n");

    ret = alloc_chrdev_region(&rdma_dev, 0, 1, "rdma_stub");
    if (ret) return ret;

    cdev_init(&rdma_cdev, &rdma_fops);
    ret = cdev_add(&rdma_cdev, rdma_dev, 1);
    if (ret) goto err_chr;

    rdma_class = class_create(THIS_MODULE, "rdma_stub");
    if (IS_ERR(rdma_class)) { ret = PTR_ERR(rdma_class); goto err_cdev; }

    device_create(rdma_class, NULL, rdma_dev, NULL, "rdma_stub0");
    pr_info("rdma_stub: loaded (/dev/rdma_stub0)\n");
    return 0;

err_cdev:
    cdev_del(&rdma_cdev);
err_chr:
    unregister_chrdev_region(rdma_dev, 1);
    return ret;
}

static void __exit rdma_exit(void)
{
    if (chan) dma_release_channel(chan);
    device_destroy(rdma_class, rdma_dev);
    class_destroy(rdma_class);
    cdev_del(&rdma_cdev);
    unregister_chrdev_region(rdma_dev, 1);
    pr_info("rdma_stub: unloaded\n");
}

module_init(rdma_init);
module_exit(rdma_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RDMA-style dma-buf memcpy via DMAengine (scaffold, Linux 5.10)");
