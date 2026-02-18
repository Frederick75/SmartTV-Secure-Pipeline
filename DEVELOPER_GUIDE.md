
# Smart TV Secure Pipeline — Developer Guide

## Target Platform
- SoC: MediaTek MT5895 / MT9615
- Kernel: Linux 5.10
- TEE: OP‑TEE
- Display: DRM/KMS + GBM + EGL + GLES2
- Memory: DMA‑HEAP secure heap
- Transfer: DMAengine RDMA‑style copy

---

## Responsibilities by Layer

### Kernel Team
Implements:

- SVP allocator using DMA‑HEAP secure heap
- RDMA DMAengine transfers
- DRM/KMS protected plane integration
- Secure buffer lifecycle enforcement

Key files:

```
kernel/secure_video/svp_drv.c
kernel/secure_video/svp_dmabuf_dmaheap.c
kernel/rdma_stub/rdma_stub.c
```

---

### TEE Team

Implements Trusted Application:

Responsibilities:

- Session creation
- Key blob storage
- Secure playback authorization

Interface:

```
TEE Client API
/dev/tee0
```

Files:

```
tee/ta/ta_svp.c
tee/host/tee_svp_client.c
```

---

### Userspace / Middleware Team

Responsibilities:

- NRDP integration
- PlayReady CDM binding
- Video pipeline orchestration
- EGL rendering

Files:

```
user/player/pipeline.cpp
user/renderer/egl_gl_renderer.cpp
```

---

## Secure Buffer Lifecycle

1. Userspace requests secure buffer
2. Kernel allocates via DMA‑HEAP secure heap
3. Video decoder writes encrypted content
4. TEE authorizes session
5. RDMA engine transfers buffers
6. GPU renders securely
7. DRM/KMS displays frame

---

## Kernel Integration Steps

Enable kernel config:

```
CONFIG_OPTEE=y
CONFIG_TEE=y
CONFIG_DMA_HEAP=y
CONFIG_DMA_ENGINE=y
CONFIG_DMABUF_HEAPS=y
CONFIG_DRM=y
```

Build:

```
make modules
```

Load:

```
insmod svp.ko
insmod rdma_stub.ko
```

---

## Debugging

Check heap:

```
ls /dev/dma_heap/
```

Check DRM:

```
ls /dev/dri/
```

Check TEE:

```
ls /dev/tee*
```

---

## Performance Optimization

Recommended:

- Use dedicated secure heap
- Enable IOMMU secure domains
- Use hardware DMA engines
- Avoid CPU buffer mapping

