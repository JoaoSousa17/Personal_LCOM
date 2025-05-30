#include "letter_rain.h"
#include "sprite.h"
#include "videocard.h"
#include "font.h"
#include <stdlib.h>
#include <string.h>

/* Scancode definitions */
#define A_MAKE 0x1E
#define D_MAKE 0x20

/* Variável estática para seed do random */
static uint32_t rand_seed = 1;

/* Função simples de random (Linear Congruential Generator) */
static uint32_t simple_rand() {
    static uint32_t call_count = 0;
    call_count++;

    rand_seed = rand_seed * 1103515245 + 12345 + call_count;

    return (rand_seed / 65536) % 32768;
}

/* Inicializa o seed do random */
static void simple_srand(uint32_t seed) {
    rand_seed = seed;
}

/* Tabela de probabilidades acumuladas para letras portuguesas (x100 para evitar decimais) */
static const int letter_probabilities[] = {
    1463,  /* A: 14.63% */
    1567,  /* B: 1.04% (1463 + 104) */
    1955,  /* C: 3.88% (1567 + 388) */
    2454,  /* D: 4.99% (1955 + 499) */
    3711,  /* E: 12.57% (2454 + 1257) */
    3813,  /* F: 1.02% (3711 + 102) */
    3943,  /* G: 1.30% (3813 + 130) */
    4071,  /* H: 1.28% (3943 + 128) */
    4689,  /* I: 6.18% (4071 + 618) */
    4729,  /* J: 0.40% (4689 + 40) */
    4731,  /* K: 0.02% (4729 + 2) */
    5009,  /* L: 2.78% (4731 + 278) */
    5483,  /* M: 4.74% (5009 + 474) */
    5988,  /* N: 5.05% (5483 + 505) */
    7061,  /* O: 10.73% (5988 + 1073) */
    7313,  /* P: 2.52% (7061 + 252) */
    7433,  /* Q: 1.20% (7313 + 120) */
    8086,  /* R: 6.53% (7433 + 653) */
    8867,  /* S: 7.81% (8086 + 781) */
    9301,  /* T: 4.34% (8867 + 434) */
    9764,  /* U: 4.63% (9301 + 463) */
    9931,  /* V: 1.67% (9764 + 167) */
    9932,  /* W: 0.01% (9931 + 1) */
    9953,  /* X: 0.21% (9932 + 21) */
    9954,  /* Y: 0.01% (9953 + 1) */
    10000  /* Z: 0.47% (9954 + 46) - Total: 100.00% */
};

char get_random_letter() {
    /* Gerar número aleatório entre 0 e 9999 (equivale a 0.00% a 99.99%) */
    int random_value = simple_rand() % 10000;
    
    /* Encontrar a letra correspondente usando as probabilidades acumuladas */
    for (int i = 0; i < 26; i++) {
        if (random_value < letter_probabilities[i]) {
            return 'A' + i;  /* Retorna a letra correspondente */
        }
    }
    
    /* Fallback para 'Z' (não deve acontecer) */
    return 'Z';
}

/* Função para apagar um sprite específico */
static void erase_letter_sprite(int x, int y, int width, int height) {
    if (draw_filled_rectangle(x, y, width, height, 0x1a1a2e) != 0) {
        printf("Error erasing letter sprite\n");
    }
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
        game->letters[i].last_x = -1; /* Initialize previous positions */
        game->letters[i].last_y = -1;
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
    game->board.last_x = -1; /* Initialize previous position */
    
    // Criar sprite do board
    game->board.sprite = create_board_sprite(game->board.x, game->board.y);
    if (game->board.sprite == NULL) {
        return 1;
    }
    
    // Initialize game state
    game->caught_letter = 0;
    game->game_over = false;
    game->frame_counter = 0;
    game->spawn_rate = 35; // Spawn a letter every second (35 FPS)
    game->first_draw = true; /* Flag for first draw */
    
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
                if (screen_width <= 16) break;
                
                int x = simple_rand() % (screen_width - 30);
                int y = -16; // Start above screen
                
                game->letters[i].sprite = create_letter_sprite(game->letters[i].letter, x, y);
                if (game->letters[i].sprite == NULL) {
                    game->letters[i].active = false;
                } else {
                    /* Initialize previous position to INVALID so it won't be erased */
                    game->letters[i].last_x = -1;
                    game->letters[i].last_y = -1;
                }
                break;
            }
        }
    }
    
    // Update falling letters
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].active && game->letters[i].sprite != NULL) {
            
            // Move letter down (this updates sprite position)
            animate_sprite(game->letters[i].sprite);
            
            // Check if letter hit the board
            if (check_letter_board_collision(&game->letters[i], &game->board)) {
                char caught = game->letters[i].letter;
                int letter_index = caught - 'A';
                
                /* APAGAR o sprite da tela antes de destruir */
                if (game->letters[i].last_x >= 0 && game->letters[i].last_y >= 0) {
                    erase_letter_sprite(game->letters[i].last_x, game->letters[i].last_y, 16, 16);
                }
                erase_letter_sprite(game->letters[i].sprite->x, game->letters[i].sprite->y, 16, 16);
                
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
                        game->letters[i].last_x = -1;
                        game->letters[i].last_y = -1;
                        
                        return 1; // Game won!
                    }
                }
                
                // Cleanup this letter (caught once, continue game)
                destroy_sprite(game->letters[i].sprite);
                game->letters[i].sprite = NULL;
                game->letters[i].active = false;
                game->letters[i].last_x = -1;
                game->letters[i].last_y = -1;
                continue;
            }
            
            // Check if letter hit the bottom edge (missed)
            if (game->letters[i].sprite->y >= (int)get_v_res()) {
                /* APAGAR o sprite da tela antes de destruir */
                if (game->letters[i].last_x >= 0 && game->letters[i].last_y >= 0) {
                    erase_letter_sprite(game->letters[i].last_x, game->letters[i].last_y, 16, 16);
                }
                erase_letter_sprite(game->letters[i].sprite->x, game->letters[i].sprite->y, 16, 16);
                
                // Deactivate letter
                destroy_sprite(game->letters[i].sprite);
                game->letters[i].sprite = NULL;
                game->letters[i].active = false;
                game->letters[i].last_x = -1;
                game->letters[i].last_y = -1;
            }
        }
    }
    
    return 0;
}

