#include "rdma_client.h"
#include <sys/ioctl.h>

extern "C" {
#include "../../kernel/rdma_stub/rdma_stub_uapi.h"
}

int rdma_copy(int rdma_dev_fd, const RdmaCopyReq& r)
{
  rdma_copy_req req{};
  req.src_dmabuf_fd = r.src_fd;
  req.dst_dmabuf_fd = r.dst_fd;
  req.src_offset = r.src_off;
  req.dst_offset = r.dst_off;
  req.size = r.size;
  req.flags = r.flags;
  return ioctl(rdma_dev_fd, RDMA_IOC_COPY, &req);
}
