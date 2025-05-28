#ifndef _FONT_H_
#define _FONT_H_

#include <stdint.h>

/**
 * @brief Simple 8x8 bitmap font structure
 */
typedef struct {
  uint8_t width;   /* Character width in pixels */
  uint8_t height;  /* Character height in pixels */
  uint8_t data[8]; /* 8 bytes for 8x8 character */
} font_char_t;

/**
 * @brief Initialize the font system
 */
void font_init();

/**
 * @brief Draw a single character at specified position
 * 
 * @param x X coordinate
 * @param y Y coordinate  
 * @param ch Character to draw
 * @param color Color of the character
 * @return 0 on success, non-zero otherwise
 */
int draw_char(uint16_t x, uint16_t y, char ch, uint32_t color);

/**
 * @brief Draw a string at specified position
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param str String to draw
 * @param color Color of the text
 * @return 0 on success, non-zero otherwise
 */
int draw_string(uint16_t x, uint16_t y, const char *str, uint32_t color);

/**
 * @brief Draw a pixel at specified coordinates
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Pixel color
 * @return 0 on success, non-zero otherwise
 */
int draw_pixel(uint16_t x, uint16_t y, uint32_t color);

/**
 * @brief Clear the screen with specified color
 * 
 * @param color Background color
 * @return 0 on success, non-zero otherwise
 */
int clear_screen(uint32_t color);

#endif /* _FONT_H_ */