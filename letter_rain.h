#ifndef _LETTER_RAIN_H_
#define _LETTER_RAIN_H_

#include <stdint.h>
#include <stdbool.h>
#include "sprite.h"

#define MAX_FALLING_LETTERS 10
#define BOARD_WIDTH 100
#define BOARD_HEIGHT 60
#define LETTER_FALL_SPEED 2
#define BOARD_SPEED 5

/**
 * @brief Letter probabilities (multiplied by 100 for integer math)
 */
typedef enum {
    PROB_A = 1463, PROB_B = 104, PROB_C = 388, PROB_D = 499,
    PROB_E = 1257, PROB_F = 102, PROB_G = 130, PROB_H = 128,
    PROB_I = 618, PROB_J = 40, PROB_K = 2, PROB_L = 278,
    PROB_M = 474, PROB_N = 505, PROB_O = 1073, PROB_P = 252,
    PROB_Q = 120, PROB_R = 653, PROB_S = 781, PROB_T = 434,
    PROB_U = 463, PROB_V = 167, PROB_W = 1, PROB_X = 21,
    PROB_Y = 1, PROB_Z = 47
} letter_probability_t;

/**
 * @brief Falling letter structure
 */
typedef struct {
    char letter;        // The letter character
    Sprite *sprite;     // Sprite for the letter
    bool active;        // Whether this letter is falling
    uint32_t color;     // Letter color
} falling_letter_t;

/**
 * @brief Board structure for catching letters
 */
typedef struct {
    int x, y;           // Board position
    int width, height;  // Board dimensions
    Sprite *sprite;     // Board sprite
    int missed_count;   // Number of letters that hit the board
} board_t;

/**
 * @brief Letter rain game state
 */
typedef struct {
    falling_letter_t letters[MAX_FALLING_LETTERS];
    board_t board;
    char caught_letter;     // The letter that was caught
    bool game_over;         // Game over flag
    uint32_t frame_counter; // For timing letter spawning
    uint32_t spawn_rate;    // Frames between letter spawns
} letter_rain_t;

/**
 * @brief Initialize the letter rain game
 * 
 * @param game Pointer to letter rain game structure
 * @return 0 on success, non-zero otherwise
 */
int letter_rain_init(letter_rain_t *game);

/**
 * @brief Update the letter rain game state
 * 
 * @param game Pointer to letter rain game structure
 * @return 0 if game continues, 1 if game over
 */
int letter_rain_update(letter_rain_t *game);

/**
 * @brief Draw the letter rain game
 * 
 * @param game Pointer to letter rain game structure
 * @return 0 on success, non-zero otherwise
 */
int letter_rain_draw(letter_rain_t *game);

/**
 * @brief Handle keyboard input for the board
 * 
 * @param game Pointer to letter rain game structure
 * @param scancode Keyboard scancode
 * @return 0 on success, non-zero otherwise
 */
int letter_rain_handle_input(letter_rain_t *game, uint8_t scancode);

/**
 * @brief Cleanup letter rain game resources
 * 
 * @param game Pointer to letter rain game structure
 */
void letter_rain_cleanup(letter_rain_t *game);

/**
 * @brief Get a random letter based on Portuguese frequency
 * 
 * @return Random letter character
 */
char get_random_letter();

/**
 * @brief Create a letter sprite for the given character
 * 
 * @param letter The letter character
 * @param x Initial X position
 * @param y Initial Y position
 * @return Pointer to created sprite or NULL on error
 */
Sprite *create_letter_sprite(char letter, int x, int y);

/**
 * @brief Create board sprite
 * 
 * @param x Initial X position
 * @param y Initial Y position
 * @return Pointer to created sprite or NULL on error
 */
Sprite *create_board_sprite(int x, int y);

/**
 * @brief Check if a falling letter collides with the board
 * 
 * @param letter Pointer to falling letter
 * @param board Pointer to board
 * @return true if collision detected, false otherwise
 */
bool check_letter_board_collision(falling_letter_t *letter, board_t *board);

#endif /* _LETTER_RAIN_H_ */
