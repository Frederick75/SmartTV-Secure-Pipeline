#pragma once
#include <string>

class GbmKmsRenderer {
public:
  bool init(const std::string& card_path);
  void shutdown();

  // Present a simple test pattern (no dmabuf sampling required to compile/run).
  bool render_test_pattern(int frames);

private:
  int drm_fd_ = -1;
  void* gbm_dev_ = nullptr;
  void* gbm_surf_ = nullptr;

  void* egl_display_ = nullptr;
  void* egl_context_ = nullptr;
  void* egl_surface_ = nullptr;

  unsigned int crtc_id_ = 0;
  unsigned int conn_id_ = 0;
  unsigned int fb_id_ = 0;
  unsigned int width_ = 0;
  unsigned int height_ = 0;
};
