/* SPDX-License-Identifier: GPL-2.0 */
#pragma once
#include <linux/types.h>

#define RDMA_IOC_MAGIC 'R'

struct rdma_copy_req {
    __s32 src_dmabuf_fd;
    __s32 dst_dmabuf_fd;
    __u32 src_offset;
    __u32 dst_offset;
    __u32 size;
    __u32 flags; /* bit0: secure-policy */
};

#define RDMA_IOC_COPY _IOW(RDMA_IOC_MAGIC, 1, struct rdma_copy_req)
