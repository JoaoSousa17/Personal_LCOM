#ifndef __VIDEOCARD_H
#define __VIDEOCARD_H

#include <lcom/lcf.h>

int set_graphics_mode(uint16_t mode);
int map_vram(uint16_t mode);
int draw_pixel(uint16_t x, uint16_t y, uint32_t color);
int draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
int draw_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

// Getters
uint16_t get_h_res(void);
uint16_t get_v_res(void);
uint8_t get_bits_per_pixel(void);
vbe_mode_info_t* get_vmi_p(void);

#endif /* __VIDEOCARD_H */

