#ifndef _SPRITE_H_
#define _SPRITE_H_

#include <stdint.h>

/**
 * @brief Basic sprite structure
 */
typedef struct {
    int x, y;           // current position
    int width, height;  // dimensions
    int xspeed, yspeed; // current speed
    char *map;          // the pixmap
} Sprite;

/**
 * @brief Creates a new sprite from XPM data
 * 
 * @param pic XPM data array
 * @param x Initial X position
 * @param y Initial Y position
 * @param xspeed Horizontal speed
 * @param yspeed Vertical speed
 * @return Pointer to new sprite or NULL on error
 */
Sprite *create_sprite(const char *pic[], int x, int y, int xspeed, int yspeed);

/**
 * @brief Destroys a sprite and frees its memory
 * 
 * @param sp Pointer to sprite
 */
void destroy_sprite(Sprite *sp);

/**
 * @brief Moves the sprite based on its speed
 * 
 * @param sp Pointer to sprite
 * @return 0 on success, non-zero otherwise
 */
int animate_sprite(Sprite *sp);

/**
 * @brief Draws the sprite on the screen
 * 
 * @param sp Pointer to sprite
 * @param base Pointer to video memory base
 * @return 0 on success, non-zero otherwise
 */
int draw_sprite(Sprite *sp, char *base);

/**
 * @brief Check if sprite collides with screen boundaries
 * 
 * @param sp Pointer to sprite
 * @return 1 if collision detected, 0 otherwise
 */
int check_boundary_collision(Sprite *sp);

/**
 * @brief Set sprite position
 * 
 * @param sp Pointer to sprite
 * @param x New X position
 * @param y New Y position
 */
void set_sprite_position(Sprite *sp, int x, int y);

/**
 * @brief Set sprite speed
 * 
 * @param sp Pointer to sprite
 * @param xspeed New horizontal speed
 * @param yspeed New vertical speed
 */
void set_sprite_speed(Sprite *sp, int xspeed, int yspeed);

#endif /* _SPRITE_H_ */
