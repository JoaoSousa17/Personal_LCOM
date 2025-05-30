#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>

#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "videocard.h"
#include "game.h"
#include "singleplayer.h"

/* Game state management */
typedef enum {
    STATE_MAIN_MENU,
    STATE_SINGLE_PLAYER,
    STATE_MULTIPLAYER, 
    STATE_INSTRUCTIONS,
    STATE_SP_ENTER_INITIALS,
    STATE_SP_COUNTDOWN,
    STATE_SP_LETTER_RAIN,
    STATE_SP_PLAYING,
    STATE_FORCA  /* Novo estado para o minigame da forca */
} program_state_t;

/* Global state variables */
static program_state_t current_state = STATE_MAIN_MENU;
static jogo_t main_game;
static bool program_initialized = false;

/* Interrupt variables */
static int timer_irq_set, kbd_irq_set, mouse_irq_set;

/* Function prototypes */
void set_game_state(program_state_t new_state);
program_state_t get_game_state(void);
jogo_t* get_current_game(void);
int handle_state_transitions(void);
int draw_current_state(void);

/* Key scancodes */
#define ESC_BREAKCODE 0x81
#define ENTER_MAKE 0x1C

/* Timer frequency for 60 FPS */
#define TIMER_FREQUENCY 60

int main(int argc, char *argv[]) {
    /* Set language and logging */
    lcf_set_language("EN-US");
    lcf_trace_calls("/home/lcom/labs/proj/trace.txt");
    lcf_log_output("/home/lcom/labs/proj/output.txt");

    /* Start LCF framework */
    if (lcf_start(argc, argv)) {
        printf("Failed to start LCF framework\n");
        return 1;
    }

    /* Cleanup and exit */
    lcf_cleanup_and_exit(); 
    return 0;
}

