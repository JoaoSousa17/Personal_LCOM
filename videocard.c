#include "videocard.h"
#include <machine/int86.h>
#include <lcom/vbe.h>

static void *video_mem;         /* Process (virtual) address to which VRAM is mapped */
static vbe_mode_info_t vmi_p;   /* VBE mode information */
static uint16_t h_res;          /* Horizontal resolution */
static uint16_t v_res;          /* Vertical resolution */
static uint8_t bits_per_pixel;  /* Bits per pixel */
static uint8_t bytes_per_pixel; /* Bytes per pixel */
static uint8_t red_mask_size, green_mask_size, blue_mask_size;
static uint8_t red_field_position, green_field_position, blue_field_position;

int map_vram(uint16_t mode) {
  struct minix_mem_range mr;
  unsigned int vram_base;  /* VRAM's physical address */
  unsigned int vram_size;  /* VRAM's size */

  /* Initialize vbe_mode_info_t struct */
  if (vbe_get_mode_info(mode, &vmi_p) != OK) {
    printf("map_vram(): vbe_get_mode_info() failed\n");
    return 1;
  }

  /* Store relevant mode information */
  h_res = vmi_p.XResolution;
  v_res = vmi_p.YResolution;
  bits_per_pixel = vmi_p.BitsPerPixel;
  bytes_per_pixel = (bits_per_pixel + 7) / 8;
  
  red_mask_size = vmi_p.RedMaskSize;
  green_mask_size = vmi_p.GreenMaskSize;
  blue_mask_size = vmi_p.BlueMaskSize;
  
  red_field_position = vmi_p.RedFieldPosition;
  green_field_position = vmi_p.GreenFieldPosition;
  blue_field_position = vmi_p.BlueFieldPosition;

  /* Calculate VRAM physical address and size */
  vram_base = vmi_p.PhysBasePtr;
  vram_size = h_res * v_res * bytes_per_pixel;

  /* Allow memory mapping */
  mr.mr_base = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + vram_size;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != OK) {
    printf("map_vram(): sys_privctl (ADD_MEM) failed\n");
    return 1;
  }

  /* Map memory */
  video_mem = vm_map_phys(SELF, (void *)mr.mr_base, vram_size);

  if (video_mem == MAP_FAILED) {
    printf("map_vram(): vm_map_phys() failed\n");
    return 1;
  }

  return 0;
}

int set_graphics_mode(uint16_t mode) {
  reg86_t reg86;

  memset(&reg86, 0, sizeof(reg86));
  reg86.intno = 0x10;
  reg86.ah = 0x4F;
  reg86.al = 0x02;
  reg86.bx = (1 << 14) | mode;  /* Set bit 14 to use linear framebuffer */

  if (sys_int86(&reg86) != OK) {
    printf("set_graphics_mode(): sys_int86() failed\n");
    return 1;
  }

  return 0;
}

int exit_graphics_mode() {
  reg86_t reg86;

  memset(&reg86, 0, sizeof(reg86));
  reg86.intno = 0x10;
  reg86.ah = 0x00;
  reg86.al = 0x03;  /* Standard text mode (80x25) */

  if (sys_int86(&reg86) != OK) {
    printf("exit_graphics_mode(): sys_int86() failed\n");
    return 1;
  }

  /* Unmap VRAM if it was mapped */
  if (video_mem != NULL) {
    uint32_t vram_size = h_res * v_res * bytes_per_pixel;
    vm_unmap_phys(SELF, video_mem, vram_size);
    video_mem = NULL;
  }

  return 0;
}

uint16_t get_h_res() {
  return h_res;
}

uint16_t get_v_res() {
  return v_res;
}

uint8_t get_bits_per_pixel() {
  return bits_per_pixel;
}

vbe_mode_info_t *get_vmi_p() {
  return &vmi_p;
} 
