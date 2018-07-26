/*========================================================================================================
  Basically, all of files downloaded from my website can be modified or redistributed for any purpose.
  It is my honor to share my interesting to everybody.
  If you find any illeage content out from my website, please contact me firstly.
  I will remove all of the illeage parts.
  Thanks :)

  Steward Fu
  steward.fu@gmail.com
  https://steward-fu.github.io/website/index.htm
========================================================================================================*/
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/jzsoc.h>

#include "efb.h"
#include "ioctl.h"

MODULE_AUTHOR("Steward");
MODULE_DESCRIPTION("Enhanced framebuffer driver for RS97 handheld");
MODULE_LICENSE("GPL");

struct _mythread_t
{
  int id;
  int exit;
};

struct _myefb_t
{
  unsigned int *da0;
  dma_addr_t phys;
  uint16_t *virt;
};

static int major = -1;
static struct cdev mycdev;
static struct class *myclass = NULL;
static struct _mythread_t mythread;
static struct _myefb_t myefb;

static void ram_adjust_latency(void)
{
  REG_DDRC_TIMING1 = 0x10001000; //0x31114111
  REG_DDRC_TIMING2 = 0x4400; //0x8f11
  REG_DDRC_DQS_ADJ = 0x2121; //0x2321
  printk("%s, adjust ram latency\n", __func__);
}

static int efb_thread(void *arg)
{
  printk("%s, ++\n", __func__);
  myefb.da0 = ioremap_nocache(REG_LCD_DA0, sizeof(unsigned int) * 8);
  ram_adjust_latency();

  // triple buffer size
  myefb.virt = dma_alloc_coherent(NULL, DMA_SIZE * 3, (resource_size_t*)&myefb.phys, GFP_KERNEL | GFP_DMA);
  memset(myefb.virt, 0, DMA_SIZE * 3);
  printk("%s, DMA phys:0x%x, virt:0x%x\n", __func__, (unsigned int)myefb.phys, (unsigned int)myefb.virt);

  myefb.da0[1] = myefb.phys + (DMA_SIZE * 2);
  while(mythread.exit == 0)
  {
    msleep(10);
  }
  iounmap(myefb.da0);
  dma_free_coherent(NULL, DMA_SIZE * 3, myefb.virt, myefb.phys);
  printk("%s, --\n", __func__);
  return 0;
}

static int efb_open_close(struct inode *inode, struct file *file)
{
  printk("%s\n", __func__);
  return 0;
}

static long efb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  unsigned long ptr;

  //printk("%s, cmd: 0x%x\n", __func__, cmd);
  switch(cmd)
  {
    case IOCTL_GET_DMA_PHYS:
      ptr = myefb.phys;
      copy_to_user((void*)arg,  &ptr, sizeof(unsigned long));
      break;
    case IOCTL_SET_DMA_BUF:
      myefb.da0[1] = myefb.phys + (DMA_SIZE * arg);
      break;
    default:
      printk("%s, unsupported ioctl: 0x%x\n", __func__, cmd);
      return -1;
  }
  return 0;
}

static const struct file_operations fops =
{
  .owner = THIS_MODULE,
  .open = efb_open_close,
  .release = efb_open_close,
  .unlocked_ioctl = efb_ioctl,
};

static void efb_clean_node(void)
{
  if(myclass)
  {
    device_destroy(myclass, major);
    cdev_del(&mycdev);
    class_destroy(myclass);
  }
  if(major != -1)
  {
    unregister_chrdev_region(major, 1);
  }
}

static int __init efb_create_node(void)
{
  // /proc/devices
  if(alloc_chrdev_region(&major, 0, 1, DRIVER_NAME) < 0)
  {
    goto error;
  }
  // /sys/class
  if((myclass = class_create(THIS_MODULE, DRIVER_NAME)) == NULL)
  {
    goto error;
  }
  // /dev/
  if(device_create(myclass, NULL, major, NULL, DRIVER_NAME) == NULL)
  {
    goto error;
  }
  cdev_init(&mycdev, &fops);
  if(cdev_add(&mycdev, major, 1) == -1)
  {
    goto error;
  }
  printk("%s, create device node successfully\n", __func__);
  return 0;

error:
  efb_clean_node();
  printk("%s, failed to create device node\n", __func__);
  return -1;
}

static int __init efb_init(void)
{
  printk("%s, %s\n", __func__, __DATE__);
  efb_create_node();
  memset(&mythread, 0, sizeof(mythread));
  mythread.id = kernel_thread(efb_thread, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
  if(mythread.id <= 0)
  {
    printk("%s, failed to create mythread\n", __func__);
  }
  else
  {
    printk("%s, create mythread successfully\n", __func__);
  }
  return 0;
}

static void __exit efb_exit(void)
{
  printk("%s, %s\n", __func__, __DATE__);
  if(mythread.id > 0)
  {
    mythread.exit = 1;
    msleep(300);
    //kill_pid(find_vpid(mythread.id), SIGKILL, 1);
  }
  efb_clean_node();
}

module_init(efb_init);
module_exit(efb_exit);