int (proj_main_loop)(int argc, char *argv[]) {
    printf("=== Starting Minix Labs Project ===\n");
    printf("Initializing graphics and game systems...\n");

    /* Initialize video mode (1024x768, 16-bit color) */
    if (set_graphics_mode(0x117) != 0) {
        printf("ERROR: Failed to set graphics mode 0x117\n");
        return 1;
    }
    printf("Graphics mode 0x117 (1024x768) initialized successfully\n");

    /* Subscribe to interrupts - ORDER MATTERS! */
    uint8_t timer_bit, kbd_bit, mouse_bit;
    
    /* Timer subscription */
    if (timer_subscribe_int(&timer_bit) != 0) {
        printf("ERROR: Failed to subscribe timer interrupts\n");
        exit_graphics_mode();
        return 1;
    }
    printf("Timer interrupts subscribed successfully (bit %d)\n", timer_bit);
    
    /* Keyboard subscription */
    if (kbd_subscribe_int(&kbd_bit) != 0) {
        printf("ERROR: Failed to subscribe keyboard interrupts\n");
        timer_unsubscribe_int();
        exit_graphics_mode();
        return 1;
    }
    printf("Keyboard interrupts subscribed successfully (bit %d)\n", kbd_bit);
    
    /* Mouse subscription */
    if (mouse_subscribe_int(&mouse_bit) != 0) {
        printf("ERROR: Failed to subscribe mouse interrupts\n");
        kbd_unsubscribe_int();
        timer_unsubscribe_int();
        exit_graphics_mode();
        return 1;
    }
    printf("Mouse interrupts subscribed successfully (bit %d)\n", mouse_bit);

    /* Enable mouse data reporting */
    if (mouse_enable_data_reporting() != 0) {
        printf("WARNING: Failed to enable mouse data reporting\n");
    } else {
        printf("Mouse data reporting enabled\n");
    }

    /* Initialize main game structure */
    if (game_init(&main_game) != 0) {
        printf("ERROR: Failed to initialize main game structure\n");
        mouse_unsubscribe_int();
        kbd_unsubscribe_int();
        timer_unsubscribe_int();
        exit_graphics_mode();
        return 1;
    }
    printf("Main game structure initialized successfully\n");

    /* Set initial state and draw main menu */
    current_state = STATE_MAIN_MENU;
    program_initialized = true;
    
    if (draw_main_menu() != 0) {
        printf("WARNING: Failed to draw initial main menu\n");
    } else {
        printf("Main menu drawn successfully\n");
    }

    printf("=== Entering main game loop ===\n");

    /* Main game loop variables */
    int ipc_status;
    message msg;
    int running = 1;
    uint32_t frame_counter = 0;

    while (running) {
        /* Wait for interrupt */
        if (driver_receive(ANY, &msg, &ipc_status) != 0) {
            continue;
        }

        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    /* Timer interrupt - 60 FPS game loop */
                    if (msg.m_notify.interrupts & BIT(timer_bit)) {
                        timer_int_handler();
                        frame_counter++;
                        
                        /* Update game logic based on current state */
                        switch (current_state) {
                            case STATE_SP_LETTER_RAIN:
                            case STATE_SP_PLAYING:
                                /* Update singleplayer game */
                                if (game_update(&main_game) != 0) {
                                    /* Game finished, return to menu */
                                    printf("Singleplayer game finished, returning to menu\n");
                                    game_cleanup(&main_game);
                                    set_game_state(STATE_MAIN_MENU);
                                    if (draw_current_state() != 0) {
                                        printf("WARNING: Failed to draw state after game finish\n");
                                    }
                                } else {
                                    /* Continue game, redraw */
                                    if (game_draw(&main_game) != 0) {
                                        printf("WARNING: Failed to draw singleplayer game\n");
                                    }
                                }
                                break;
                                
                            case STATE_FORCA:
                                /* Update forca mini-game */
                                if (game_update(&main_game) != 0) {
                                    /* Forca game finished, return to menu */
                                    printf("Forca game finished, returning to menu\n");
                                    game_cleanup(&main_game);
                                    set_game_state(STATE_MAIN_MENU);
                                    if (draw_current_state() != 0) {
                                        printf("WARNING: Failed to draw state after forca finish\n");
                                    }
                                } else {
                                    /* Continue forca game, redraw */
                                    if (game_draw(&main_game) != 0) {
                                        printf("WARNING: Failed to draw forca game\n");
                                    }
                                }
                                break;
                                
                            case STATE_SP_COUNTDOWN:
                                /* Handle countdown in singleplayer */
                                if (singleplayer_update(&main_game.singleplayer_game) != 0) {
                                    /* Countdown finished, start letter rain */
                                    if (main_game.singleplayer_game.state == SP_STATE_LETTER_RAIN) {
                                        set_game_state(STATE_SP_LETTER_RAIN);
                                        if (game_start_letter_rain(&main_game) != 0) {
                                            printf("WARNING: Failed to start letter rain\n");
                                        }
                                    } else if (main_game.singleplayer_game.state == SP_STATE_PLAYING) {
                                        set_game_state(STATE_SP_PLAYING);
                                    }
                                }
                                if (singleplayer_draw(&main_game.singleplayer_game) != 0) {
                                    printf("WARNING: Failed to draw countdown\n");
                                }
                                break;
                                
                            default:
                                /* Other states don't need timer updates */
                                break;
                        }
                    }

                    /* Keyboard interrupt */
                    if (msg.m_notify.interrupts & BIT(kbd_bit)) {
                        uint8_t scancode;
                        if (kbd_read_scancode(&scancode) == 0) {
                            /* Handle ESC key globally */
                            if (scancode == ESC_BREAKCODE) {
                                switch (current_state) {
                                    case STATE_MAIN_MENU:
                                        printf("ESC pressed in main menu - exiting program\n");
                                        running = 0; /* Exit program */
                                        break;
                                        
                                    case STATE_FORCA:
                                        /* ESC exits forca game */
                                        printf("ESC pressed - exiting forca game\n");
                                        game_cleanup(&main_game);
                                        set_game_state(STATE_MAIN_MENU);
                                        if (draw_current_state() != 0) {
                                            printf("WARNING: Failed to draw menu after forca exit\n");
                                        }
                                        break;
                                        
                                    default:
                                        /* Return to main menu from any other state */
                                        printf("ESC pressed - returning to main menu from state %d\n", current_state);
                                        game_cleanup(&main_game);
                                        set_game_state(STATE_MAIN_MENU);
                                        if (draw_current_state() != 0) {
                                            printf("WARNING: Failed to draw menu after ESC\n");
                                        }
                                        break;
                                }
                                continue;
                            }

                            /* Handle ENTER key for state transitions */
                            if (scancode == ENTER_MAKE) {
                                switch (current_state) {
                                    case STATE_SP_COUNTDOWN:
                                        /* Skip countdown with ENTER */
                                        main_game.singleplayer_game.countdown_timer = 0;
                                        break;
                                        
                                    default:
                                        /* ENTER handled by individual games */
                                        break;
                                }
                            }

                            /* Handle state-specific keyboard input */
                            switch (current_state) {
                                case STATE_MAIN_MENU:
                                    /* Main menu keyboard navigation (if implemented) */
                                    break;
                                    
                                case STATE_SP_ENTER_INITIALS:
                                    /* Handle initials input */
                                    if (singleplayer_handle_input(&main_game.singleplayer_game, scancode) == 0) {
                                        /* Check if state changed to countdown */
                                        if (main_game.singleplayer_game.state == SP_STATE_COUNTDOWN) {
                                            set_game_state(STATE_SP_COUNTDOWN);
                                        }
                                    }
                                    /* Redraw initials screen */
                                    if (singleplayer_draw(&main_game.singleplayer_game) != 0) {
                                        printf("WARNING: Failed to draw initials screen\n");
                                    }
                                    break;
                                    
                                case STATE_SP_LETTER_RAIN:
                                case STATE_SP_PLAYING:
                                    /* Handle game input */
                                    game_handle_input(&main_game, scancode);
                                    break;
                                    
                                case STATE_FORCA:
                                    /* Handle forca input */
                                    game_handle_input(&main_game, scancode);
                                    /* Redraw forca game */
                                    if (game_draw(&main_game) != 0) {
                                        printf("WARNING: Failed to redraw forca after input\n");
                                    }
                                    break;
                                    
                                default:
                                    /* Other states may handle input differently */
                                    break;
                            }
                        }
                    }

                    /* Mouse interrupt */
                    if (msg.m_notify.interrupts & BIT(mouse_bit)) {
                        /* Only process mouse in menu states */
                        if (current_state == STATE_MAIN_MENU || 
                            current_state == STATE_INSTRUCTIONS ||
                            current_state == STATE_MULTIPLAYER) {
                            
                            if (mouse_int_handler() == 0) {
                                /* Check for menu clicks */
                                int menu_result = mouse_check_menu_click();
                                
                                if (menu_result == 1) {
                                    /* Quit selected */
                                    printf("Quit selected via mouse - exiting program\n");
                                    running = 0;
                                    
                                } else if (menu_result == 0) {
                                    /* Valid menu option selected */
                                    printf("Menu option selected, current state: %d\n", current_state);
                                    
                                    /* Handle state-specific drawing */
                                    if (mouse_should_redraw_page()) {
                                        if (handle_state_transitions() != 0) {
                                            printf("WARNING: State transition failed\n");
                                        }
                                        mouse_clear_page_redraw_flag();
                                    }
                                }
                            }
                        }
                    }
                    break;
                    
                default:
                    /* Ignore other message sources */
                    break;
            }
        }
    }

    /* Cleanup and exit */
    printf("=== Cleaning up and exiting ===\n");
    
    /* Cleanup game resources */
    if (program_initialized) {
        game_cleanup(&main_game);
        printf("Game resources cleaned up\n");
    }
    
    /* Disable mouse reporting */
    if (mouse_disable_data_reporting() != 0) {
        printf("WARNING: Failed to disable mouse data reporting\n");
    }
    
    /* Unsubscribe from interrupts in reverse order */
    if (mouse_unsubscribe_int() != 0) {
        printf("WARNING: Failed to unsubscribe mouse interrupts\n");
    } else {
        printf("Mouse interrupts unsubscribed\n");
    }
    
    if (kbd_unsubscribe_int() != 0) {
        printf("WARNING: Failed to unsubscribe keyboard interrupts\n");
    } else {
        printf("Keyboard interrupts unsubscribed\n");
    }
    
    if (timer_unsubscribe_int() != 0) {
        printf("WARNING: Failed to unsubscribe timer interrupts\n");
    } else {
        printf("Timer interrupts unsubscribed\n");
    }
    
    /* Exit graphics mode */
    if (exit_graphics_mode() != 0) {
        printf("WARNING: Failed to exit graphics mode cleanly\n");
    } else {
        printf("Graphics mode exited successfully\n");
    }
    
    printf("=== Program terminated successfully ===\n");
    return 0;
}

