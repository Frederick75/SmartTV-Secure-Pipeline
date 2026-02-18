cmake_minimum_required(VERSION 3.16)
project(smarttv_secure_pipeline LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(GBM REQUIRED gbm)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES2 REQUIRED glesv2)

add_subdirectory(../tee/host tee_host_build)

add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(renderer
  renderer/gbm_kms_renderer.cpp
  renderer/gbm_kms_renderer.h
  common/log.h
  common/fd.h
)
target_include_directories(renderer PRIVATE ${DRM_INCLUDE_DIRS} ${GBM_INCLUDE_DIRS} ${EGL_INCLUDE_DIRS} ${GLES2_INCLUDE_DIRS})
target_link_libraries(renderer PRIVATE ${DRM_LIBRARIES} ${GBM_LIBRARIES} ${EGL_LIBRARIES} ${GLES2_LIBRARIES})
target_compile_options(renderer PRIVATE ${DRM_CFLAGS_OTHER} ${GBM_CFLAGS_OTHER} ${EGL_CFLAGS_OTHER} ${GLES2_CFLAGS_OTHER})

add_library(drm_adapters
  drm/cdm_adapter_stub.cpp
  drm/cdm_adapter.h
  nrdp/nrdp_adapter_stub.cpp
  nrdp/nrdp_adapter.h
  common/log.h
)
target_include_directories(drm_adapters PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(drm_adapters PRIVATE -Wall -Wextra)

add_library(pipeline
  player/pipeline.cpp
  player/pipeline.h
  player/rdma_client.cpp
  player/rdma_client.h
  common/log.h
  common/fd.h
)
target_include_directories(pipeline PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../tee/host ../kernel/secure_video ../kernel/rdma_stub)
target_link_libraries(pipeline PRIVATE renderer drm_adapters tee_svp_client)
target_compile_options(pipeline PRIVATE -Wall -Wextra)

add_executable(demo_player apps/demo_player.cpp)
target_link_libraries(demo_player PRIVATE pipeline)
target_compile_options(demo_player PRIVATE -Wall -Wextra)
