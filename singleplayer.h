#ifndef _SINGLEPLAYER_H_
#define _SINGLEPLAYER_H_

#include <stdint.h>
#include <stdbool.h>
#include "dicionarios.h"

#define MAX_INPUT_LENGTH 30
#define MAX_ANSWERED_WORDS 50
#define GAME_TIME_SECONDS 35
#define TIMER_FREQUENCY 60  /* 60 Hz timer */

/* Single Player Game States */
typedef enum {
    SP_STATE_STARTING,     /* Just started, showing category */
    SP_STATE_PLAYING,      /* Active gameplay */
    SP_STATE_FINISHED,     /* Game over, showing results */
    SP_STATE_CLEANUP       /* Cleaning up resources */
} sp_game_state_t;

/* Single Player Game Structure */
typedef struct {
    /* Game State */
    sp_game_state_t state;
    bool game_active;
    
    /* Selected Category */
    Categoria *current_category;
    int category_index;
    
    /* Player Info */
    char player_initials[4];
    char caught_letter;  /* Letter from letter rain */
    
    /* Game Progress */
    char answered_words[MAX_ANSWERED_WORDS][MAX_TAMANHO_PALAVRA];
    int answered_count;
    int total_score;
    
    /* Current Input */
    char current_input[MAX_INPUT_LENGTH];
    int input_length;
    
    /* Timer */
    uint32_t timer_ticks;
    uint32_t remaining_seconds;
    
    /* UI State */
    bool show_cursor;
    uint32_t cursor_counter;
    
    /* Game Results */
    bool all_words_found;
    bool time_expired;
    
} singleplayer_game_t;

/**
 * @brief Initialize single player game
 * 
 * @param game Pointer to game structure
 * @param player_initials Player's initials from previous phase
 * @param caught_letter Letter caught in letter rain mini-game
 * @return 0 on success, non-zero otherwise
 */
int singleplayer_init(singleplayer_game_t *game, const char *player_initials, char caught_letter);

/**
 * @brief Update single player game logic (called every timer tick)
 * 
 * @param game Pointer to game structure
 * @return 0 if game continues, 1 if game finished
 */
int singleplayer_update(singleplayer_game_t *game);

/**
 * @brief Draw single player game screen
 * 
 * @param game Pointer to game structure
 * @return 0 on success, non-zero otherwise
 */
int singleplayer_draw(singleplayer_game_t *game);

/**
 * @brief Handle keyboard input for single player game
 * 
 * @param game Pointer to game structure
 * @param scancode Keyboard scancode
 * @return 0 on success, non-zero otherwise
 */
int singleplayer_handle_input(singleplayer_game_t *game, uint8_t scancode);

/**
 * @brief Check if a word is valid for the current category
 * 
 * @param game Pointer to game structure
 * @param word Word to check
 * @return Score (0 if invalid, positive if valid)
 */
int singleplayer_check_word(singleplayer_game_t *game, const char *word);

/**
 * @brief Add a character to current input
 * 
 * @param game Pointer to game structure
 * @param c Character to add
 * @return 0 on success, 1 if input is full
 */
int singleplayer_add_char(singleplayer_game_t *game, char c);

/**
 * @brief Remove last character from current input
 * 
 * @param game Pointer to game structure
 */
void singleplayer_remove_char(singleplayer_game_t *game);

/**
 * @brief Submit current input as a word
 * 
 * @param game Pointer to game structure
 * @return Score earned (0 if invalid word)
 */
int singleplayer_submit_word(singleplayer_game_t *game);

/**
 * @brief Check if all words in category have been found
 * 
 * @param game Pointer to game structure
 * @return True if all words found
 */
bool singleplayer_all_words_found(singleplayer_game_t *game);

/**
 * @brief Get time remaining in seconds
 * 
 * @param game Pointer to game structure
 * @return Seconds remaining
 */
uint32_t singleplayer_get_time_remaining(singleplayer_game_t *game);

/**
 * @brief Clean up single player game resources
 * 
 * @param game Pointer to game structure
 */
void singleplayer_cleanup(singleplayer_game_t *game);

/**
 * @brief Get a random category from the available categories
 * 
 * @return Pointer to randomly selected category
 */
Categoria* singleplayer_get_random_category();

/**
 * @brief Save final score to leaderboard
 * 
 * @param game Pointer to game structure
 */
void singleplayer_save_score(singleplayer_game_t *game);

/**
 * @brief Convert scancode to character (for Portuguese keyboard)
 * 
 * @param scancode Keyboard scancode
 * @return Character or 0 if not a valid letter
 */
char singleplayer_scancode_to_char(uint8_t scancode);

#endif /* _SINGLEPLAYER_H_ */
