#include "game.h"
#include "letter_rain.h"
#include "singleplayer.h"
#include "forca.h"
#include <string.h>
#include <stdio.h>

int game_init(jogo_t *game) {
    if (game == NULL) {
        printf("game_init: game is NULL\n");
        return 1;
    }
    
    printf("Initializing main game structure...\n");
    
    /* Initialize game state */
    game->state = GAME_STATE_MENU;
    game->initialized = false;
    game->frame_counter = 0;
    game->high_score = 0;
    
    /* Initialize player name */
    strcpy(game->player_name, "Player");
    
    /* Initialize all sub-games to default states */
    memset(&game->letter_rain_game, 0, sizeof(letter_rain_t));
    memset(&game->singleplayer_game, 0, sizeof(singleplayer_game_t));
    memset(&game->forca_game, 0, sizeof(forca_game_t));
    
    /* Mark as initialized */
    game->initialized = true;
    
    printf("Main game initialized successfully\n");
    return 0;
}

int game_start_letter_rain(jogo_t *game) {
    if (game == NULL) {
        printf("game_start_letter_rain: game is NULL\n");
        return 1;
    }
    
    printf("Initializing letter rain game...\n");
    
    /* Cleanup previous state */
    letter_rain_cleanup(&game->letter_rain_game);
    
    /* Clear the letter rain game structure */
    memset(&game->letter_rain_game, 0, sizeof(letter_rain_t));
    
    /* Initialize the letter rain mini-game */
    int init_result = letter_rain_init(&game->letter_rain_game);
    
    if (init_result != 0) {
        printf("game_start_letter_rain: letter_rain_init failed with code %d\n", init_result);
        
        /* Create a minimal valid state to avoid crashes */
        game->letter_rain_game.caught_letter = 'A';
        game->letter_rain_game.game_over = false;
        game->letter_rain_game.frame_counter = 0;
        game->letter_rain_game.spawn_rate = 60;
        game->letter_rain_game.first_draw = true;
        
        printf("Using fallback letter rain state\n");
    }
    
    printf("Letter rain initialized successfully\n");
    game->state = GAME_STATE_LETTER_RAIN;
    
    return 0; /* Always return success to avoid critical failures */
}

int game_start_singleplayer(jogo_t *game) {
    if (game == NULL) {
        printf("game_start_singleplayer: game is NULL\n");
        return 1;
    }
    
    printf("Initializing singleplayer game...\n");
    
    /* Cleanup previous state */
    singleplayer_cleanup(&game->singleplayer_game);
    
    /* Clear the singleplayer game structure */
    memset(&game->singleplayer_game, 0, sizeof(singleplayer_game_t));
    
    /* Initialize singleplayer game */
    int init_result = singleplayer_init(&game->singleplayer_game);
    
    if (init_result != 0) {
        printf("game_start_singleplayer: singleplayer_init failed with code %d\n", init_result);
        return 1;
    }
    
    printf("Singleplayer game initialized successfully\n");
    game->state = GAME_STATE_SINGLEPLAYER_GAME;
    
    return 0;
}

int game_start_multiplayer(jogo_t *game) {
    if (game == NULL) {
        printf("game_start_multiplayer: game is NULL\n");
        return 1;
    }
    
    printf("Multiplayer mode not yet implemented\n");
    
    /* For now, just set the state */
    game->state = GAME_STATE_MULTIPLAYER;
    
    return 0;
}

int game_start_forca(jogo_t *game) {
    if (game == NULL) {
        printf("game_start_forca: game is NULL\n");
        return 1;
    }
    
    printf("Initializing forca easter egg game...\n");
    
    /* Cleanup previous state */
    forca_cleanup(&game->forca_game);
    
    /* Clear the forca game structure */
    memset(&game->forca_game, 0, sizeof(forca_game_t));
    
    /* Initialize the forca mini-game */
    int init_result = forca_init(&game->forca_game);
    
    if (init_result != 0) {
        printf("game_start_forca: forca_init failed with code %d\n", init_result);
        return 1;
    }
    
    printf("Forca game initialized successfully\n");
    game->state = GAME_STATE_FORCA;
    
    return 0;
}

int game_update(jogo_t *game) {
    if (game == NULL || !game->initialized) {
        return 1;
    }
    
    /* Increment frame counter */
    game->frame_counter++;
    
    /* Update based on current state */
    switch (game->state) {
        case GAME_STATE_LETTER_RAIN:
            return letter_rain_update(&game->letter_rain_game);
            
        case GAME_STATE_SINGLEPLAYER_GAME:
            return singleplayer_update(&game->singleplayer_game);
            
        case GAME_STATE_FORCA:
            return forca_update(&game->forca_game);
            
        case GAME_STATE_MENU:
        case GAME_STATE_SINGLE_PLAYER:
        case GAME_STATE_MULTIPLAYER:
        case GAME_STATE_INSTRUCTIONS:
        default:
            return 0; /* These states don't need updates */
    }
}

