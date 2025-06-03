#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "videocard.h"
#include "keyboard.h"
#include "mouse.h"
#include "font.h"
#include "leaderboard.h"
#include "game.h"
#include "utils.h"
#include "singleplayer.h"
#include "serial.h"

uint16_t mode;
uint8_t kbd_bit_no = 0;
uint8_t mouse_bit_no = 0;
uint8_t timer_bit_no = 0;

// Define some colors for different bit depths
#define COLOR_WHITE 0xFFFFFF
#define COLOR_RED   0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE  0x0000FF
#define COLOR_BLACK 0x000000

int main(int argc, char *argv[])
{
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/grupo_2leic10_2/proj/src/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/grupo_2leic10_2/proj/src/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // Run our main loop here, before lcf_cleanup
  int result = proj_main_loop(argc, argv);

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return result;
}

int (proj_main_loop)(int argc, char* argv[])
{ 
  /* Available modes:  */
  mode = 0x115;
  
  printf("=== Starting proj_main_loop ===\n");
  
  /* Initialize graphics mode */
  printf("Setting graphics mode...\n");
  if (set_graphics_mode(mode) != 0) {
    printf("Error setting graphics mode\n");
    return 1;
  }
  
  /* Map VRAM */
  printf("Mapping VRAM...\n");
  if (map_vram(mode) != 0) {
    printf("Error mapping VRAM\n");
    return 1;
  }
  
  printf("Graphics mode 0x%X initialized successfully\n", mode);
  printf("Resolution: %dx%d, %d bits per pixel\n", get_h_res(), get_v_res(), get_bits_per_pixel());
  
  /* Subscribe keyboard interrupts - ONLY ONCE */
  printf("Subscribing keyboard interrupts...\n");
  if (kbd_subscribe_int(&kbd_bit_no) != 0) {
    printf("Error subscribing keyboard interrupts\n");
    exit_graphics_mode();
    return 1;
  }
  printf("Keyboard subscribed successfully with bit_no=%d\n", kbd_bit_no);
  
  /* Subscribe mouse interrupts - ONLY ONCE */
  printf("Subscribing mouse interrupts...\n");
  if (mouse_enable(&mouse_bit_no) != 0) {
    printf("Error subscribing mouse interrupts\n");
    kbd_unsubscribe_int();
    exit_graphics_mode();
    return 1;
  }
  printf("Mouse subscribed successfully with bit_no=%d\n", mouse_bit_no);
  
  /* Subscribe timer interrupts - ONLY ONCE */
  printf("Subscribing timer interrupts...\n");
  if (timer_subscribe_int(&timer_bit_no) != 0) {
    printf("Error subscribing timer interrupts\n");
    mouse_disable();
    kbd_unsubscribe_int();
    exit_graphics_mode();
    return 1;
  }
  printf("Timer subscribed successfully with bit_no=%d\n", timer_bit_no);
  
  /* Initialize font system */
  font_init();
  
  /* Set initial state and draw the main page */
  set_game_state(STATE_MAIN_MENU);
  if (draw_current_page(400, 300) != 0) {
    printf("Error drawing initial page\n");
    goto cleanup_and_exit;
  }
  
  int ipc_status;
  message msg;
  bool running = true;
  
  /* Main loop */
  printf("=== Entering main loop ===\n");
  printf("Press ESC to exit graphics mode\n");
  
  while (running) {
    /* Wait for interrupt */
    if (driver_receive(ANY, &msg, &ipc_status) != 0) {
      printf("Error receiving message\n");
      continue;
    }
    
    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          /* Timer interrupt */
          if (msg.m_notify.interrupts & BIT(timer_bit_no)) {
            timer_int_handler();
            
            /* Handle countdown updates for single player mode */
            if (get_game_state() == STATE_SP_COUNTDOWN) {
              jogo_t *game = get_current_game();
              bool countdown_finished = game_update_countdown(game);
              
              if (countdown_finished) {
                printf("Countdown finished, starting letter rain...\n");
                int lr_init_result = game_start_letter_rain(game);
                
                if (lr_init_result != 0) {
                  printf("Error starting letter rain, using fallback\n");
                  game->state = GAME_STATE_LETTER_RAIN;
                }
                
                set_game_state(STATE_SP_LETTER_RAIN);
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              } else {
                /* Redraw countdown if number changed */
                static uint8_t last_countdown_value = 255;
                if (last_countdown_value != game->countdown) {
                  last_countdown_value = game->countdown;
                  uint16_t mouse_x = mouse_get_x();
                  uint16_t mouse_y = mouse_get_y();
                  draw_current_page(mouse_x, mouse_y);
                }
              }
            } 
            else if (get_game_state() == STATE_SP_ENTER_INITIALS) {
              /* Keep initials screen updated for automatic transitions */
              static int initials_redraw_counter = 0;
              initials_redraw_counter++;
              if (initials_redraw_counter >= 30) { /* Redraw every 0.5 seconds */
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
                initials_redraw_counter = 0;
              }
            }
            else if (get_game_state() == STATE_SP_LETTER_RAIN) {
              jogo_t *game = get_current_game();
              int lr_result = game_update_letter_rain(game);
              
              if (lr_result == 1) {
                /* Letter rain finished */
                if (game->letra != 0) {
                  printf("Letter rain finished, caught letter: %c\n", game->letra);
                  set_game_state(STATE_SP_PLAYING);
                } else {
                  printf("Letter rain failed\n");
                  reset_singleplayer();
                  set_game_state(STATE_MAIN_MENU);
                }
                
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              } else {
                /* Continue letter rain animation */
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              }
            }
            else if (get_game_state() == STATE_SP_PLAYING) {
              extern singleplayer_game_t sp_game;
              int sp_result = singleplayer_update(&sp_game);
              
              if (sp_result == 1) {
                /* Game finished */
                printf("Singleplayer game finished\n");
                
                /* Check if we're in multiplayer mode */
                if (is_in_multiplayer_mode()) {
                  printf("Multiplayer mode: transitioning to waiting screen\n");
                  set_game_state(STATE_MP_WAITING_FOR_OTHER_PLAYER);
                } else {
                  printf("Single player mode: staying in results state\n");
                  /* Note: score is saved automatically in singleplayer_draw_results */
                  /* Game will stay in finished state until ESC is pressed */
                }
              }
              
              /* Always redraw to update timer */
              uint16_t mouse_x = mouse_get_x();
              uint16_t mouse_y = mouse_get_y();
              draw_current_page(mouse_x, mouse_y);
            }
            else if (get_game_state() == STATE_MULTIPLAYER_TEST) {
              /* Update multiplayer connection test every timer tick */
              uint16_t mouse_x = mouse_get_x();
              uint16_t mouse_y = mouse_get_y();
              draw_current_page(mouse_x, mouse_y);
            }
            else if (get_game_state() == STATE_MP_WAITING_FOR_OTHER_PLAYER) {
              /* Update multiplayer waiting screen every timer tick */
              uint16_t mouse_x = mouse_get_x();
              uint16_t mouse_y = mouse_get_y();
              draw_current_page(mouse_x, mouse_y);
            }
            else if (get_game_state() == STATE_MP_RESULTS) {
              /* Update multiplayer results screen every timer tick */
              uint16_t mouse_x = mouse_get_x();
              uint16_t mouse_y = mouse_get_y();
              draw_current_page(mouse_x, mouse_y);
            }
          }
          
          /* Keyboard interrupt */
          if (msg.m_notify.interrupts & BIT(kbd_bit_no)) {
            kbd_int_handler();
            
            /* Handle ESC key */
            if (is_esc_key()) {
              game_state_t current = get_game_state();
              printf("ESC key pressed in state: %d\n", current);
              
              if (current == STATE_SP_ENTER_INITIALS || current == STATE_SP_COUNTDOWN || 
                  current == STATE_SP_LETTER_RAIN || current == STATE_SP_PLAYING) {
                /* Exit single player mode */
                printf("Exiting single player mode...\n");
                
                if (current == STATE_SP_LETTER_RAIN) {
                  jogo_t *game = get_current_game();
                  game_cleanup_letter_rain(game);
                }
                
                reset_singleplayer();
                set_game_state(STATE_MAIN_MENU);
                
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              } 
              else if (current == STATE_MULTIPLAYER_TEST || current == STATE_MP_WAITING_FOR_OTHER_PLAYER) {
                /* Exit multiplayer mode */
                printf("Exiting multiplayer mode...\n");
                serial_cleanup();
                reset_multiplayer_connection();
                set_game_state(STATE_MAIN_MENU);
                
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              }
              else if (current == STATE_MP_RESULTS) {
                /* Exit multiplayer results screen */
                printf("Exiting multiplayer results...\n");
                reset_multiplayer_connection();
                set_game_state(STATE_MAIN_MENU);
                
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              }
              else if (current != STATE_MAIN_MENU) {
                /* Go back to main menu */
                printf("Going back to main menu...\n");
                set_game_state(STATE_MAIN_MENU);
                
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              } 
              else {
                /* Exit application */
                printf("ESC key detected, exiting...\n");
                running = false;
              }
            }
            
            /* Handle keyboard input for different states */
            game_state_t current_state = get_game_state();
            
            if (current_state == STATE_SP_ENTER_INITIALS) {
              int kb_result = handle_initials_keyboard(last_scancode);
              
              if (kb_result == 1) {
                /* Initials confirmed */
                printf("Initials confirmed, starting countdown...\n");
                jogo_t *game = get_current_game();
                game_start_countdown(game);
                set_game_state(STATE_SP_COUNTDOWN);
              } else if (kb_result == 0) {
                /* Character changed, redraw */
                /* Redraw happens automatically */
              }
              
              /* Always redraw for initials page */
              uint16_t mouse_x = mouse_get_x();
              uint16_t mouse_y = mouse_get_y();
              draw_current_page(mouse_x, mouse_y);
            }
            else if (current_state == STATE_SP_COUNTDOWN && last_scancode == 0x1C) {
              /* Enter pressed during countdown - skip to letter rain */
              printf("Enter pressed, skipping countdown...\n");
              jogo_t *game = get_current_game();
              
              int lr_init_result = game_start_letter_rain(game);
              if (lr_init_result != 0) {
                game->state = GAME_STATE_LETTER_RAIN;
              }
              
              set_game_state(STATE_SP_LETTER_RAIN);
              uint16_t mouse_x = mouse_get_x();
              uint16_t mouse_y = mouse_get_y();
              draw_current_page(mouse_x, mouse_y);
            }
            else if (current_state == STATE_SP_LETTER_RAIN) {
              jogo_t *game = get_current_game();
              
              if (last_scancode == 0x1C) {
                /* Enter pressed - skip letter rain */
                printf("Enter pressed, skipping letter rain...\n");
                
                if (game->letra == 0) {
                  game->letra = 'A';
                  printf("Using default letter 'A'\n");
                }
                
                set_game_state(STATE_SP_PLAYING);
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                draw_current_page(mouse_x, mouse_y);
              } else {
                /* Normal letter rain input */
                game_handle_letter_rain_input(game, last_scancode);
              }
            }
            else if (current_state == STATE_SP_PLAYING) {
              extern singleplayer_game_t sp_game;
              singleplayer_handle_input(&sp_game, last_scancode);
              
              /* Redraw happens automatically via timer */
            }
          }
          
          /* Mouse interrupt */
          if (msg.m_notify.interrupts & BIT(mouse_bit_no)) {
            game_state_t current_state = get_game_state();
            
            /* Ignore mouse during active gameplay states */
            if (current_state == STATE_SP_COUNTDOWN || current_state == STATE_SP_LETTER_RAIN || 
                current_state == STATE_SP_PLAYING) {
              mouse_ih_custom();
              if (mouse_has_packet_ready()) {
                mouse_clear_packet_ready();
              }
            } else {
              /* Handle mouse normally */
              mouse_ih_custom();
              
              if (mouse_has_packet_ready()) {
                struct packet pp = mouse_get_packet();
                mouse_clear_packet_ready();
                
                /* Redraw if mouse moved */
                if (mouse_menu_needs_redraw() || mouse_should_redraw_page()) {
                  uint16_t mouse_x = mouse_get_x();
                  uint16_t mouse_y = mouse_get_y();
                  draw_current_page(mouse_x, mouse_y);
                  mouse_clear_redraw_flag();
                  mouse_clear_page_redraw_flag();
                }
                
                /* Handle left clicks */
                if (pp.lb) {
                  uint16_t mouse_x = mouse_get_x();
                  uint16_t mouse_y = mouse_get_y();
                  
                  if (current_state == STATE_MAIN_MENU) {
                    int click_result = handle_menu_click(mouse_x, mouse_y, true);
                    if (click_result == 1) {
                      /* Quit clicked */
                      running = false;
                    } else if (click_result == 0) {
                      /* State changed */
                      draw_current_page(mouse_x, mouse_y);
                    }
                  }
                  else if (current_state == STATE_LEADERBOARD) {
                    int click_result = handle_leaderboard_click(mouse_x, mouse_y, true);
                    if (click_result == 1) {
                      /* Back button clicked */
                      set_game_state(STATE_MAIN_MENU);
                      draw_current_page(mouse_x, mouse_y);
                    }
                  }
                  else if (current_state == STATE_INSTRUCTIONS) {
                    int click_result = handle_instructions_click(mouse_x, mouse_y, true);
                    if (click_result == 1) {
                      /* Back button clicked */
                      set_game_state(STATE_MAIN_MENU);
                      draw_current_page(mouse_x, mouse_y);
                    }
                  }
                  else if (current_state == STATE_SP_ENTER_INITIALS) {
                    int click_result = handle_initials_click(mouse_x, mouse_y, true);
                    if (click_result == 1) {
                      /* Done button clicked */
                      jogo_t *game = get_current_game();
                      if (game_validate_initials(game)) {
                        game_start_countdown(game);
                        set_game_state(STATE_SP_COUNTDOWN);
                        draw_current_page(mouse_x, mouse_y);
                      }
                    }
                  }
                }
              }
            }
          }
          break;
        default:
          break;
      }
    }
  }
  
cleanup_and_exit:
  printf("=== Cleaning up and exiting ===\n");
  
  /* Unsubscribe interrupts - ONLY ONCE at the end */
  printf("Unsubscribing timer interrupts...\n");
  timer_unsubscribe_int();
  
  printf("Unsubscribing mouse interrupts...\n");
  mouse_disable();
  
  printf("Unsubscribing keyboard interrupts...\n");
  kbd_unsubscribe_int();
  
  /* Exit graphics mode */
  printf("Exiting graphics mode...\n");
  exit_graphics_mode();
  
  printf("Program terminated successfully\n");
  return 0;
}
