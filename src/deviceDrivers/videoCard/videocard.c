#include "videocard.h"
#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>

static vbe_mode_info_t vmi_p;
static void *video_mem;
static uint16_t h_res;
static uint16_t v_res;
static uint8_t bits_per_pixel;

int map_vram(uint16_t mode) {
  if (vbe_get_mode_info(mode, &vmi_p) != OK) {
    printf("Failed to get VBE mode info\n");
    return 1;
  }

  if (vmi_p.XResolution == 0 || vmi_p.YResolution == 0 || vmi_p.BitsPerPixel == 0) {
    printf("Invalid mode information received.\n");
    return 1;
  }

  h_res = vmi_p.XResolution;
  v_res = vmi_p.YResolution;
  bits_per_pixel = vmi_p.BitsPerPixel;

  unsigned int bytes_per_pixel_local = (bits_per_pixel + 7) / 8;

  struct minix_mem_range mr;
  mr.mr_base = vmi_p.PhysBasePtr;
  mr.mr_limit = mr.mr_base + (unsigned int)(h_res) * (unsigned int)(v_res) * bytes_per_pixel_local;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != OK) {
    printf("sys_privctl (ADD_MEM) failed\n");
    return 1;
  }

  video_mem = vm_map_phys(SELF, (void *)mr.mr_base, (unsigned int)(h_res) * (unsigned int)(v_res) * bytes_per_pixel_local);

  if (video_mem == MAP_FAILED) {
    printf("Couldn't map video memory\n");
    return 1;
  }

  return 0;
}

int set_graphics_mode(uint16_t mode) {
  reg86_t r;

  memset(&r, 0, sizeof(r));
  r.intno = 0x10;
  r.ah = 0x4F;
  r.al = 0x02;
  r.bx = (1 << 14) | mode;

  if (sys_int86(&r) != OK) {
    printf("set_graphics_mode(): sys_int86() failed\n");
    return 1;
  }

  return 0;
}

int draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= h_res || y >= v_res) return 1;

  unsigned int bytes_per_pixel_local = (bits_per_pixel + 7) / 8;

  uint8_t *pixel_addr = ((uint8_t *)video_mem) + ((unsigned int)(y) * (unsigned int)(h_res) + (unsigned int)(x)) * bytes_per_pixel_local;

  for (unsigned int i = 0; i < bytes_per_pixel_local; i++) {
    pixel_addr[i] = (color >> (i * 8)) & 0xFF;
  }

  return 0;
}

int draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (uint16_t i = 0; i < len; i++) {
    if (draw_pixel(x + i, y, color) != 0) return 1;
  }
  return 0;
}

int draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for (uint16_t i = 0; i < height; i++) {
    if (draw_hline(x, y + i, width, color) != 0) return 1;
  }
  return 0;
}

// Getters
uint16_t get_h_res(void) {
  return h_res;
}

uint16_t get_v_res(void) {
  return v_res;
}

uint8_t get_bits_per_pixel(void) {
  return bits_per_pixel;
}

vbe_mode_info_t* get_vmi_p(void) {
  return &vmi_p;
}
