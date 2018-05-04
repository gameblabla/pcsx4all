#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "efb.h"
#include "ioctl.h"

int fd;
void* map_it(unsigned long addr, unsigned long size)
{
  fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(fd < 0){
    printf("failed to open /dev/mem\n");
    return NULL;
  }
  return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr);
}

void unmap_it(void* addr, unsigned long size)
{
	munmap(addr, size);
	close(fd);
}

int main(int argc, char** argv)
{
	int fd;

	printf("%s, %s\n", __func__, __DATE__);
	fd = open(DRIVER_NAME, O_RDWR);
	if(fd < 0){
		printf("failed to open efb driveri: %s\n", DRIVER_NAME);
		return -1;
	}
	
	// get dma phys
	unsigned long col=0xaaaa, dma_phys;
	ioctl(fd, IOCTL_GET_DMA_PHYS, &dma_phys);
	uint16_t *ptr;
	uint8_t *dma_virt = map_it(dma_phys, DMA_SIZE * 3);
	printf("DMA phys: 0x%x\n", dma_phys);
	printf("DMA virt: 0x%x\n", dma_virt);
	
	for(int cnt=0; cnt<100; cnt++){
		for(int index=0; index<3; index++){
			ptr = dma_virt + (DMA_SIZE * index);
			for(int y=0; y<LCD_H; y++){
				for(int x=0; x<LCD_W; x++){
					*ptr++ = col;
				}
			}
			col^= 0xffff;
			ioctl(fd, IOCTL_SET_DMA_BUF, index);
			usleep(50000);
		}
	}
	unmap_it(dma_virt, DMA_SIZE * 3);
	close(fd);
	return 0;
}

