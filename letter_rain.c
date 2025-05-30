#include "letter_rain.h"
#include "sprite.h"
#include "videocard.h"
#include "font.h"
#include <stdlib.h>
#include <string.h>
// Removido #include <time.h> pois não está disponível no MINIX

/* Scancode definitions */
#define A_MAKE 0x1E
#define D_MAKE 0x20

/* Letter frequency table (cumulative probabilities) */
static const uint16_t letter_freq_table[26] = {
    1463,  // A: 0-1462
    1567,  // B: 1463-1566
    1955,  // C: 1567-1954
    2454,  // D: 1955-2453
    3711,  // E: 2454-3710
    3813,  // F: 3711-3812
    3943,  // G: 3813-3942
    4071,  // H: 3943-4070
    4689,  // I: 4071-4688
    4729,  // J: 4689-4728
    4731,  // K: 4729-4730
    5009,  // L: 4731-5008
    5483,  // M: 5009-5482
    5988,  // N: 5483-5987
    7061,  // O: 5988-7060
    7313,  // P: 7061-7312
    7433,  // Q: 7313-7432
    8086,  // R: 7433-8085
    8867,  // S: 8086-8866
    9301,  // T: 8867-9300
    9764,  // U: 9301-9763
    9931,  // V: 9764-9930
    9932,  // W: 9931-9931
    9953,  // X: 9932-9952
    9954,  // Y: 9953-9953
    10000  // Z: 9954-9999
};

/* Variável estática para seed do random */
static uint32_t rand_seed = 1;

/* Função simples de random (Linear Congruential Generator) */
static uint32_t simple_rand() {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768;
}

/* Inicializa o seed do random */
static void simple_srand(uint32_t seed) {
    rand_seed = seed;
}

/* Removido board_pattern desnecessário */

char get_random_letter() {
    uint16_t rand_val = simple_rand() % 10000;
    
    for (int i = 0; i < 26; i++) {
        if (rand_val < letter_freq_table[i]) {
            return 'A' + i;
        }
    }
    
    return 'A'; // Fallback
}

/* Função simplificada para criar sprite de letra */
Sprite *create_letter_sprite(char letter, int x, int y) {
    Sprite *sp = (Sprite *) malloc(sizeof(Sprite));
    if (sp == NULL) {
        return NULL;
    }
    
    sp->width = 16;
    sp->height = 16;
    sp->x = x;
    sp->y = y;
    sp->xspeed = 0;
    sp->yspeed = LETTER_FALL_SPEED;
    sp->map = NULL; // Não precisamos de mapa para desenho simples
    
    return sp;
}

/* Função simplificada para criar sprite do board */
Sprite *create_board_sprite(int x, int y) {
    Sprite *sp = (Sprite *) malloc(sizeof(Sprite));
    if (sp == NULL) {
        return NULL;
    }
    
    sp->width = BOARD_WIDTH;
    sp->height = BOARD_HEIGHT;
    sp->x = x;
    sp->y = y;
    sp->xspeed = 0;
    sp->yspeed = 0;
    sp->map = NULL; // Não precisamos de mapa para desenho simples
    
    return sp;
}

int letter_rain_init(letter_rain_t *game) {
    if (game == NULL)
        return 1;
    
    // Initialize random seed com valor baseado em endereço (pseudo-random)
    simple_srand((uint32_t)game);
    
    // Initialize all letters as inactive
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        game->letters[i].active = false;
        game->letters[i].sprite = NULL;
        game->letters[i].letter = 'A';
        game->letters[i].color = 0xFFFFFF;
    }
    
    // Initialize letter collection counters (A=0, B=1, ..., Z=25)
    for (int i = 0; i < 26; i++) {
        game->letter_counters[i] = 0;
    }
    
    // Initialize board at bottom center
    uint16_t screen_width = get_h_res();
    uint16_t screen_height = get_v_res();
    
    if (screen_width == 0 || screen_height == 0) {
        return 1; // Tela não inicializada
    }
    
    game->board.x = (screen_width - BOARD_WIDTH) / 2;
    game->board.y = screen_height - BOARD_HEIGHT - 20;
    game->board.width = BOARD_WIDTH;
    game->board.height = BOARD_HEIGHT;
    // Removed missed_count as it's no longer needed
    
    // Criar sprite do board
    game->board.sprite = create_board_sprite(game->board.x, game->board.y);
    if (game->board.sprite == NULL) {
        return 1;
    }
    
    // Initialize game state
    game->caught_letter = 0;
    game->game_over = false;
    game->frame_counter = 0;
    game->spawn_rate = 60; // Spawn a letter every second (60 FPS)
    
    return 0;
}

