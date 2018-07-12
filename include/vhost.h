//FIXME: which lisence for this file?
#ifndef __VHOST_H__
#define __VHOST_H__

#include <linux/ioctl.h>
#include "types.h"

struct vhost_vring_state {
	uint32_t index;
	uint32_t num;
};

struct vhost_vring_file {
	uint32_t index;
	int32_t fd; /* Pass -1 to unbind from file. */
};

struct vhost_vring_addr {
	uint32_t index;
	uint32_t flags;
#define VHOST_VRING_F_LOG 0
	unsigned long long desc_user_addr;
	unsigned long long used_user_addr;
	unsigned long long avail_user_addr;
	unsigned long long log_guest_addr;
};

struct vhost_memory_region {
	uint64_t guest_phys_addr;
	uint64_t memory_size; /* bytes */
	uint64_t userspace_addr;
	uint64_t flags_padding; /* No flags are currently specified. */
};

struct vhost_memory {
	uint32_t nregions;
	uint32_t padding;
	struct vhost_memory_region regions[0];
};

struct vhost_dev {
	struct virtio_base *base;
	int32_t vhostfd;
	int32_t backend_fd;
	int32_t num_of_queues;
	struct vhost_memory vhost_mem;
	uint64_t	features;
};

/* ioctls */
#define VHOST_VIRTIO 0xAF

#define VHOST_GET_FEATURES  _IOR(VHOST_VIRTIO, 0x00, uint64_t)
#define VHOST_SET_FEATURES  _IOW(VHOST_VIRTIO, 0x00, uint64_t)

#define VHOST_SET_OWNER _IO(VHOST_VIRTIO, 0x01)
#define VHOST_RESET_OWNER _IO(VHOST_VIRTIO, 0x02)

#define VHOST_SET_MEM_TABLE _IOW(VHOST_VIRTIO, 0x03, struct vhost_memory)
#define VHOST_SET_LOG_BASE _IOW(VHOST_VIRTIO, 0x04, uint64_t)
#define VHOST_SET_LOG_FD _IOW(VHOST_VIRTIO, 0x07, int32_t)

#define VHOST_SET_VRING_NUM _IOW(VHOST_VIRTIO, 0x10, struct vhost_vring_state)

#define VHOST_SET_VRING_ADDR _IOW(VHOST_VIRTIO, 0x11, struct vhost_vring_addr)

#define VHOST_SET_VRING_BASE _IOW(VHOST_VIRTIO, 0x12, struct vhost_vring_state)
#define VHOST_GET_VRING_BASE _IOWR(VHOST_VIRTIO, 0x12, struct vhost_vring_state)

#define VHOST_VRING_LITTLE_ENDIAN 0
#define VHOST_VRING_BIG_ENDIAN 1
#define VHOST_SET_VRING_ENDIAN _IOW(VHOST_VIRTIO, 0x13, struct vhost_vring_state)
#define VHOST_GET_VRING_ENDIAN _IOW(VHOST_VIRTIO, 0x14, struct vhost_vring_state)

#define VHOST_SET_VRING_KICK _IOW(VHOST_VIRTIO, 0x20, struct vhost_vring_file)

#define VHOST_SET_VRING_CALL _IOW(VHOST_VIRTIO, 0x21, struct vhost_vring_file)

#define VHOST_SET_VRING_ERR _IOW(VHOST_VIRTIO, 0x22, struct vhost_vring_file)

#define VHOST_SET_VRING_BUSYLOOP_TIMEOUT _IOW(VHOST_VIRTIO, 0x23, struct vhost_vring_state)
#define VHOST_GET_VRING_BUSYLOOP_TIMEOUT _IOW(VHOST_VIRTIO, 0x24, struct vhost_vring_state)

#define VHOST_NET_SET_BACKEND _IOW(VHOST_VIRTIO, 0x30, struct vhost_vring_file)

int vhost_dev_init(struct vhost_dev *vdev, struct virtio_base *base, int fd);
int vhost_dev_start(struct vhost_dev *vdev, struct virtio_base *base);
int vhost_dev_stop(struct vhost_dev *vdev, struct virtio_base *base);

#endif
