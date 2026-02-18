#include "gbm_kms_renderer.h"
#include "../common/log.h"
#include "../common/fd.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

static drmModeConnector* find_connected_connector(int fd, drmModeRes* res, uint32_t* out_conn_id) {
  for (int i = 0; i < res->count_connectors; ++i) {
    drmModeConnector* conn = drmModeGetConnector(fd, res->connectors[i]);
    if (!conn) continue;
    if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
      *out_conn_id = conn->connector_id;
      return conn;
    }
    drmModeFreeConnector(conn);
  }
  return nullptr;
}

static uint32_t find_crtc_for_connector(int fd, drmModeRes* res, drmModeConnector* conn) {
  for (int i = 0; i < conn->count_encoders; ++i) {
    drmModeEncoder* enc = drmModeGetEncoder(fd, conn->encoders[i]);
    if (!enc) continue;
    uint32_t crtc_id = enc->crtc_id;
    drmModeFreeEncoder(enc);
    if (crtc_id) return crtc_id;
  }
  // Fallback: first CRTC
  if (res->count_crtcs > 0) return res->crtcs[0];
  return 0;
}

bool GbmKmsRenderer::init(const std::string& card_path) {
  drm_fd_ = ::open(card_path.c_str(), O_RDWR | O_CLOEXEC);
  if (drm_fd_ < 0) {
    LOGE("Failed to open DRM card: %s", card_path.c_str());
    return false;
  }

  drmModeRes* res = drmModeGetResources(drm_fd_);
  if (!res) {
    LOGE("drmModeGetResources failed");
    return false;
  }

  uint32_t conn_id = 0;
  drmModeConnector* conn = find_connected_connector(drm_fd_, res, &conn_id);
  if (!conn) {
    drmModeFreeResources(res);
    LOGE("No connected connector found");
    return false;
  }

  drmModeModeInfo mode = conn->modes[0];
  width_ = mode.hdisplay;
  height_ = mode.vdisplay;

  uint32_t crtc_id = find_crtc_for_connector(drm_fd_, res, conn);
  drmModeFreeConnector(conn);
  drmModeFreeResources(res);

  if (!crtc_id) {
    LOGE("No CRTC found");
    return false;
  }

  conn_id_ = conn_id;
  crtc_id_ = crtc_id;

  gbm_device* gbm = gbm_create_device(drm_fd_);
  if (!gbm) {
    LOGE("gbm_create_device failed");
    return false;
  }
  gbm_dev_ = gbm;

  gbm_surface* surf = gbm_surface_create(
      gbm,
      width_, height_,
      GBM_FORMAT_XRGB8888,
      GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  if (!surf) {
    LOGE("gbm_surface_create failed");
    return false;
  }
  gbm_surf_ = surf;

  // EGL init via EGL platform GBM
  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  if (!eglGetPlatformDisplayEXT) {
    LOGE("eglGetPlatformDisplayEXT not available (need EGL_KHR_platform_gbm / EGL_EXT_platform_base)");
    return false;
  }

  EGLDisplay dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, (void*)gbm, nullptr);
  if (dpy == EGL_NO_DISPLAY) {
    LOGE("eglGetPlatformDisplayEXT returned NO_DISPLAY");
    return false;
  }

  if (!eglInitialize(dpy, nullptr, nullptr)) {
    LOGE("eglInitialize failed");
    return false;
  }

  static const EGLint cfg_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  EGLConfig cfg;
  EGLint ncfg = 0;
  if (!eglChooseConfig(dpy, cfg_attribs, &cfg, 1, &ncfg) || ncfg < 1) {
    LOGE("eglChooseConfig failed");
    return false;
  }

  static const EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctx_attribs);
  if (ctx == EGL_NO_CONTEXT) {
    LOGE("eglCreateContext failed");
    return false;
  }

  EGLSurface esurf = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)surf, nullptr);
  if (esurf == EGL_NO_SURFACE) {
    LOGE("eglCreateWindowSurface failed");
    return false;
  }

  if (!eglMakeCurrent(dpy, esurf, esurf, ctx)) {
    LOGE("eglMakeCurrent failed");
    return false;
  }

  egl_display_ = (void*)dpy;
  egl_context_ = (void*)ctx;
  egl_surface_ = (void*)esurf;

  LOGI("Renderer initialized: %ux%u connector=%u crtc=%u", width_, height_, conn_id_, crtc_id_);
  return true;
}

bool GbmKmsRenderer::render_test_pattern(int frames) {
  if (!egl_display_ || !egl_surface_ || !egl_context_) return false;

  EGLDisplay dpy = (EGLDisplay)egl_display_;
  EGLSurface surf = (EGLSurface)egl_surface_;

  for (int i = 0; i < frames; ++i) {
    float t = (float)i / (float)frames;
    glViewport(0, 0, (GLint)width_, (GLint)height_);
    glClearColor(t, 0.2f, 1.0f - t, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!eglSwapBuffers(dpy, surf)) {
      LOGE("eglSwapBuffers failed");
      return false;
    }
  }
  return true;
}

void GbmKmsRenderer::shutdown() {
  EGLDisplay dpy = (EGLDisplay)egl_display_;
  EGLContext ctx = (EGLContext)egl_context_;
  EGLSurface surf = (EGLSurface)egl_surface_;

  if (dpy && dpy != EGL_NO_DISPLAY) {
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (surf && surf != EGL_NO_SURFACE) eglDestroySurface(dpy, surf);
    if (ctx && ctx != EGL_NO_CONTEXT) eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);
  }

  egl_display_ = egl_context_ = egl_surface_ = nullptr;

  if (gbm_surf_) {
    gbm_surface_destroy((gbm_surface*)gbm_surf_);
    gbm_surf_ = nullptr;
  }
  if (gbm_dev_) {
    gbm_device_destroy((gbm_device*)gbm_dev_);
    gbm_dev_ = nullptr;
  }
  if (drm_fd_ >= 0) {
    ::close(drm_fd_);
    drm_fd_ = -1;
  }
}
