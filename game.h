#ifndef _GAME_H_
#define _GAME_H_

#include <stdint.h>
#include <stdbool.h>
#include "letter_rain.h"
#include "fight_list.h"

/* Maximum length for player initials */
#define MAX_INITIALS 4

/* Game states for single player mode */
typedef enum {
  GAME_STATE_ENTER_INITIALS,
  GAME_STATE_COUNTDOWN,
  GAME_STATE_LETTER_RAIN,
  GAME_STATE_PLAYING,
  GAME_STATE_FINISHED
} single_player_state_t;

/* Game structure */
typedef struct {
  char nome[MAX_INITIALS];     /* Player initials (3 chars + null terminator) */
  char letra;                  /* Current letter from letter rain */
  int pontuacao;              /* Current score */
  single_player_state_t state; /* Current game state */
  uint8_t countdown;          /* Countdown timer (3, 2, 1) */
  uint32_t timer_counter;     /* Timer counter for countdown */
  letter_rain_t letter_rain_game; /* Letter rain mini-game */
} jogo_t;

/**
 * @brief Initialize a new game
 * 
 * @param game Pointer to game structure
 */
void game_init(jogo_t *game);

/**
 * @brief Get current game instance
 * 
 * @return Pointer to current game
 */
jogo_t* get_current_game();

/**
 * @brief Add a character to the player's initials
 * 
 * @param game Pointer to game structure
 * @param c Character to add
 * @return 0 on success, 1 if initials are full
 */
int game_add_initial(jogo_t *game, char c);

/**
 * @brief Remove last character from initials
 * 
 * @param game Pointer to game structure
 */
void game_remove_initial(jogo_t *game);

/**
 * @brief Validate initials (check if we have at least 1 character)
 * 
 * @param game Pointer to game structure
 * @return True if initials are valid
 */
bool game_validate_initials(jogo_t *game);

/**
 * @brief Start the countdown phase
 * 
 * @param game Pointer to game structure
 */
void game_start_countdown(jogo_t *game);

/**
 * @brief Update countdown timer
 * 
 * @param game Pointer to game structure
 * @return True if countdown finished
 */
bool game_update_countdown(jogo_t *game);

/**
 * @brief Draw the enter initials page
 * 
 * @param mouse_x Current mouse X position
 * @param mouse_y Current mouse Y position
 * @return 0 on success, non-zero otherwise
 */
int draw_enter_initials_page(uint16_t mouse_x, uint16_t mouse_y);

/**
 * @brief Draw the countdown page
 * 
 * @return 0 on success, non-zero otherwise
 */
int draw_countdown_page();

/**
 * @brief Handle mouse clicks on enter initials page
 * 
 * @param x Mouse x coordinate
 * @param y Mouse y coordinate
 * @param left_click True if left button was clicked
 * @return 1 if done button was clicked, -1 if no action
 */
int handle_initials_click(uint16_t x, uint16_t y, bool left_click);

/**
 * @brief Handle keyboard input for initials
 * 
 * @param scancode Keyboard scancode
 * @return 0 on success, 1 if enter was pressed (confirm), -1 if no action
 */
int handle_initials_keyboard(uint8_t scancode);

/**
 * @brief Start the letter rain mini-game
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_start_letter_rain(jogo_t *game);

/**
 * @brief Update letter rain mini-game
 * 
 * @param game Pointer to game structure
 * @return 0 if continuing, 1 if finished
 */
int game_update_letter_rain(jogo_t *game);

/**
 * @brief Draw letter rain mini-game
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int game_draw_letter_rain(jogo_t *game);

/**
 * @brief Handle keyboard input for letter rain
 * 
 * @param game Pointer to game structure
 * @param scancode Keyboard scancode
 * @return 0 on success, non-zero otherwise
 */
int game_handle_letter_rain_input(jogo_t *game, uint8_t scancode);

/**
 * @brief Cleanup letter rain resources
 * 
 * @param game Pointer to game structure
 */
void game_cleanup_letter_rain(jogo_t *game);

#endif /* _GAME_H_ */