int letter_rain_draw(letter_rain_t *game) {
    if (game == NULL)
        return 1;
    
    /* Only clear screen on first draw */
    if (game->first_draw) {
        if (clear_screen(0x1a1a2e) != 0)
            return 1;
        game->first_draw = false;
    }
    
    /* Handle falling letters */
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].active && game->letters[i].sprite != NULL) {
            
            /* SEMPRE apagar a posição anterior se existir */
            if (game->letters[i].last_x >= 0 && game->letters[i].last_y >= 0) {
                erase_letter_sprite(game->letters[i].last_x, game->letters[i].last_y, 16, 16);
            }
            
            /* Desenhar na nova posição apenas se estiver dentro dos limites */
            if (game->letters[i].sprite->x >= -16 && game->letters[i].sprite->y >= -16 &&
                game->letters[i].sprite->x < (int)get_h_res() + 16 && 
                game->letters[i].sprite->y < (int)get_v_res() + 16) {
                
                /* Draw letter background (simple rectangle) */
                if (draw_filled_rectangle(game->letters[i].sprite->x, game->letters[i].sprite->y,
                                         16, 16, 0x666666) != 0) {
                    return 1;
                }
                
                /* Draw letter character on top */
                char letter_str[2] = {game->letters[i].letter, '\0'};
                int text_x = game->letters[i].sprite->x + 4;
                int text_y = game->letters[i].sprite->y + 4;
                
                if (draw_string_scaled(text_x, text_y, letter_str, 0xFFFFFF, 1) != 0)
                    return 1;
            }
            
            /* SEMPRE atualizar a última posição */
            game->letters[i].last_x = game->letters[i].sprite->x;
            game->letters[i].last_y = game->letters[i].sprite->y;
        }
    }
    
    /* Erase board from previous position if it moved */
    if (game->board.last_x != -1 && game->board.last_x != game->board.x) {
        if (draw_filled_rectangle(game->board.last_x, game->board.y, 
                                 game->board.width, game->board.height, 0x1a1a2e) != 0) {
            return 1;
        }
    }
    
    /* Draw board at current position */
    if (draw_filled_rectangle(game->board.x, game->board.y, 
                             game->board.width, game->board.height, 0xFFD700) != 0) {
        return 1;
    }
    
    /* Draw board border */
    if (draw_rectangle_border(game->board.x, game->board.y, 
                             game->board.width, game->board.height, 0xFFA500, 2) != 0) {
        return 1;
    }
    
    /* Update board last position */
    game->board.last_x = game->board.x;
    
    /* Draw UI info in a fixed area */
    /* Clear the UI area first */
    if (draw_filled_rectangle(10, 10, 300, 200, 0x1a1a2e) != 0)
        return 1;
    
    char status[100];
    sprintf(status, "Letter Collection Status:");
    if (draw_string_scaled(20, 20, status, 0x00FF88, 1) != 0)
        return 1;
    
    /* Show counters for letters that have been caught at least once */
    int line_y = 40;
    for (int i = 0; i < 26; i++) {
        if (game->letter_counters[i] > 0) {
            char letter_info[20];
            sprintf(letter_info, "%c: %d/2", 'A' + i, game->letter_counters[i]);
            
            uint32_t color = (game->letter_counters[i] >= 2) ? 0x00FF00 : 0xFFFF00;
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
    
    // Cleanup falling letters - erase them first
    for (int i = 0; i < MAX_FALLING_LETTERS; i++) {
        if (game->letters[i].active && game->letters[i].sprite != NULL) {
            /* Erase sprite before cleanup */
            erase_letter_sprite(game->letters[i].sprite->x, game->letters[i].sprite->y, 16, 16);
            destroy_sprite(game->letters[i].sprite);
            game->letters[i].sprite = NULL;
        }
        game->letters[i].active = false;
        game->letters[i].last_x = -1;
        game->letters[i].last_y = -1;
    }
    
    // Cleanup board - erase it first
    if (game->board.sprite != NULL) {
        erase_letter_sprite(game->board.x, game->board.y, game->board.width, game->board.height);
        destroy_sprite(game->board.sprite);
        game->board.sprite = NULL;
    }
}
