#include "letter_rain.h"
#include "sprite.h"
#include "videocard.h"
#include "font.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/* Simple XPM patterns for letters - 16x16 pixels */
static const char *letter_a_xpm[] = {
    "16 16 2 1",
    "0 c #000000",
    "1 c #FFFFFF",
    "0000000000000000",
    "0000001100000000",
    "0000011110000000",
    "0000110011000000",
    "0001100001100000",
    "0011000000110000",
    "0110000000011000",
    "0111111111111000",
    "1110000000001100",
    "1100000000001100",
    "1100000000001100",
    "1100000000001100",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000",
    "0000000000000000"
};

static const char *board_xpm[] = {
    "100 20 3 1",
    "0 c #000000",
    "1 c #FFD700",
    "2 c #FFA500",
    "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222221",
    "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"
};

char get_random_letter() {
    uint16_t rand_val = rand() % 10000;
    
    for (int i = 0; i < 26; i++) {
        if (rand_val < letter_freq_table[i]) {
            return 'A' + i;
        }
    }
    
    return 'A'; // Fallback
}

Sprite *create_letter_sprite(char letter, int x, int y) {
    // For simplicity, we'll use the same XPM for all letters and draw the character over it
    // In a real implementation, you'd have different XPMs for each letter
    return create_sprite(letter_a_xpm, x, y, 0, LETTER_FALL_SPEED);
}

Sprite *create_board_sprite(int x, int y) {
    return create_sprite(board_xpm, x, y, 0, 0);
}

int letter_rain_init(letter_rain_t *game) {
    if (game == NULL)
        return 1;
    
    // Initialize random seed
    srand(time(NULL));
    
    // Initialize all letters as inactive
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        game->letters[i].active = false;
        game->letters[i].sprite = NULL;
        game->letters[i].letter = 'A';
        game->letters[i].color = 0xFFFFFF;
    }
    
    // Initialize board at bottom center
    uint16_t screen_width = get_h_res();
    uint16_t screen_height = get_v_res();
    
    game->board.x = (screen_width - BOARD_WIDTH) / 2;
    game->board.y = screen_height - BOARD_HEIGHT - 20;
    game->board.width = BOARD_WIDTH;
    game->board.height = BOARD_HEIGHT;
    game->board.missed_count = 0;
    game->board.sprite = create_board_sprite(game->board.x, game->board.y);
    
    if (game->board.sprite == NULL)
        return 1;
    
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
                int x = rand() % (get_h_res() - 16);
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
                game->caught_letter = game->letters[i].letter;
                game->game_over = true;
                return 1;
            }
            
            // Check if letter hit the bottom edge (missed)
            if (game->letters[i].sprite->y >= get_v_res()) {
                game->board.missed_count++;
                
                // Deactivate letter
                destroy_sprite(game->letters[i].sprite);
                game->letters[i].sprite = NULL;
                game->letters[i].active = false;
                
                // Check if game over (2 misses)
                if (game->board.missed_count >= 2) {
                    game->game_over = true;
                    return 1;
                }
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
    
    // Draw board using sprite
    if (game->board.sprite != NULL) {
        if (draw_sprite(game->board.sprite, (char*)get_video_mem()) != 0)
            return 1;
    }
    
    // Draw falling letters
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].active && game->letters[i].sprite != NULL) {
            // Draw sprite
            if (draw_sprite(game->letters[i].sprite, (char*)get_video_mem()) != 0)
                return 1;
            
            // Draw letter character on top of sprite
            char letter_str[2] = {game->letters[i].letter, '\0'};
            int text_x = game->letters[i].sprite->x + 4;
            int text_y = game->letters[i].sprite->y + 4;
            
            if (draw_string_scaled(text_x, text_y, letter_str, 0x000000, 1) != 0)
                return 1;
        }
    }
    
    // Draw UI info
    char info[50];
    snprintf(info, sizeof(info), "Misses: %d/2", game->board.missed_count);
    if (draw_string_scaled(20, 20, info, 0xFFFFFF, 2) != 0)
        return 1;
    
    // Draw controls
    if (draw_string_scaled(20, get_v_res() - 40, "Use A and D to move the board", 0xFFFFFF, 1) != 0)
        return 1;
    
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
            if (game->board.x < get_h_res() - game->board.width - BOARD_SPEED) {
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