/* State management functions */
void set_game_state(program_state_t new_state) {
    if (new_state != current_state) {
        printf("State transition: %d -> %d\n", current_state, new_state);
        current_state = new_state;
    }
}

program_state_t get_game_state(void) {
    return current_state;
}

jogo_t* get_current_game(void) {
    return &main_game;
}

/* Handle state transitions and drawing */
int handle_state_transitions(void) {
    switch (current_state) {
        case STATE_SINGLE_PLAYER:
            printf("Transitioning to single player mode\n");
            if (draw_init_sp_game() != 0) {
                printf("ERROR: Failed to draw SP init screen\n");
                return 1;
            }
            /* Automatically transition to initials entry */
            if (game_start_singleplayer(&main_game) != 0) {
                printf("ERROR: Failed to start singleplayer game\n");
                set_game_state(STATE_MAIN_MENU);
                return 1;
            }
            set_game_state(STATE_SP_ENTER_INITIALS);
            break;
            
        case STATE_MULTIPLAYER:
            printf("Transitioning to multiplayer mode\n");
            if (draw_multiplayer_menu() != 0) {
                printf("ERROR: Failed to draw multiplayer menu\n");
                return 1;
            }
            break;
            
        case STATE_INSTRUCTIONS:
            printf("Transitioning to instructions\n");
            if (draw_instructions() != 0) {
                printf("ERROR: Failed to draw instructions\n");
                return 1;
            }
            break;
            
        case STATE_FORCA:
            printf("Transitioning to forca easter egg\n");
            if (game_start_forca(&main_game) != 0) {
                printf("ERROR: Failed to start forca game\n");
                set_game_state(STATE_MAIN_MENU);
                return 1;
            }
            if (game_draw(&main_game) != 0) {
                printf("ERROR: Failed to draw forca game\n");
                return 1;
            }
            break;
            
        case STATE_MAIN_MENU:
            printf("Returning to main menu\n");
            if (draw_main_menu() != 0) {
                printf("ERROR: Failed to draw main menu\n");
                return 1;
            }
            break;
            
        default:
            printf("WARNING: Unhandled state transition: %d\n", current_state);
            break;
    }
    
    return 0;
}

/* Draw current state */
int draw_current_state(void) {
    switch (current_state) {
        case STATE_MAIN_MENU:
            return draw_main_menu();
            
        case STATE_MULTIPLAYER:
            return draw_multiplayer_menu();
            
        case STATE_INSTRUCTIONS:
            return draw_instructions();
            
        case STATE_SP_ENTER_INITIALS:
        case STATE_SP_COUNTDOWN:
            return singleplayer_draw(&main_game.singleplayer_game);
            
        case STATE_SP_LETTER_RAIN:
        case STATE_SP_PLAYING:
        case STATE_FORCA:
            return game_draw(&main_game);
            
        default:
            printf("WARNING: No draw function for state %d\n", current_state);
            return 1;
    }
}
