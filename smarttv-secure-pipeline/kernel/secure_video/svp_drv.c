/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "svp_uapi.h"

/* Alloc+export implemented in svp_dmabuf_dmaheap.c */
extern int svp_dmabuf_alloc_export_fd(u32 w, u32 h, u32 fourcc, u32 flags, int *out_fd);

static dev_t svp_dev;
static struct cdev svp_cdev;
static struct class *svp_class;
static DEFINE_MUTEX(svp_lock);

static long svp_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    (void)f;
    if (_IOC_TYPE(cmd) != SVP_IOC_MAGIC)
        return -ENOTTY;

    mutex_lock(&svp_lock);

    switch (cmd) {
    case SVP_IOC_ALLOC_BUF: {
        struct svp_alloc_req req;
        int fd = -1, ret;

        if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
            mutex_unlock(&svp_lock);
            return -EFAULT;
        }

        ret = svp_dmabuf_alloc_export_fd(req.width, req.height, req.fourcc, req.flags, &fd);
        if (ret) {
            mutex_unlock(&svp_lock);
            return ret;
        }

        req.out_dmabuf_fd = fd;
        if (copy_to_user((void __user *)arg, &req, sizeof(req))) {
            mutex_unlock(&svp_lock);
            return -EFAULT;
        }
        break;
    }
    case SVP_IOC_OPEN_SESSION: {
        /* In production: tie this to OP-TEE session authorization (policy gate). */
        struct svp_session_req s;
        if (copy_from_user(&s, (void __user *)arg, sizeof(s))) {
            mutex_unlock(&svp_lock);
            return -EFAULT;
        }
        memset(s.session_id, 0xAB, sizeof(s.session_id)); /* demo token */
        if (copy_to_user((void __user *)arg, &s, sizeof(s))) {
            mutex_unlock(&svp_lock);
            return -EFAULT;
        }
        break;
    }
    case SVP_IOC_CLOSE_SESSION:
        /* no-op in scaffold */
        break;
    default:
        mutex_unlock(&svp_lock);
        return -ENOTTY;
    }

    mutex_unlock(&svp_lock);
    return 0;
}

static const struct file_operations svp_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = svp_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = svp_ioctl,
#endif
};

static int __init svp_init(void)
{
    int ret = alloc_chrdev_region(&svp_dev, 0, 1, "svp");
    if (ret)
        return ret;

    cdev_init(&svp_cdev, &svp_fops);
    ret = cdev_add(&svp_cdev, svp_dev, 1);
    if (ret)
        goto err_chr;

    svp_class = class_create(THIS_MODULE, "svp");
    if (IS_ERR(svp_class)) {
        ret = PTR_ERR(svp_class);
        goto err_cdev;
    }

    device_create(svp_class, NULL, svp_dev, NULL, "svp0");
    pr_info("svp: loaded (/dev/svp0)\n");
    return 0;

err_cdev:
    cdev_del(&svp_cdev);
err_chr:
    unregister_chrdev_region(svp_dev, 1);
    return ret;
}

static void __exit svp_exit(void)
{
    device_destroy(svp_class, svp_dev);
    class_destroy(svp_class);
    cdev_del(&svp_cdev);
    unregister_chrdev_region(svp_dev, 1);
    pr_info("svp: unloaded\n");
}

module_init(svp_init);
module_exit(svp_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Secure Video Path char driver (DMA-HEAP secure allocator scaffold)");
