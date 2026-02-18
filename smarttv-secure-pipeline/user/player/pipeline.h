#pragma once
#include <string>
#include "../renderer/gbm_kms_renderer.h"

class SecurePipeline {
public:
  // Runs a demo: TEE import, SVP alloc, optional RDMA copy, then render test pattern.
  int run_demo(const std::string& card,
               const std::string& heap_hint,
               int width, int height,
               bool do_rdma_copy,
               int frames);

private:
  GbmKmsRenderer renderer_;
};
