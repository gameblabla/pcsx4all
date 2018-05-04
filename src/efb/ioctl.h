#ifndef __IOCTL_H__
#define __IOCTL_H__

#define IOCTL_BASE						0x100
#define	IOCTL_GET_DMA_PHYS  	_IOW (IOCTL_BASE, 1, unsigned long)
#define IOCTL_SET_DMA_BUF			_IOW (IOCTL_BASE, 2, unsigned long)

#endif

