/*
 * Copyright (C) 2018 Steward Fu <steward.fu@gmail.com>
 *
 *  Enhanced framebuffer driver for RS97
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/console.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <asm/div64.h>

#define SDL_NUM               2
#define MAX_XRES              800
#define MAX_YRES              480
#define MAX_BPP               24
#define WAIT_FRAME_COUNT      10

#define LCD_XRES              320
#define LCD_YRES              240
#define LCD_BPP               16
#define LCD_RAM_PAGE          3
#define LCD_DMA_INDEX         0
#define LCD_SCR_INDEX         1
#define LCD_RAW_INDEX         2

#define DRIVER_NAME           "jz4760-rtc"
#define PALETTE_SIZE          256

#define EFB_FBIO_WAITFORVSYNC  _IOWR(0, 0, int)

struct myfb_par {
  struct device *dev;

  u32 pseudo_palette[PALETTE_SIZE];
  resource_size_t p_palette_base;
  unsigned char *v_palette_base;

  void *vram_virt;
  dma_addr_t vram_phys;
  unsigned long vram_size;

  unsigned int id;
  unsigned int bpp;
  struct fb_videomode mode;
  struct fb_var_screeninfo var;
  struct fb_fix_screeninfo fix;
};

struct _mylcd {
  struct myfb_par *fb[SDL_NUM];
  unsigned int cur_fb;

  void *dma_addr;
  unsigned int dma_xoffset;
  unsigned int dma_yoffset;

  unsigned long lram_size;
  void *lram_virt[LCD_RAM_PAGE];
  dma_addr_t lram_phys[LCD_RAM_PAGE];

  void __iomem *io_reg;
  struct resource *lcd_reg;

	int is_buffer_ready;
  unsigned int irq;
  unsigned int avg_filter;
  unsigned int vsync_flag;
  unsigned int vsync_timeout;
  wait_queue_head_t vsync_wait;

	int thread_id;
	int thread_exit;

	int extra_options;
	int is_double_buffer;
	int double_buffer_ready;
	int wait_frame_ready;
};

static struct _mylcd mylcd;

#define CNVT_TOHW(val, width) ((((val) << (width)) + 0x7FFF - (val)) >> 16)
static int myfb_setcolreg(unsigned no, unsigned r, unsigned g, unsigned b, unsigned t, struct fb_info *info)
{
  r = CNVT_TOHW(r, info->var.red.length);
  b = CNVT_TOHW(b, info->var.blue.length);
  g = CNVT_TOHW(g, info->var.green.length);
  ((u32*)(info->pseudo_palette))[no] = (r << info->var.red.offset) | (g << info->var.green.offset) | (b << info->var.blue.offset);
  return 0;
}
#undef CNVT_TOHW

static int myfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
  struct myfb_par *par = info->par;
  unsigned long bpp = var->bits_per_pixel/8;
  unsigned long line_size = var->xres_virtual * bpp;

  printk("%s(fb%d), xres:%d, yres:%d, bpp:%d, xres_virtual:%d, yres_virtual:%d\n", 
    __func__, par->id, var->xres, var->yres, var->bits_per_pixel, var->xres_virtual, var->yres_virtual);
  if((var->xres > MAX_XRES) || (var->yres > MAX_YRES) || (var->bits_per_pixel > MAX_BPP)){
    printk("%s, invalid parameter\n", __func__);
    return -EINVAL;
  }

  switch(var->bits_per_pixel){
  case 16:
    var->transp.offset = 0;
    var->transp.length = 0;
    var->red.offset = 11;
    var->red.length = 5;
    var->green.offset = 5;
    var->green.length = 6;
    var->blue.offset = 0;
    var->blue.length = 5;
    break;
  case 24:
    var->transp.offset = 0;
    var->transp.length = 0;
    var->red.offset = 16;
    var->red.length = 8;
    var->green.offset = 8;
    var->green.length = 8;
    var->blue.offset = 0;
    var->blue.length = 8;
    break;
  case 32:
    var->transp.offset = 24;
    var->transp.length = 8;
    var->red.offset = 16;
    var->red.length = 8;
    var->green.offset = 8;
    var->green.length = 8;
    var->blue.offset = 0;
    var->blue.length = 8;
    break;
  }
  var->red.msb_right = 0;
  var->green.msb_right = 0;
  var->blue.msb_right = 0;
  var->transp.msb_right = 0;

  if(line_size * var->yres_virtual > par->vram_size){
    var->yres_virtual = par->vram_size / line_size;
  }
  if(var->yres > var->yres_virtual){
    var->yres = var->yres_virtual;
  }
  if(var->xres > var->xres_virtual){
    var->xres = var->xres_virtual;
  }
  if(var->xres + var->xoffset > var->xres_virtual){
    var->xoffset = var->xres_virtual - var->xres;
  }
  if(var->yres + var->yoffset > var->yres_virtual){
    var->yoffset = var->yres_virtual - var->yres;
  }
  return 0;
}

static int myfb_remove(struct platform_device *dev)
{
  struct fb_info *info = dev_get_drvdata(&dev->dev);

	if(mylcd.thread_id){
		mylcd.thread_exit = 1;
		//kill_pid(find_vpid(mylcd.thread_id), SIGKILL, 1);
	}

  if(info){
    int x;
    struct myfb_par *par = info->par;

    unregister_framebuffer(info);
    fb_dealloc_cmap(&info->cmap);
    dma_free_coherent(NULL, par->vram_size, par->vram_virt, par->vram_phys);
    if(par->id == 1){
      for(x=0; x<LCD_RAM_PAGE; x++){
        dma_free_coherent(NULL, mylcd.lram_size, mylcd.lram_virt[x], mylcd.lram_phys[x]);
      }
    }
    dma_free_coherent(NULL, PALETTE_SIZE, par->v_palette_base, par->p_palette_base);
    framebuffer_release(info);
  }
  return 0;
}

static void lcd_pixel_process(unsigned int old_w, unsigned int old_h, unsigned int bpp)
{
  uint16_t r, g, b;
  unsigned int x, y, l, c, e;
	uint16_t *src = mylcd.lram_virt[LCD_RAW_INDEX];
  uint16_t *dst = mylcd.lram_virt[LCD_SCR_INDEX];

  //printk("%s, x:%d, y:%d, bpp:%d\n", __func__, old_w, old_h, bpp);
  for(y=mylcd.dma_yoffset; y<LCD_YRES; y++){
    for(x=mylcd.dma_xoffset; x<LCD_XRES; x++){
      l = ((y * 1000 * old_h) / LCD_YRES) / 1000;
      c = ((x * 1000 * old_w) / LCD_XRES) / 1000;
      if(bpp != 24){
        if(LCD_XRES == old_w){
          r = ((*((uint16_t*)src + (l * old_w) + c + 0) & 0xf800) >> 8);
          g = ((*((uint16_t*)src + (l * old_w) + c + 0) & 0x7e0) >> 3);
          b = ((*((uint16_t*)src + (l * old_w) + c + 0) & 0x1f) << 3);
        }
        else{
          e = 0;
          if(x != (LCD_XRES - 1)){
            e = 1;
          }
          r = ((*((uint16_t*)src + (l * old_w) + c + 0) & 0xf800) >> 8);
          g = ((*((uint16_t*)src + (l * old_w) + c + 0) & 0x7e0) >> 3);
          b = ((*((uint16_t*)src + (l * old_w) + c + 0) & 0x1f) << 3);
          
          if(mylcd.avg_filter){
            r+= ((*((uint16_t*)src + (l * old_w) + c + e) & 0xf800) >> 8);
            r>>= 1;
            g+= ((*((uint16_t*)src + (l * old_w) + c + e) & 0x7e0) >> 3);
            g>>= 1;
            b+= ((*((uint16_t*)src + (l * old_w) + c + e) & 0x1f) << 3);
            b>>= 1;
          }
        }
      }
      else{
        b = *((uint8_t*)src + (l * old_w * 3) + (c * 3) + 0);
        g = *((uint8_t*)src + (l * old_w * 3) + (c * 3) + 1);
        r = *((uint8_t*)src + (l * old_w * 3) + (c * 3) + 2);
      } 
      *((uint16_t*)dst + ((LCD_XRES - x) * LCD_YRES) + y) = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
    }
  }
	mylcd.is_buffer_ready = 1;
}

static int myfb_set_par(struct fb_info *info)
{
  struct myfb_par *par = info->par;
  
  printk("%s(fb%d), xres:%d, yres:%d, bpp:%d, xoffset:%d, yoffset:%d, xvirtual:%d, yvirtual:%d\n", __func__, 
    par->id, info->var.xres, info->var.yres, info->var.bits_per_pixel, info->var.xoffset, info->var.yoffset, 
		info->var.xres_virtual, info->var.yres_virtual);

  fb_var_to_videomode(&par->mode, &info->var);
  par->bpp = info->var.bits_per_pixel;
  info->fix.visual = FB_VISUAL_TRUECOLOR;
  info->fix.line_length = (par->mode.xres * par->bpp) / 8;

  mylcd.cur_fb = par->id;
	if(mylcd.lram_virt[LCD_DMA_INDEX]){
  	memset(mylcd.lram_virt[LCD_DMA_INDEX], 0, (LCD_XRES * LCD_YRES * LCD_BPP) / 8);
	}
  
  mylcd.dma_yoffset = 0;
  if(LCD_YRES > info->var.yres){
    mylcd.dma_yoffset = (LCD_YRES - info->var.yres) / 2;
  }

  mylcd.dma_xoffset = 0;
  if(LCD_XRES > info->var.xres){
    mylcd.dma_xoffset = (LCD_XRES - info->var.xres) / 2;
  }
  mylcd.dma_addr = par->vram_virt + (mylcd.dma_xoffset * info->fix.line_length) + ((mylcd.dma_xoffset * info->var.bits_per_pixel) / 8); 

  mylcd.avg_filter = 0;
  if(info->var.xres == 512){ // for PS1 game with x resolution = 512 pixels
    mylcd.avg_filter = 1;
  }
	mylcd.wait_frame_ready = WAIT_FRAME_COUNT;
	mylcd.is_double_buffer = (info->var.yres_virtual == info->var.yres) ? 0 : 1;
	printk("%s, double buffer: %d\n", __func__, mylcd.is_double_buffer);
  return 0;
}

static int fb_wait_for_vsync(struct fb_info *info)
{
  int ret;

  mylcd.vsync_flag = 0; 
  ret = wait_event_interruptible_timeout(mylcd.vsync_wait, mylcd.vsync_flag != 0, mylcd.vsync_timeout);
  if(ret < 0){
    return ret;
  } 
  return (ret == 0) ? -ETIMEDOUT : 0;
}

static int myfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
  int ret=0;
  struct myfb_par *par = info->par;

  printk("%s(fb%d)++\n", __func__, par->id);
  switch(cmd){
  case EFB_FBIO_WAITFORVSYNC:
    printk("%s, FBIO_WAITFORVSYNC\n", __func__);
    ret = fb_wait_for_vsync(info);
    break;
  default:
    ret = -EINVAL;
    printk("%s, unknown ioctl: 0x%x\n", __func__, cmd);
    break;
  }
  printk("%s(ret:%d)--\n", __func__, ret);
  return ret;
}

static int myfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	return 0;
}

static int myfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
  struct myfb_par *par = info->par;
  struct fb_var_screeninfo new_var;
  struct fb_fix_screeninfo *fix = &info->fix;

  printk("%s(fb%d), xres:%d, yres:%d, xoffset:%d, yoffset:%d\n", __func__, par->id, var->xres, var->yres, var->xoffset, var->yoffset);
  if((var->xoffset != info->var.xoffset) || (var->yoffset != info->var.yoffset)){
    memcpy(&new_var, &info->var, sizeof(new_var));
    new_var.xoffset = var->xoffset;
    new_var.yoffset = var->yoffset;
    memcpy(&info->var, &new_var, sizeof(new_var));  
    mylcd.dma_addr = par->vram_virt + (new_var.yoffset * fix->line_length) + ((new_var.xoffset * info->var.bits_per_pixel) / 8);

		memcpy(mylcd.lram_virt[LCD_RAW_INDEX], mylcd.dma_addr, (var->xres * var->yres * info->var.bits_per_pixel) / 8);
		mylcd.double_buffer_ready = 1;
  }
  return 0;
}

static struct fb_ops myfb_ops = {
  .owner          = THIS_MODULE,
  .fb_check_var   = myfb_check_var,
  .fb_set_par     = myfb_set_par,
  .fb_setcolreg   = myfb_setcolreg,
  .fb_pan_display = myfb_pan_display,
  .fb_ioctl       = myfb_ioctl,
  .fb_mmap        = myfb_mmap,

  .fb_fillrect    = cfb_fillrect,
  .fb_copyarea    = cfb_copyarea,
  .fb_imageblit   = cfb_imageblit,
};

static int lcd_thread(void* unused)
{
	struct myfb_par *par;
	while(mylcd.thread_exit == 0){
		msleep(10);
		par = mylcd.fb[mylcd.cur_fb];
		if(mylcd.is_double_buffer){
			if(mylcd.double_buffer_ready){
				mylcd.double_buffer_ready = 0;
				lcd_pixel_process(par->var.xres, par->var.yres, par->var.bits_per_pixel);
			}
		}
		else{
			if(mylcd.wait_frame_ready){
				mylcd.wait_frame_ready-= 1;
			}
			else{
				memcpy(mylcd.lram_virt[LCD_RAW_INDEX], mylcd.dma_addr, (par->var.xres * par->var.yres * par->var.bits_per_pixel) / 8);
				lcd_pixel_process(par->var.xres, par->var.yres, par->var.bits_per_pixel);
			}
		}
	}
	return 0;
}

static irqreturn_t lcd_interrupt_handler(int irq, void *arg)
{
  return IRQ_HANDLED;
}

static int myfb_probe(struct platform_device *device)
{
  int ret=-1, x=0;
  struct fb_info *info=NULL;
  struct myfb_par *par=NULL;

	printk("%s\n", __func__);
  for(x=0; x<SDL_NUM; x++){
    info = framebuffer_alloc(sizeof(struct myfb_par), &device->dev);
    if(!info){
      printk("%s, failed to allocate framebuffer\n", __func__);
      return -ENOMEM; // sorry, no error handling
    }
    par = info->par;
    par->dev = &device->dev;
    par->bpp = LCD_BPP;
    par->mode.name = DRIVER_NAME;
    par->mode.xres = LCD_XRES;
    par->mode.yres = LCD_YRES;
    par->mode.vmode = FB_VMODE_NONINTERLACED;

    par->var.grayscale = 0;
    par->var.bits_per_pixel = par->bpp;
    par->var.activate = FB_ACTIVATE_FORCE; 
    fb_videomode_to_var(&par->var, &par->mode);
    
    info->cmap.len = 32;
    info->var = par->var;
    info->fbops = &myfb_ops;
    info->flags = FBINFO_FLAG_DEFAULT;
    info->fix.visual = FB_VISUAL_TRUECOLOR;
    info->pseudo_palette = par->pseudo_palette;

    // init buffer
    par->vram_size = (MAX_XRES * MAX_YRES * MAX_BPP * 4) / 8; // compatible with openpandora
    par->vram_virt = dma_alloc_coherent(NULL, par->vram_size, (resource_size_t*)&par->vram_phys, GFP_KERNEL | GFP_DMA);
    if(!par->vram_virt){
      printk("%s, failed to allocate dma buffer for vram\n", __func__);
      return -ENOMEM; // sorry, no error handling
    }
    memset(par->vram_virt, 0, par->vram_size);
    info->screen_base = (char __iomem*)par->vram_virt;
    strcpy(par->fix.id, DRIVER_NAME);
    par->fix.type = FB_TYPE_PACKED_PIXELS;
    par->fix.visual = FB_VISUAL_TRUECOLOR;
    par->fix.type_aux = 0;
    par->fix.xpanstep = 0;
    par->fix.ypanstep = 1;
    par->fix.ywrapstep = 0;
    par->fix.accel = FB_ACCEL_NONE;
    par->fix.smem_start = par->vram_phys;
    par->fix.smem_len = par->vram_size;
    par->fix.line_length = (par->mode.xres * par->bpp) / 8;
    info->fix = par->fix;

    par->v_palette_base = dma_alloc_coherent(NULL, PALETTE_SIZE, (resource_size_t*)&par->p_palette_base, GFP_KERNEL | GFP_DMA);
    if(!par->v_palette_base){
      printk("%s, failed to allocate dma buffer for v_palette\n", __func__);
      return -ENOMEM; // sorry, no error handling
    }
    memset(par->v_palette_base, 0, PALETTE_SIZE);
    if(fb_alloc_cmap(&info->cmap, PALETTE_SIZE, 0)){
      printk("%s, failed to allocate buffer for cmap\n", __func__);
      return -ENOMEM; // sorry, no error handling
    }

    // register framebuffer
    par->id = x;
    mylcd.fb[x] = par;
    fb_set_var(info, &par->var);
    dev_set_drvdata(&device->dev, info);
    if(register_framebuffer(info) < 0){
      printk("%s, failed to register framebuffer fb%d\n", __func__, x);
      return -ENOMEM; // sorry, no error handling
    }
  }

  // init buffer
  mylcd.lram_size = (LCD_XRES * LCD_YRES * LCD_BPP) / 8;
  for(x=0; x<LCD_RAM_PAGE; x++){
    mylcd.lram_virt[x] = dma_alloc_coherent(NULL, mylcd.lram_size, (resource_size_t*)&mylcd.lram_phys[x], GFP_KERNEL | GFP_DMA);
    if(!mylcd.lram_virt[x]){
      printk("%s, failed to allocate dma buffer for lram[%d]\n", __func__, x);
      return -ENOMEM; // sorry, no error handling
    }
    memset(mylcd.lram_virt[x], 0, mylcd.lram_size);
  }
	
	mylcd.is_double_buffer = 1;
	mylcd.double_buffer_ready = 0;
  memcpy(mylcd.lram_virt[LCD_RAW_INDEX], mylcd.fb[0]->vram_virt, (LCD_XRES * LCD_YRES * LCD_BPP) / 8);
	lcd_pixel_process(par->var.xres, par->var.yres, par->var.bits_per_pixel);

  mylcd.vsync_timeout = HZ / 5;
  init_waitqueue_head(&mylcd.vsync_wait);
 
	mylcd.thread_exit = 0;
	mylcd.thread_id = kernel_thread(lcd_thread, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
	printk("%s, thread id: 0x%x\n", __func__, mylcd.thread_id);
  return 0;
}

static int myfb_suspend(struct platform_device *dev, pm_message_t state)
{
  struct fb_info *info = platform_get_drvdata(dev);
  return 0;
}

static int myfb_resume(struct platform_device *dev)
{
  struct fb_info *info = platform_get_drvdata(dev);
  return 0;
}

static struct platform_driver fb_driver = {
  .probe    = myfb_probe,
  .remove   = myfb_remove,
  .suspend  = myfb_suspend,
  .resume   = myfb_resume,
  .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
  },
};

static int __init fb_init(void)
{
  memset(&mylcd, 0 ,sizeof(mylcd));
  return platform_driver_register(&fb_driver);
}

static void __exit fb_cleanup(void)
{
  platform_driver_unregister(&fb_driver);
}

module_init(fb_init);
module_exit(fb_cleanup);

MODULE_DESCRIPTION("Enhanced framebuffer driver for RS97");
MODULE_AUTHOR("Steward Fu <steward.fu@gmail.com>");
MODULE_LICENSE("GPL");

