#ifndef _VIDEOCARD_H_
#define _VIDEOCARD_H_

#include <lcom/lcf.h>
#include <stdint.h>

/**
 * @brief Maps the VRAM to the process's address space
 * 
 * @param mode The video mode to set
 * @return 0 on success, non-zero otherwise
 */
int map_vram(uint16_t mode);

/**
 * @brief Sets the video card to the specified graphics mode
 * 
 * @param mode The video mode to set
 * @return 0 on success, non-zero otherwise
 */
int set_graphics_mode(uint16_t mode);

/**
 * @brief Gets the horizontal resolution of the current video mode
 * 
 * @return Horizontal resolution
 */
uint16_t get_h_res();

/**
 * @brief Gets the vertical resolution of the current video mode
 * 
 * @return Vertical resolution
 */
uint16_t get_v_res();

/**
 * @brief Gets the bits per pixel of the current video mode
 * 
 * @return Bits per pixel
 */
uint8_t get_bits_per_pixel();

/**
 * @brief Gets a pointer to the video_info structure
 * 
 * @return Pointer to the vmi_p structure
 */
vbe_mode_info_t *get_vmi_p();

#endif /* _VIDEOCARD_H_ */ 
