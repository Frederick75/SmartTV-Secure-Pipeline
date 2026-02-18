# smarttv-secure-pipeline (OP-TEE + DMA-HEAP + RDMA-style + GBM/DRM/KMS) — scaffold

This repository is a **compilable integration scaffold** for a Smart TV secure playback pipeline:
- Kernel 5.10: Secure Video Path allocator using **DMA-HEAP** (secure heap) exporting **dma-buf fds**
- Kernel 5.10: **RDMA-style** copy using **DMAengine** between dma-buf buffers
- OP-TEE: Trusted Application (TA) + host client stub for key/session plumbing (no DRM internals)
- Userspace: GBM + DRM/KMS + EGL/OpenGL ES renderer scaffold

**Important:** Proprietary DRM components (Netflix NRDP, PlayReady CDM/TA) are **not** implemented here.
You bind those through the adapter interfaces under `user/drm/` and `user/nrdp/`.

## Build (userspace)
Prereqs (Ubuntu/Debian example):
- libdrm-dev, libgbm-dev, libegl1-mesa-dev, libgles2-mesa-dev, pkg-config, cmake

```bash
cmake -S user -B build-user -DCMAKE_BUILD_TYPE=Release
cmake --build build-user -j
```

Run on target (needs DRM/KMS access, typically as root):
```bash
./build-user/demo_player --card /dev/dri/card0 --heap secure --width 1920 --height 1080
```

## Build (kernel modules)
You need the target kernel headers/build tree (KDIR):
```bash
make -C <KDIR> M=$PWD/kernel modules
```

Install (on target):
```bash
insmod svp.ko svp_heap_name=secure
insmod rdma_stub.ko
```

Devices:
- /dev/svp0
- /dev/rdma_stub0

## Notes
- The secure property is enforced by the **DMA-HEAP secure heap** and platform IOMMU/TZ/Display rules.
- For true secure DMA copies you usually need a vendor secure DMA channel or secure domain mapping.
  Hook points are indicated in the RDMA driver.
