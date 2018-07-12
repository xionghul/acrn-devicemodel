//FIXME: which lisence for this file?

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "dm.h"
#include "pci_core.h"
#include "vmmapi.h"
#include "virtio.h"
#include "vhost.h"
#include "vhost_kernel.h"

#define WPRINTF(params) (printf params)

#define VIRTIO_NET_F_MRG_RXBUF  15
#define VHOST_NET_F_VIRTIO_NET_HDR 27

int vhost_kernel_init(struct vhost_dev *vdev, struct virtio_base *base, int fd)
{
	vdev->base = base;
	vdev->vhostfd = fd;

	return 0;
}

int vhost_kernel_deinit(int fd)
{
	//close resources here.
	return -1;
}

int vhost_kernel_call(int fd, unsigned long int request,
                             void *arg)
{
	WPRINTF(("fd:0x%x, request: 0x%lx !\n\r", fd, request));
	return ioctl(fd, request, arg);
}

int vhost_kernel_ioctl(struct vhost_dev* vdev, unsigned long int request, ...)
{
    void *arg;
    va_list ap;

    va_start(ap, request);
    arg = va_arg(ap, void *);

    switch (request) {
    case VHOST_SET_OWNER:
    case VHOST_RESET_OWNER:
		return vhost_kernel_call(vdev->vhostfd, request, NULL);
        break;
    case VHOST_GET_FEATURES:
    case VHOST_SET_FEATURES:
    case VHOST_SET_MEM_TABLE:
    case VHOST_SET_VRING_NUM:
    case VHOST_SET_VRING_BASE:
    case VHOST_SET_VRING_KICK:
    case VHOST_SET_VRING_CALL:
    case VHOST_SET_VRING_ADDR:
    case VHOST_NET_SET_BACKEND:
	case VHOST_GET_VRING_BASE:
		vhost_kernel_call(vdev->vhostfd, request, arg);
        break;

    default:
		WPRINTF(("request %lx not handled!\n\r", request));
        return -1;
    }

    va_end(ap);

    return 0;
}

int vhost_kernel_set_owner(struct vhost_dev *vdev)
{
    return vhost_kernel_ioctl(vdev, VHOST_SET_OWNER, NULL);
}

int vhost_set_vring_call(struct vhost_dev *vdev, struct vhost_vring_file *file)
{
    return vhost_kernel_ioctl(vdev, VHOST_SET_VRING_CALL, file);
}

int set_vring_state(struct vhost_dev *vdev, struct virtio_vq_info *virtq, int index)
{
    virtq->kick = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    virtq->call = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    assert(virtq->kick >= 0);
    assert(virtq->call >= 0);

    struct vhost_vring_state num = { .index = index, .num = virtq->qsize };
    struct vhost_vring_state base = { .index = index, .num = 0 };
    struct vhost_vring_file kick = { .index = index, .fd = virtq->kick };
    struct vhost_vring_file call = { .index = index, .fd = virtq->call };
    struct vhost_vring_addr addr = { .index = index,
            .desc_user_addr = (uintptr_t) virtq->desc,
            .avail_user_addr = (uintptr_t) virtq->avail,
            .used_user_addr = (uintptr_t) virtq->used,
            .log_guest_addr = (uintptr_t) NULL,
            .flags = virtq->flags };

    if (vhost_kernel_ioctl(vdev, VHOST_SET_VRING_NUM, &num) != 0)
        return -1;
    if (vhost_kernel_ioctl(vdev, VHOST_SET_VRING_BASE, &base) != 0)
        return -1;
    if (vhost_kernel_ioctl(vdev, VHOST_SET_VRING_KICK, &kick) != 0)
        return -1;
    if (vhost_kernel_ioctl(vdev, VHOST_SET_VRING_CALL, &call) != 0)
        return -1;
    if (vhost_kernel_ioctl(vdev, VHOST_SET_VRING_ADDR, &addr) != 0)
        return -1;

    return 0;
}

int vhost_init_mem_table(struct vhost_dev *vdev, struct vhost_memory *mem, struct virtio_base *base)
{
	mem->nregions = 1;
	mem->padding = 0;
	mem->regions[0].guest_phys_addr = (uintptr_t) 0;
	mem->regions[0].memory_size = 0x800000000; /* FIXME: should use real address range for vhost to do check. */
	mem->regions[0].userspace_addr = (uint64_t)base->dev->vmctx->baseaddr;
	printf("mem_table: desc_phys:%lx\n\r", mem->regions[0].guest_phys_addr);
	printf("mem_table: uaddr:%lx\n\r", mem->regions[0].userspace_addr);
	return 0;
}

int vhost_kernel_start(struct vhost_dev *vdev, struct virtio_base *base)
{
	struct virtio_vq_info *vq;

	/* set vhost features. */
	uint64_t features = 0;
	features = (1ULL << VHOST_NET_F_VIRTIO_NET_HDR) | VIRTIO_F_VERSION_1 | (1ULL << VIRTIO_NET_F_MRG_RXBUF);
	int ret = vhost_kernel_ioctl(vdev, VHOST_SET_FEATURES, &features);
	WPRINTF(("set features:%lx, ret:%d\n\r", features, ret));

	/*set memory table.*/
	vhost_init_mem_table(vdev, &vdev->vhost_mem, base);
	vhost_kernel_ioctl(vdev, VHOST_SET_MEM_TABLE, &vdev->vhost_mem);

	int i;
	for (i = 0; i < vdev->num_of_queues; i++ ) {
		vq = &base->queues[i];
		WPRINTF(("vdev:%p, pfn:%x, desc:%p, avail:%p, used:%p\n\r", vdev, vq->pfn, vq->desc, vq->avail, vq->used ));
		set_vring_state(vdev, vq, i);
	}

	/*set backend, index need use MACRO like RX and TX. */
	struct vhost_vring_file file;
	file.index = 0;
	file.fd = vdev->backend_fd;
	WPRINTF(("backend:fd:%d\n\r", vdev->backend_fd));
	vhost_kernel_ioctl(vdev, VHOST_NET_SET_BACKEND, &file);
	file.index = 1;
	file.fd = vdev->backend_fd;
	vhost_kernel_ioctl(vdev, VHOST_NET_SET_BACKEND, &file);

	return 0;
}

/* vhost device can trigger vhost user or vhost kernel here. */
int vhost_dev_init(struct vhost_dev *vdev, struct virtio_base *base, int fd)
{
	int ret = vhost_kernel_init(vdev, base, fd);

	if(ret < 0) {
		WPRINTF(("vdev: vhost kernel init failed\n"));
		return ret;
	}
	ret = vhost_kernel_set_owner(vdev);

	return ret;
}

int vhost_dev_start(struct vhost_dev *vdev, struct virtio_base *base)
{
	return vhost_kernel_start(vdev, base);
}

int vhost_dev_stop(struct vhost_dev *vdev, struct virtio_base *base)
{

	return 0;
}
