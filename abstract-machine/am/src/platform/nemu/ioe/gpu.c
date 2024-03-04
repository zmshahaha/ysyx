#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t info = inl(VGACTL_ADDR);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = info >> 16, .height = info & 0xffff,
    .vmemsz = (info >> 16) * (info & 0xffff) * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t vga_w = inl(VGACTL_ADDR) >> 16;
  int w = ctl->w, h = ctl->h, x = ctl->x, y = ctl->y;
  uint32_t *fb = (uint32_t *)FB_ADDR;
  uint32_t *pixels = (uint32_t *)ctl->pixels;

  for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
      fb[(y+i) * vga_w + x + j] = pixels[i*w + j];

  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
