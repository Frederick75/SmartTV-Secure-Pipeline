#include "pipeline.h"
#include "../common/log.h"
#include "../common/fd.h"
#include "../drm/cdm_adapter.h"
#include "../../tee/host/tee_svp_client.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>

extern "C" {
#include "../../kernel/secure_video/svp_uapi.h"
}

#include "rdma_client.h"

static int open_dev(const char* path) {
  int fd = ::open(path, O_RDWR | O_CLOEXEC);
  if (fd < 0) LOGE("open(%s) failed", path);
  return fd;
}

int SecurePipeline::run_demo(const std::string& card,
                             const std::string& heap_hint,
                             int width, int height,
                             bool do_rdma_copy,
                             int frames)
{
  (void)heap_hint; // heap selection is done by svp.ko module param

  auto cdm = CreateCdmAdapter();
  if (!cdm->initialize()) return -1;

  // OP-TEE: import an opaque blob (stub) to demonstrate secure-world call path
  UniqueFd svp_fd(open_dev("/dev/svp0"));
  if (!svp_fd) return -2;

  tee_svp_t* tee = tee_svp_open();
  if (!tee) { LOGE("tee_svp_open failed (is OP-TEE + tee-supplicant running?)"); return -3; }

  std::vector<uint8_t> license_msg(128, 0x11);
  auto lic = cdm->process_license(license_msg);

  if (tee_svp_import_keyblob(tee, lic.key_blob.data(), lic.key_blob.size()) != 0) {
    LOGE("TEE import keyblob failed");
    tee_svp_close(tee);
    return -4;
  }
  LOGI("TEE keyblob import ok (scaffold)");

  // Open SVP session (policy token)
  svp_session_req sess{};
  if (::ioctl(svp_fd.get(), SVP_IOC_OPEN_SESSION, &sess) != 0) {
    LOGE("SVP open session ioctl failed");
    tee_svp_close(tee);
    return -5;
  }

  // Allocate two secure buffers (dmabuf fds)
  svp_alloc_req a{};
  a.width = (uint32_t)width;
  a.height = (uint32_t)height;
  a.fourcc = 0x3231564E; // 'NV12'
  a.flags = SVP_BUF_SECURE | SVP_BUF_CPU_NOACCESS;

  if (::ioctl(svp_fd.get(), SVP_IOC_ALLOC_BUF, &a) != 0) {
    LOGE("SVP alloc A failed");
    tee_svp_close(tee);
    return -6;
  }
  UniqueFd bufA(a.out_dmabuf_fd);

  svp_alloc_req b = a;
  if (::ioctl(svp_fd.get(), SVP_IOC_ALLOC_BUF, &b) != 0) {
    LOGE("SVP alloc B failed");
    tee_svp_close(tee);
    return -7;
  }
  UniqueFd bufB(b.out_dmabuf_fd);

  LOGI("Allocated secure dma-bufs: A=%d B=%d", bufA.get(), bufB.get());

  if (do_rdma_copy) {
    UniqueFd rdma_fd(open_dev("/dev/rdma_stub0"));
    if (!rdma_fd) {
      LOGW("RDMA device not available; skipping copy");
    } else {
      RdmaCopyReq r{};
      r.src_fd = bufA.get();
      r.dst_fd = bufB.get();
      r.src_off = 0;
      r.dst_off = 0;
      r.size = 4096; // demo chunk
      r.flags = 0x1; // secure-policy hint
      int rc = rdma_copy(rdma_fd.get(), r);
      if (rc != 0) {
        LOGW("RDMA copy ioctl failed rc=%d (secure DMA may require vendor integration)", rc);
      } else {
        LOGI("RDMA copy completed (scaffold)");
      }
    }
  }

  // Renderer (GBM + DRM/KMS + EGL): render a test pattern.
  if (!renderer_.init(card)) {
    tee_svp_close(tee);
    return -8;
  }
  LOGI("Rendering test pattern for %d frames", frames);
  renderer_.render_test_pattern(frames);
  renderer_.shutdown();

  // Close SVP session (optional)
  (void)::ioctl(svp_fd.get(), SVP_IOC_CLOSE_SESSION, &sess);

  tee_svp_close(tee);
  return 0;
}