int letter_rain_update(letter_rain_t *game) {
    if (game == NULL || game->game_over)
        return 1;
    
    game->frame_counter++;
    
    // Spawn new letter if it's time
    if (game->frame_counter >= game->spawn_rate) {
        game->frame_counter = 0;
        
        // Find inactive letter slot
        for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
            if (!game->letters[i].active) {
                game->letters[i].letter = get_random_letter();
                game->letters[i].active = true;
                
                // Random X position across screen width
                uint16_t screen_width = get_h_res();
                if (screen_width <= 16) break; // Evitar divisão por zero
                
                int x = simple_rand() % (screen_width - 16);
                int y = -16; // Start above screen
                
                game->letters[i].sprite = create_letter_sprite(game->letters[i].letter, x, y);
                if (game->letters[i].sprite == NULL) {
                    game->letters[i].active = false;
                }
                break;
            }
        }
    }
    
    // Update falling letters
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].active && game->letters[i].sprite != NULL) {
            // Move letter down
            animate_sprite(game->letters[i].sprite);
            
            // Check if letter hit the board
            if (check_letter_board_collision(&game->letters[i], &game->board)) {
                char caught = game->letters[i].letter;
                int letter_index = caught - 'A'; // A=0, B=1, ..., Z=25
                
                // Increment counter for this letter
                if (letter_index >= 0 && letter_index < 26) {
                    game->letter_counters[letter_index]++;
                    
                    // Check if we caught this letter twice
                    if (game->letter_counters[letter_index] >= 2) {
                        game->caught_letter = caught;
                        game->game_over = true;
                        
                        // Cleanup this letter
                        destroy_sprite(game->letters[i].sprite);
                        game->letters[i].sprite = NULL;
                        game->letters[i].active = false;
                        
                        return 1; // Game won!
                    }
                }
                
                // Cleanup this letter (caught once, continue game)
                destroy_sprite(game->letters[i].sprite);
                game->letters[i].sprite = NULL;
                game->letters[i].active = false;
                continue;
            }
            
            // Check if letter hit the bottom edge (missed) - just ignore it
            if (game->letters[i].sprite->y >= (int)get_v_res()) {
                // Simply deactivate letter - no penalty
                destroy_sprite(game->letters[i].sprite);
                game->letters[i].sprite = NULL;
                game->letters[i].active = false;
            }
        }
    }
    
    return 0;
}

int letter_rain_draw(letter_rain_t *game) {
    if (game == NULL)
        return 1;
    
    // Clear screen
    if (clear_screen(0x1a1a2e) != 0)
        return 1;
    
    // Draw board using simple rectangle instead of sprite
    if (draw_filled_rectangle(game->board.x, game->board.y, 
                             game->board.width, game->board.height, 0xFFD700) != 0) {
        return 1;
    }
    
    // Draw board border
    if (draw_rectangle_border(game->board.x, game->board.y, 
                             game->board.width, game->board.height, 0xFFA500, 2) != 0) {
        return 1;
    }
    
    // Draw falling letters
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].active && game->letters[i].sprite != NULL) {
            // Draw letter background (simple rectangle)
            if (draw_filled_rectangle(game->letters[i].sprite->x, game->letters[i].sprite->y,
                                     16, 16, 0x666666) != 0) {
                return 1;
            }
            
            // Draw letter character on top
            char letter_str[2] = {game->letters[i].letter, '\0'};
            int text_x = game->letters[i].sprite->x + 4;
            int text_y = game->letters[i].sprite->y + 4;
            
            if (draw_string_scaled(text_x, text_y, letter_str, 0xFFFFFF, 1) != 0)
                return 1;
        }
    }
    
    // Draw UI info - removed miss counter since misses are now ignored
    char status[100];
    sprintf(status, "Letter Collection Status:");
    if (draw_string_scaled(20, 20, status, 0x00FF88, 1) != 0)
        return 1;
    
    // Show counters for letters that have been caught at least once
    int line_y = 40;
    for (int i = 0; i < 26; i++) {
        if (game->letter_counters[i] > 0) {
            char letter_info[20];
            sprintf(letter_info, "%c: %d/2", 'A' + i, game->letter_counters[i]);
            
            uint32_t color = (game->letter_counters[i] >= 2) ? 0x00FF00 : 0xFFFF00; // Green if complete, yellow if partial
            if (draw_string_scaled(20, line_y, letter_info, color, 1) != 0)
                return 1;
            line_y += 15;
        }
    }
    
    return 0;
}

int letter_rain_handle_input(letter_rain_t *game, uint8_t scancode) {
    if (game == NULL || game->game_over)
        return 1;
    
    switch (scancode) {
        case A_MAKE: // Move board left
            if (game->board.x > BOARD_SPEED) {
                game->board.x -= BOARD_SPEED;
                if (game->board.sprite != NULL) {
                    set_sprite_position(game->board.sprite, game->board.x, game->board.y);
                }
            }
            break;
            
        case D_MAKE: // Move board right
            if (game->board.x < (int)get_h_res() - game->board.width - BOARD_SPEED) {
                game->board.x += BOARD_SPEED;
                if (game->board.sprite != NULL) {
                    set_sprite_position(game->board.sprite, game->board.x, game->board.y);
                }
            }
            break;
    }
    
    return 0;
}

bool check_letter_board_collision(falling_letter_t *letter, board_t *board) {
    if (letter == NULL || letter->sprite == NULL || board == NULL)
        return false;
    
    int letter_x = letter->sprite->x;
    int letter_y = letter->sprite->y;
    int letter_w = letter->sprite->width;
    int letter_h = letter->sprite->height;
    
    int board_x = board->x;
    int board_y = board->y;
    int board_w = board->width;
    int board_h = board->height;
    
    // Check for collision using AABB (Axis-Aligned Bounding Box)
    return (letter_x < board_x + board_w &&
            letter_x + letter_w > board_x &&
            letter_y < board_y + board_h &&
            letter_y + letter_h > board_y);
}

void letter_rain_cleanup(letter_rain_t *game) {
    if (game == NULL)
        return;
    
    // Cleanup falling letters
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].sprite != NULL) {
            destroy_sprite(game->letters[i].sprite);
            game->letters[i].sprite = NULL;
        }
        game->letters[i].active = false;
    }
    
    // Cleanup board
    if (game->board.sprite != NULL) {
        destroy_sprite(game->board.sprite);
        game->board.sprite = NULL;
    }
}
