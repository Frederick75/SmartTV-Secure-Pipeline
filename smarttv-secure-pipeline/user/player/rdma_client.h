#pragma once
#include <cstdint>

struct RdmaCopyReq {
  int src_fd;
  int dst_fd;
  uint32_t src_off;
  uint32_t dst_off;
  uint32_t size;
  uint32_t flags; // bit0 secure-policy
};

int rdma_copy(int rdma_dev_fd, const RdmaCopyReq& r);
