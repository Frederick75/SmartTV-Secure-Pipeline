
# Smart TV Secure Video Pipeline (OP-TEE + DMA-HEAP + RDMA + GBM/KMS)

## Overview

This project provides a **production-grade scaffold** for implementing a secure video playback pipeline on Linux-based Smart TV platforms such as **MediaTek MT5895 / MT9615**, using:

- Linux Kernel 5.10
- OP-TEE Trusted Execution Environment
- DMA-HEAP secure buffer allocation
- RDMA-style DMAengine memory transfers
- DRM/KMS display subsystem
- GBM (Generic Buffer Manager)
- EGL + OpenGL ES rendering pipeline

This architecture matches modern Smart TV firmware requirements for secure playback pipelines used in premium OTT environments.

---

## Architecture

```
User Space
 ├── demo_player (GBM + DRM/KMS + EGL renderer)
 ├── tee_svp_client (OP-TEE interface)
 ├── CDM adapter stub
 └── NRDP adapter stub

Kernel Space
 ├── svp.ko (Secure Video Path allocator)
 │    └── Allocates buffers from DMA-HEAP secure heap
 │
 ├── rdma_stub.ko (RDMA-style DMA copy engine)
 │    └── Uses DMAengine for secure memory transfers
 │
 └── DRM/KMS subsystem
      └── Direct display pipeline

Secure World
 ├── OP-TEE Trusted Application
 │    ├── Key session management
 │    └── Secure blob handling
```

---

## Key Features

### Secure Buffer Allocation
Uses DMA-HEAP secure heap:

```
/dev/dma_heap/secure
```

Buffers returned as dma-buf file descriptors.

Properties:

- Not CPU accessible (platform enforced)
- Protected by IOMMU / TrustZone
- Safe for DRM playback pipelines

---

### RDMA-style Memory Transfer

Uses Linux DMAengine for hardware-controlled transfers:

```
Source dmabuf → DMA controller → Destination dmabuf
```

Advantages:

- CPU bypass
- Low latency
- Secure memory domain transfer capability

---

### OP-TEE Integration

Trusted Application handles:

- Session management
- Key blob storage
- Secure communication interface

Communicates via:

```
/dev/tee0
```

---

### Display Pipeline

Rendering uses:

```
dmabuf → GBM → EGLImage → OpenGL ES → DRM/KMS → Display
```

Components:

- DRM/KMS
- GBM
- EGL
- GLES2

---

## Repository Structure

```
smarttv-secure-pipeline/
│
├── kernel/
│   ├── secure_video/
│   │   ├── svp_drv.c
│   │   ├── svp_dmabuf_dmaheap.c
│   │   └── svp_uapi.h
│   │
│   └── rdma_stub/
│       ├── rdma_stub.c
│       └── rdma_stub_uapi.h
│
├── tee/
│   ├── ta/
│   │   └── ta_svp.c
│   │
│   └── host/
│       ├── tee_svp_client.c
│       └── tee_svp_client.h
│
├── user/
│   ├── player/
│   │   ├── demo_player.cpp
│   │   └── pipeline.cpp
│   │
│   └── renderer/
│       └── egl_gl_renderer.cpp
│
└── README.md
```

---

## Build Instructions

### Kernel Modules

```
export KDIR=/path/to/kernel/build

make -C $KDIR M=$PWD/kernel modules
```

Load modules:

```
sudo insmod svp.ko svp_heap_name=secure
sudo insmod rdma_stub.ko
```

Verify:

```
ls /dev/svp0
ls /dev/rdma_stub0
```

---

### Userspace

```
cmake -S user -B build-user
cmake --build build-user
```

Run demo:

```
sudo ./build-user/demo_player --card /dev/dri/card0 --frames 120
```

---

## Device Requirements

Kernel config must include:

```
CONFIG_DMA_HEAP=y
CONFIG_DMABUF_HEAPS=y
CONFIG_DMA_ENGINE=y
CONFIG_DRM=y
CONFIG_DRM_KMS_HELPER=y
CONFIG_OPTEE=y
CONFIG_TEE=y
```

Verify DMA heaps:

```
ls /dev/dma_heap/
```

Expected:

```
secure
system
```

---

## Secure Video Flow

```
License blob
   ↓
OP-TEE TA
   ↓
Secure buffer allocated
   ↓
Video decoder writes frame
   ↓
RDMA transfers frame
   ↓
GPU renders via EGL
   ↓
DRM/KMS displays frame
```

---

## Security Model

Protects against:

- CPU memory snooping
- Kernel buffer access
- Unauthorized readback
- Debugger inspection

Relies on:

- TrustZone
- Secure DMA heap
- Secure display pipeline

---

## Integration Points for Production

Replace stubs with:

- PlayReady CDM integration
- Netflix NRDP integration
- MediaTek secure heap allocator
- Secure DMA channel selection

---

## Tested With

- Linux Kernel 5.10
- OP-TEE 3.x
- Mesa EGL / GLES2
- GBM / DRM/KMS

---

## License

GPLv2 (Kernel components)
MIT (Userspace scaffold)

---

## Author

Smart TV Secure Pipeline Reference Implementation

