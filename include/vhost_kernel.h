//FIXME: which lisence for this file?
#ifndef _VHOST_BACKEND_H_
#define _VHOST_BACKEND_H_
#include "vbs_common_if.h"		/* data format between VBS-U & VBS-K */
#include "vhost.h"

int vhost_kernel_init(struct vhost_dev *vdev, struct virtio_base *base, int fd);

int vhost_kernel_deinit(int fd);

int vhost_kernel_set_owner(struct vhost_dev *vdev);

int vhost_kernel_start(struct vhost_dev *vdev, struct virtio_base *base);

int vhost_kernel_ioctl(struct vhost_dev* vdev, unsigned long int request, ...);

#endif
