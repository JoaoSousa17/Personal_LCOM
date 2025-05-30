#ifndef _GAME_H_
#define _GAME_H_

#include <stdint.h>
#include <stdbool.h>
#include "letter_rain.h"
#include "singleplayer.h"
#include "forca.h"

/* Game states */
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_SINGLE_PLAYER,
    GAME_STATE_MULTIPLAYER,
    GAME_STATE_INSTRUCTIONS,
    GAME_STATE_LETTER_RAIN,
    GAME_STATE_SINGLEPLAYER_GAME,
    GAME_STATE_FORCA  /* Novo estado para o minigame da forca */
} game_state_t;

/* Main game structure */
typedef struct {
    game_state_t state;
    letter_rain_t letter_rain_game;
    singleplayer_game_t singleplayer_game;
    forca_game_t forca_game;  /* Estado do minigame da forca */
    
    /* Additional game data */
    bool initialized;
    uint32_t frame_counter;
    char player_name[32];
    int high_score;
} jogo_t;

/**
 * @brief Initialize the main game
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_init(jogo_t *game);

/**
 * @brief Start the letter rain mini-game
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_start_letter_rain(jogo_t *game);

/**
 * @brief Start the singleplayer game
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_start_singleplayer(jogo_t *game);

/**
 * @brief Start the multiplayer game
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_start_multiplayer(jogo_t *game);

/**
 * @brief Start the forca mini-game (easter egg)
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_start_forca(jogo_t *game);

/**
 * @brief Update the current game state
 * 
 * @param game Pointer to game structure
 * @return 0 if game continues, non-zero if state should change
 */
int game_update(jogo_t *game);

/**
 * @brief Draw the current game state
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_draw(jogo_t *game);

/**
 * @brief Handle keyboard input for current game state
 * 
 * @param game Pointer to game structure
 * @param scancode Keyboard scancode
 * @return 0 on success, non-zero otherwise
 */
int game_handle_input(jogo_t *game, uint8_t scancode);

/**
 * @brief Handle mouse input for current game state
 * 
 * @param game Pointer to game structure
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 * @param left_button Left mouse button state
 * @param right_button Right mouse button state
 * @return 0 on success, non-zero otherwise
 */
int game_handle_mouse(jogo_t *game, uint16_t x, uint16_t y, bool left_button, bool right_button);

/**
 * @brief Reset game to initial state
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_reset(jogo_t *game);

/**
 * @brief Save game state
 * 
 * @param game Pointer to game structure
 * @param filename Filename to save to
 * @return 0 on success, non-zero otherwise
 */
int game_save(jogo_t *game, const char *filename);

/**
 * @brief Load game state
 * 
 * @param game Pointer to game structure
 * @param filename Filename to load from
 * @return 0 on success, non-zero otherwise
 */
int game_load(jogo_t *game, const char *filename);

/**
 * @brief Get current game state
 * 
 * @param game Pointer to game structure
 * @return Current game state
 */
game_state_t game_get_state(jogo_t *game);

/**
 * @brief Set game state
 * 
 * @param game Pointer to game structure
 * @param new_state New game state
 * @return 0 on success, non-zero otherwise
 */
int game_set_state(jogo_t *game, game_state_t new_state);

/**
 * @brief Check if game is initialized
 * 
 * @param game Pointer to game structure
 * @return true if initialized, false otherwise
 */
bool game_is_initialized(jogo_t *game);

/**
 * @brief Get player name
 * 
 * @param game Pointer to game structure
 * @return Pointer to player name string
 */
const char* game_get_player_name(jogo_t *game);

/**
 * @brief Set player name
 * 
 * @param game Pointer to game structure
 * @param name Player name
 * @return 0 on success, non-zero otherwise
 */
int game_set_player_name(jogo_t *game, const char *name);

/**
 * @brief Get high score
 * 
 * @param game Pointer to game structure
 * @return High score value
 */
int game_get_high_score(jogo_t *game);

/**
 * @brief Set high score
 * 
 * @param game Pointer to game structure
 * @param score New high score
 * @return 0 on success, non-zero otherwise
 */
int game_set_high_score(jogo_t *game, int score);

/**
 * @brief Cleanup game resources
 * 
 * @param game Pointer to game structure
 */
void game_cleanup(jogo_t *game);

#endif /* _GAME_H_ */
