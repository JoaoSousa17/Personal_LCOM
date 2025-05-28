#include "sprite.h"
#include "videocard.h"
#include <lcom/lcf.h>
#include <lcom/xpm.h>
#include <stdlib.h>

Sprite *create_sprite(const char *pic[], int x, int y, int xspeed, int yspeed) {
    // Allocate space for the sprite
    Sprite *sp = (Sprite *) malloc(sizeof(Sprite));
    xpm_image_t img;
    
    if (sp == NULL)
        return NULL;
    
    // Read the sprite pixmap
    sp->map = xpm_load(pic, XPM_INDEXED, &img);
    if (sp->map == NULL) {
        free(sp);
        return NULL;
    }
    
    sp->width = img.width;
    sp->height = img.height;
    sp->x = x;
    sp->y = y;
    sp->xspeed = xspeed;
    sp->yspeed = yspeed;
    
    return sp;
}

void destroy_sprite(Sprite *sp) {
    if (sp == NULL)
        return;
    if (sp->map)
        free(sp->map);
    free(sp);
}

int animate_sprite(Sprite *sp) {
    if (sp == NULL)
        return 1;
    
    sp->x += sp->xspeed;
    sp->y += sp->yspeed;
    
    return 0;
}

int draw_sprite(Sprite *sp, char *base) {
    if (sp == NULL || sp->map == NULL)
        return 1;
    
    // Get video memory and screen dimensions
    void *video_mem = get_video_mem();
    uint16_t h_res = get_h_res();
    uint16_t v_res = get_v_res();
    uint8_t bytes_per_pixel = (get_bits_per_pixel() + 7) / 8;
    
    // Draw sprite pixel by pixel
    for (int row = 0; row < sp->height; row++) {
        for (int col = 0; col < sp->width; col++) {
            int screen_x = sp->x + col;
            int screen_y = sp->y + row;
            
            // Check bounds
            if (screen_x >= 0 && screen_x < h_res && screen_y >= 0 && screen_y < v_res) {
                uint8_t pixel_color = sp->map[row * sp->width + col];
                
                // Don't draw transparent pixels (assuming 0 is transparent)
                if (pixel_color != 0) {
                    // Convert indexed color to RGB if needed
                    uint32_t color;
                    switch (pixel_color) {
                        case 1: color = 0xFFFFFF; break; // White
                        case 2: color = 0xFF0000; break; // Red
                        case 3: color = 0x00FF00; break; // Green
                        case 4: color = 0x0000FF; break; // Blue
                        case 5: color = 0xFFFF00; break; // Yellow
                        case 6: color = 0xFF00FF; break; // Magenta
                        case 7: color = 0x00FFFF; break; // Cyan
                        default: color = 0xFFFFFF; break; // Default white
                    }
                    
                    draw_pixel(screen_x, screen_y, color);
                }
            }
        }
    }
    
    return 0;
}

int check_boundary_collision(Sprite *sp) {
    if (sp == NULL)
        return 0;
    
    uint16_t h_res = get_h_res();
    uint16_t v_res = get_v_res();
    
    // Check if sprite is outside screen boundaries
    if (sp->x < 0 || sp->x + sp->width > h_res ||
        sp->y < 0 || sp->y + sp->height > v_res) {
        return 1;
    }
    
    return 0;
}

void set_sprite_position(Sprite *sp, int x, int y) {
    if (sp == NULL)
        return;
    
    sp->x = x;
    sp->y = y;
}

void set_sprite_speed(Sprite *sp, int xspeed, int yspeed) {
    if (sp == NULL)
        return;
    
    sp->xspeed = xspeed;
    sp->yspeed = yspeed;
}
