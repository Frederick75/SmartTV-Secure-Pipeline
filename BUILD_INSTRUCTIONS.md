
# Build Instructions — Smart TV Secure Pipeline (Kernel 5.10 + OP-TEE + DMA-HEAP + GBM/DRM-KMS)

This document describes how to build and run the **smarttv-secure-pipeline** scaffold on a Linux-based Smart TV platform.

---

## 1) Prerequisites

### Target device requirements
- Linux **5.10** kernel (or vendor fork based on 5.10)
- **OP-TEE** enabled (`/dev/tee0` and `tee-supplicant` available)
- **DRM/KMS** enabled (`/dev/dri/card0` present)
- **DMA-HEAP** enabled with at least one secure heap (name varies by vendor)

Verify on target:

```bash
ls -l /dev/dri/
ls -l /dev/tee*
ls -l /dev/dma_heap/
```

Common heap names:
- `secure`
- `svp`
- `mtk_svp`
- `system`

---

## 2) Kernel Configuration (5.10)

Ensure these are enabled in your kernel config/defconfig:

```text
CONFIG_TEE=y
CONFIG_OPTEE=y

CONFIG_DMA_ENGINE=y
CONFIG_DMA_HEAP=y
CONFIG_DMABUF_HEAPS=y
CONFIG_SYNC_FILE=y

CONFIG_DRM=y
CONFIG_DRM_KMS_HELPER=y
CONFIG_DRM_GEM_DMA_HELPER=y

# Platform-specific:
# - DMA controller driver (for DMAengine memcpy)
# - MediaTek DRM driver and display pipeline
```

> Tip: If `rdma_stub` reports “no DMA_MEMCPY channel”, you likely need to enable your SoC DMA engine driver.

---

## 3) Build Kernel Modules

### 3.1 Set kernel build directory

On your build host, set `KDIR` to the path of the kernel build output directory (where `.config` and `Module.symvers` exist).

Example:

```bash
export KDIR=/path/to/kernel/out
```

### 3.2 Build modules

From the project root:

```bash
make -C "$KDIR" M="$PWD/kernel" modules
```

Expected outputs:
- `kernel/secure_video/svp.ko`
- `kernel/rdma_stub/rdma_stub.ko`

---

## 4) Load Kernel Modules on Target

Copy `.ko` files to the target, then:

```bash
sudo insmod svp.ko svp_heap_name=secure
sudo insmod rdma_stub.ko
```

If your secure heap name differs, set it accordingly:

```bash
sudo insmod svp.ko svp_heap_name=mtk_svp
```

Verify devices:

```bash
ls -l /dev/svp0
ls -l /dev/rdma_stub0
```

Check logs:

```bash
dmesg | tail -n 100
```

---

## 5) Build Userspace (GBM + DRM/KMS + EGL/GLES2)

### 5.1 Build dependencies (build host)

For Debian/Ubuntu-like systems:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config   libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev
```

> On Yocto/embedded builds, use the corresponding SDK sysroot packages (libdrm, gbm, egl, gles2).

### 5.2 Configure and build

From project root:

```bash
cmake -S user -B build-user -DCMAKE_BUILD_TYPE=Release
cmake --build build-user -j
```

Expected binary:
- `build-user/demo_player`

---

## 6) Run Demo on Target

### 6.1 Basic render loop (KMS card0)

```bash
sudo ./build-user/demo_player --card /dev/dri/card0 --frames 120
```

### 6.2 Run with RDMA-style copy enabled

```bash
sudo ./build-user/demo_player --card /dev/dri/card0 --frames 120 --rdma
```

Common options (example):
- `--card /dev/dri/card0` : DRM device
- `--frames 120` : number of frames to present
- `--rdma` : enable DMAengine-based copy between dmabufs

---

## 7) Troubleshooting

### 7.1 DMA-HEAP secure heap not found
Error: `svp: dma_heap 'secure' not found`

Fix:
- Check available heaps:

```bash
ls /dev/dma_heap/
ls /sys/kernel/dma_heap/
```

- Reload `svp.ko` with correct heap name:

```bash
sudo rmmod svp
sudo insmod svp.ko svp_heap_name=mtk_svp
```

### 7.2 No DMAengine memcpy channel
Log: `rdma_stub: no DMA_MEMCPY channel`

Fix:
- Enable SoC DMA controller driver in kernel config
- Confirm DMAengine is active:
```bash
dmesg | grep -i dma
```

### 7.3 DRM card missing
Verify DRM nodes:
```bash
ls -l /dev/dri/
```

Ensure MediaTek DRM/display drivers are loaded.

### 7.4 OP-TEE not accessible
Verify:
```bash
ls -l /dev/tee*
ps | grep tee-supplicant
```

---

## 8) Production Notes (Firmware Teams)

- For **true protected content**, the secure heap must enforce:
  - no CPU mapping
  - protected IOMMU domain / TZASC rules
  - secure display planes/layers
- RDMA secure copies may require:
  - secure-capable DMA channels
  - vendor-specific DMA/IOMMU mapping hooks
- Bindings for **Netflix NRDP** and **PlayReady CDM/TA** must be done via licensed SDKs.

---

## 9) Clean / Rebuild

Userspace rebuild:

```bash
rm -rf build-user
cmake -S user -B build-user -DCMAKE_BUILD_TYPE=Release
cmake --build build-user -j
```

Kernel modules clean:

```bash
make -C "$KDIR" M="$PWD/kernel" clean
```