int game_draw(jogo_t *game) {
    if (game == NULL || !game->initialized) {
        return 1;
    }
    
    /* Draw based on current state */
    switch (game->state) {
        case GAME_STATE_LETTER_RAIN:
            return letter_rain_draw(&game->letter_rain_game);
            
        case GAME_STATE_SINGLEPLAYER_GAME:
            return singleplayer_draw(&game->singleplayer_game);
            
        case GAME_STATE_FORCA:
            return forca_draw(&game->forca_game);
            
        case GAME_STATE_MENU:
        case GAME_STATE_SINGLE_PLAYER:
        case GAME_STATE_MULTIPLAYER:
        case GAME_STATE_INSTRUCTIONS:
        default:
            return 0; /* These states are handled elsewhere */
    }
}

int game_handle_input(jogo_t *game, uint8_t scancode) {
    if (game == NULL || !game->initialized) {
        return 1;
    }
    
    /* Handle input based on current state */
    switch (game->state) {
        case GAME_STATE_LETTER_RAIN:
            return letter_rain_handle_input(&game->letter_rain_game, scancode);
            
        case GAME_STATE_SINGLEPLAYER_GAME:
            return singleplayer_handle_input(&game->singleplayer_game, scancode);
            
        case GAME_STATE_FORCA:
            return forca_handle_input(&game->forca_game, scancode);
            
        case GAME_STATE_MENU:
        case GAME_STATE_SINGLE_PLAYER:
        case GAME_STATE_MULTIPLAYER:
        case GAME_STATE_INSTRUCTIONS:
        default:
            return 0; /* These states handle input elsewhere */
    }
}

int game_handle_mouse(jogo_t *game, uint16_t x, uint16_t y, bool left_button, bool right_button) {
    if (game == NULL || !game->initialized) {
        return 1;
    }
    
    /* For now, mouse input is primarily handled in the main menu */
    /* Individual games can implement mouse support as needed */
    
    switch (game->state) {
        case GAME_STATE_MENU:
            /* Mouse handling for menu is done in mouse.c */
            break;
            
        case GAME_STATE_SINGLEPLAYER_GAME:
            /* Singleplayer games might use mouse in the future */
            break;
            
        case GAME_STATE_FORCA:
            /* Forca game is keyboard-only */
            break;
            
        default:
            break;
    }
    
    return 0;
}

int game_reset(jogo_t *game) {
    if (game == NULL) {
        return 1;
    }
    
    printf("Resetting game to initial state...\n");
    
    /* Cleanup all sub-games */
    game_cleanup(game);
    
    /* Reinitialize */
    return game_init(game);
}

int game_save(jogo_t *game, const char *filename) {
    if (game == NULL || filename == NULL) {
        return 1;
    }
    
    printf("Game save functionality not yet implemented\n");
    /* TODO: Implement game saving */
    
    return 0;
}

int game_load(jogo_t *game, const char *filename) {
    if (game == NULL || filename == NULL) {
        return 1;
    }
    
    printf("Game load functionality not yet implemented\n");
    /* TODO: Implement game loading */
    
    return 0;
}

game_state_t game_get_state(jogo_t *game) {
    if (game == NULL) {
        return GAME_STATE_MENU; /* Default state */
    }
    
    return game->state;
}

int game_set_state(jogo_t *game, game_state_t new_state) {
    if (game == NULL) {
        return 1;
    }
    
    printf("Setting game state from %d to %d\n", game->state, new_state);
    game->state = new_state;
    
    return 0;
}

bool game_is_initialized(jogo_t *game) {
    if (game == NULL) {
        return false;
    }
    
    return game->initialized;
}

const char* game_get_player_name(jogo_t *game) {
    if (game == NULL) {
        return "Unknown";
    }
    
    return game->player_name;
}

int game_set_player_name(jogo_t *game, const char *name) {
    if (game == NULL || name == NULL) {
        return 1;
    }
    
    /* Copy name with bounds checking */
    strncpy(game->player_name, name, sizeof(game->player_name) - 1);
    game->player_name[sizeof(game->player_name) - 1] = '\0'; /* Ensure null termination */
    
    printf("Player name set to: %s\n", game->player_name);
    
    return 0;
}

int game_get_high_score(jogo_t *game) {
    if (game == NULL) {
        return 0;
    }
    
    return game->high_score;
}

int game_set_high_score(jogo_t *game, int score) {
    if (game == NULL) {
        return 1;
    }
    
    if (score > game->high_score) {
        game->high_score = score;
        printf("New high score: %d\n", score);
    }
    
    return 0;
}

void game_cleanup(jogo_t *game) {
    if (game == NULL) {
        return;
    }
    
    printf("Cleaning up game resources...\n");
    
    /* Cleanup all sub-games */
    letter_rain_cleanup(&game->letter_rain_game);
    singleplayer_cleanup(&game->singleplayer_game);
    forca_cleanup(&game->forca_game);
    
    /* Reset game structure */
    memset(&game->letter_rain_game, 0, sizeof(letter_rain_t));
    memset(&game->singleplayer_game, 0, sizeof(singleplayer_game_t));
    memset(&game->forca_game, 0, sizeof(forca_game_t));
    
    /* Keep some persistent data */
    /* Don't reset player_name and high_score */
    
    game->frame_counter = 0;
    game->state = GAME_STATE_MENU;
    
    printf("Game cleanup completed\n");
}
