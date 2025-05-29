#include "sprite.h"
#include "videocard.h"
#include <lcom/lcf.h>
#include <lcom/xpm.h>
#include <stdlib.h>

Sprite *create_sprite(const char *pic[], int x, int y, int xspeed, int yspeed) {
    // Allocate space for the sprite
    Sprite *sp = (Sprite *) malloc(sizeof(Sprite));
    if (sp == NULL)
        return NULL;
    
    // Se pic é NULL, criar sprite simples sem XPM
    if (pic == NULL) {
        sp->width = 16;
        sp->height = 16;
        sp->map = (char*)malloc(sp->width * sp->height);
        if (sp->map == NULL) {
            free(sp);
            return NULL;
        }
        memset(sp->map, 1, sp->width * sp->height); // Preencher com cor 1
    } else {
        // Tentar carregar XPM
        xpm_image_t img;
        sp->map = (char*)xpm_load(pic, XPM_INDEXED, &img);
        if (sp->map == NULL) {
            // Se falhar, criar sprite simples
            sp->width = 16;
            sp->height = 16;
            sp->map = (char*)malloc(sp->width * sp->height);
            if (sp->map == NULL) {
                free(sp);
                return NULL;
            }
            memset(sp->map, 1, sp->width * sp->height);
        } else {
            sp->width = img.width;
            sp->height = img.height;
        }
    }
    
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
    
    // Get screen dimensions
    uint16_t h_res = get_h_res();
    uint16_t v_res = get_v_res();
    
    // Check if sprite is completely outside screen bounds
    if (sp->x >= (int)h_res || sp->y >= (int)v_res || 
        sp->x + sp->width <= 0 || sp->y + sp->height <= 0) {
        return 0; // Don't draw if completely outside
    }
    
    // Draw sprite pixel by pixel with bounds checking
    for (int row = 0; row < sp->height; row++) {
        for (int col = 0; col < sp->width; col++) {
            int screen_x = sp->x + col;
            int screen_y = sp->y + row;
            
            // Check bounds
            if (screen_x >= 0 && screen_x < (int)h_res && 
                screen_y >= 0 && screen_y < (int)v_res) {
                
                uint8_t pixel_color = (uint8_t)sp->map[row * sp->width + col];
                
                // Don't draw transparent pixels (assuming 0 is transparent)
                if (pixel_color != 0) {
                    // Convert indexed color to RGB
                    uint32_t color;
                    switch (pixel_color) {
                        case 1: color = 0xFFFFFF; break; // White
                        case 2: color = 0xFFD700; break; // Gold
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
    if (sp->x < 0 || sp->x + sp->width > (int)h_res ||
        sp->y < 0 || sp->y + sp->height > (int)v_res) {
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
