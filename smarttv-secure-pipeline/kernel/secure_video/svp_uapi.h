/* SPDX-License-Identifier: GPL-2.0 */
#pragma once
#include <linux/types.h>

#define SVP_IOC_MAGIC 'S'

enum svp_buf_flags {
    SVP_BUF_SECURE       = 1u << 0,  /* protected buffer (platform-enforced) */
    SVP_BUF_CPU_NOACCESS = 1u << 1,  /* disallow CPU mapping (policy) */
};

struct svp_alloc_req {
    __u32 width;
    __u32 height;
    __u32 fourcc;         /* DRM_FORMAT_* fourcc, e.g. 'NV12' */
    __u32 flags;          /* svp_buf_flags */
    __u32 reserved;
    __s32 out_dmabuf_fd;  /* returned to userspace */
};

struct svp_session_req {
    __u8  session_id[16]; /* opaque */
    __u32 reserved;
};

#define SVP_IOC_ALLOC_BUF     _IOWR(SVP_IOC_MAGIC, 1, struct svp_alloc_req)
#define SVP_IOC_OPEN_SESSION  _IOWR(SVP_IOC_MAGIC, 2, struct svp_session_req)
#define SVP_IOC_CLOSE_SESSION _IOW(SVP_IOC_MAGIC, 3, struct svp_session_req)
