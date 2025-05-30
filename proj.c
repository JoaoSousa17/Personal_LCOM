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
  //mode = 0x105;
  //mode = 0x110;
  mode = 0x115;
  //mode = 0x11A;
  //mode = 0x14C;
  /* ------------------ */
  /* 
  To choose a different mode just uncomment the mode you wish to use, comment the previous
  one and recompile the code.  
  */
  /* ------------------ */
  
  /* Initialize graphics mode */
  if (set_graphics_mode(mode) != 0) {
    printf("Error setting graphics mode\n");
    return 1;
  }
  
  /* Map VRAM */
  if (map_vram(mode) != 0) {
    printf("Error mapping VRAM\n");
    return 1;
  }
  
  printf("Graphics mode 0x%X initialized successfully\n", mode);
  printf("Resolution: %dx%d, %d bits per pixel\n", get_h_res(), get_v_res(), get_bits_per_pixel());
  
  /* Subscribe keyboard interrupts */
  if (kbd_subscribe_int(&kbd_bit_no) != 0) {
    printf("Error subscribing keyboard interrupts\n");
    exit_graphics_mode();
    return 1;
  }
  
  /* Subscribe mouse interrupts */
  if (mouse_enable(&mouse_bit_no) != 0) {
    printf("Error subscribing mouse interrupts\n");
    kbd_unsubscribe_int();
    exit_graphics_mode();
    return 1;
  }
  
  /* Subscribe timer interrupts */
  if (timer_subscribe_int(&timer_bit_no) != 0) {
    printf("Error subscribing timer interrupts\n");
    mouse_disable();
    kbd_unsubscribe_int();
    exit_graphics_mode();
    return 1;
  }
  
  /* Initialize font system */
  font_init();
  
  /* Set initial state and draw the main page */
  set_game_state(STATE_MAIN_MENU);
  if (draw_current_page(400, 300) != 0) {
    printf("Error drawing initial page\n");
    exit_graphics_mode();
    kbd_unsubscribe_int();
    return 1;
  }
  
  int ipc_status;
  message msg;
  bool running = true;
  
  /* Main loop - wait for ESC key to exit */
  printf("Press ESC to exit graphics mode\n");
  
  while (running) {
    /* Wait for interrupt */
    if (driver_receive(ANY, &msg, &ipc_status) != 0) {
      printf("Error receiving message\n");
      continue;
    }
    
    /* Check if it's a keyboard interrupt */
    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & BIT(timer_bit_no)) {
            /* Handle timer interrupt */
            timer_int_handler();
            
            /* Handle countdown updates for single player mode */
            if (get_game_state() == STATE_SP_COUNTDOWN) {
              jogo_t *game = get_current_game();
              bool countdown_finished = game_update_countdown(game);
              
              if (countdown_finished) {
                /* Countdown finished, move to letter rain state */
                printf("Countdown finished, starting letter rain...\n");
                
                /* Debug: Check game state before initialization */
                printf("Game state before letter rain init: %d\n", game->state);
                
                /* Primeiro inicializa o letter rain */
                int lr_init_result = game_start_letter_rain(game);
                printf("Letter rain init result: %d\n", lr_init_result);
                
                if (lr_init_result != 0) {
                  printf("Error starting letter rain mini-game (code: %d)\n", lr_init_result);
                  printf("Attempting to continue anyway...\n");
                  
                  /* Tenta inicializar manualmente em caso de erro */
                  game->state = GAME_STATE_LETTER_RAIN;
                  set_game_state(STATE_SP_LETTER_RAIN);
                  
                  printf("Forced state change to letter rain\n");
                } else {
                  /* Letter rain inicializado com sucesso */
                  printf("Letter rain initialized successfully\n");
                  set_game_state(STATE_SP_LETTER_RAIN);
                }
                
                /* Força redesenho da página */
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error drawing letter rain page\n");
                }
                
              } else {
                /* Ainda na contagem regressiva, mas pode ter mudado o número */
                /* Redesenha a página para mostrar a mudança */
                static uint8_t last_countdown_value = 255; /* Initialize to invalid value */
                if (last_countdown_value != game->countdown) {
                  last_countdown_value = game->countdown;
                  printf("Countdown: %d\n", game->countdown);
                  uint16_t mouse_x = mouse_get_x();
                  uint16_t mouse_y = mouse_get_y();
                  if (draw_current_page(mouse_x, mouse_y) != 0) {
                    printf("Error redrawing countdown page\n");
                  }
                }
              }
            }
            
            /* Handle letter rain updates */
            if (get_game_state() == STATE_SP_LETTER_RAIN) {
              jogo_t *game = get_current_game();
              int lr_result = game_update_letter_rain(game);
              if (lr_result == 1) {
                /* Letter rain finished */
                if (game->letra != 0) {
                  printf("Letter rain finished, caught letter: %c\n", game->letra);
                  set_game_state(STATE_SP_SINGLEPLAYER);  /* MUDANÇA: vai para singleplayer */
                } else {
                  printf("Letter rain failed, game over\n");
                  set_game_state(STATE_MAIN_MENU);
                }
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error drawing next page\n");
                }
              } else {
                /* Redraw letter rain a cada frame para animação */
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error redrawing letter rain page\n");
                }
              }
            }
            
            /* Handle singleplayer updates */
            if (get_game_state() == STATE_SP_SINGLEPLAYER) {
              jogo_t *game = get_current_game();
              int sp_result = game_update_singleplayer(game);
              if (sp_result == 1) {
                /* Singleplayer finished */
                printf("Singleplayer finished, final score: %d\n", game->pontuacao);
                set_game_state(STATE_MAIN_MENU);
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error drawing main menu\n");
                }
              } else {
                /* Redraw singleplayer page */
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error redrawing singleplayer page\n");
                }
              }
            }
          }
          
          if (msg.m_notify.interrupts & BIT(kbd_bit_no)) {
            /* Handle keyboard interrupt */
            kbd_int_handler();
            
            /* Check if ESC key was pressed */
            if (is_esc_key()) {
              game_state_t current = get_game_state();
              if (current == STATE_SP_ENTER_INITIALS || current == STATE_SP_COUNTDOWN || 
                  current == STATE_SP_LETTER_RAIN || current == STATE_SP_SINGLEPLAYER || 
                  current == STATE_SP_PLAYING) {
                /* In single player mode, ESC goes back to main menu */
                printf("Exiting single player mode...\n");
                
                /* Cleanup based on current state */
                jogo_t *game = get_current_game();
                if (current == STATE_SP_LETTER_RAIN) {
                  game_cleanup_letter_rain(game);
                } else if (current == STATE_SP_SINGLEPLAYER) {
                  game_cleanup_singleplayer(game);
                }
                
                set_game_state(STATE_MAIN_MENU);
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error drawing main menu\n");
                }
              } else if (current != STATE_MAIN_MENU) {
                /* Go back to main menu */
                printf("Going back to main menu...\n");
                set_game_state(STATE_MAIN_MENU);
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error drawing main menu\n");
                }
              } else {
                /* Exit application */
                printf("ESC key detected, exiting graphics mode...\n");
                running = false;
              }
            }
            
            /* Handle keyboard input for initials entry */
            if (get_game_state() == STATE_SP_ENTER_INITIALS) {
              int kb_result = handle_initials_keyboard(last_scancode);
              if (kb_result == 1) {
                /* Initials confirmed, start countdown */
                jogo_t *game = get_current_game();
                game_start_countdown(game);
                set_game_state(STATE_SP_COUNTDOWN);
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error drawing countdown page\n");
                }
              } else if (kb_result == 0) {
                /* Character added/removed, redraw */
                uint16_t mouse_x = mouse_get_x();
                uint16_t mouse_y = mouse_get_y();
                if (draw_current_page(mouse_x, mouse_y) != 0) {
                  printf("Error redrawing initials page\n");
                }
              }
            }
            
            /* Handle keyboard input for letter rain */
            if (get_game_state() == STATE_SP_LETTER_RAIN) {
              jogo_t *game = get_current_game();
              if (game_handle_letter_rain_input(game, last_scancode) != 0) {
                printf("Error handling letter rain input\n");
              }
            }
            
            /* Handle keyboard input for singleplayer */
            if (get_game_state() == STATE_SP_SINGLEPLAYER) {
              jogo_t *game = get_current_game();
              if (game_handle_singleplayer_input(game, last_scancode) != 0) {
                printf("Error handling singleplayer input\n");
              }
            }
          }
          
          if (msg.m_notify.interrupts & BIT(mouse_bit_no)) {
            /* Handle mouse interrupt only for certain states */
            game_state_t current_state = get_game_state();
            
            /* Ignore mouse during countdown, letter rain, singleplayer and playing */
            if (current_state == STATE_SP_COUNTDOWN || current_state == STATE_SP_LETTER_RAIN || 
                current_state == STATE_SP_SINGLEPLAYER || current_state == STATE_SP_PLAYING) {
              /* Just clear the mouse packet but don't process it */
              mouse_ih_custom();
              if (mouse_has_packet_ready()) {
                mouse_clear_packet_ready();
              }
            } else {
              /* Normal mouse processing for other states */
              mouse_ih_custom();
              
              /* Check if mouse packet is ready */
              if (mouse_has_packet_ready()) {
                struct packet pp = mouse_get_packet();
                mouse_clear_packet_ready();
                
                /* Redraw page if needed (mouse moved or state changed) */
                if (mouse_menu_needs_redraw() || mouse_should_redraw_page()) {
                  uint16_t mouse_x = mouse_get_x();
                  uint16_t mouse_y = mouse_get_y();
                  
                  if (draw_current_page(mouse_x, mouse_y) != 0) {
                    printf("Error redrawing page\n");
                  }
                  mouse_clear_redraw_flag();
                  mouse_clear_page_redraw_flag();
                }
                
                /* Handle left click based on current state */
                if (pp.lb) {
                  uint16_t mouse_x = mouse_get_x();
                  uint16_t mouse_y = mouse_get_y();
                  
                  if (get_game_state() == STATE_MAIN_MENU) {
                    int click_result = handle_menu_click(mouse_x, mouse_y, true);
                    if (click_result == 1) { // Quit was clicked
                      printf("Quit selected, exiting graphics mode...\n");
                      running = false;
                    } else if (click_result == 0) {
                      /* State changed, redraw page */
                      if (draw_current_page(mouse_x, mouse_y) != 0) {
                        printf("Error drawing new page\n");
                      }
                    }
                  } else if (get_game_state() == STATE_LEADERBOARD) {
                    int click_result = handle_leaderboard_click(mouse_x, mouse_y, true);
                    if (click_result == 1) { // Back button was clicked
                      printf("Back button clicked, returning to main menu...\n");
                      set_game_state(STATE_MAIN_MENU);
                      if (draw_current_page(mouse_x, mouse_y) != 0) {
                        printf("Error drawing main menu\n");
                      }
                    }
                  } else if (get_game_state() == STATE_INSTRUCTIONS) {
                    int click_result = handle_instructions_click(mouse_x, mouse_y, true);
                    if (click_result == 1) { // Back button was clicked
                      printf("Back button clicked, returning to main menu...\n");
                      set_game_state(STATE_MAIN_MENU);
                      if (draw_current_page(mouse_x, mouse_y) != 0) {
                        printf("Error drawing main menu\n");
                      }
                    }
                  } else if (get_game_state() == STATE_SP_ENTER_INITIALS) {
                    int click_result = handle_initials_click(mouse_x, mouse_y, true);
                    if (click_result == 1) { // Done button was clicked
                      printf("Done button clicked, starting countdown...\n");
                      jogo_t *game = get_current_game();
                      game_start_countdown(game);
                      set_game_state(STATE_SP_COUNTDOWN);
                      if (draw_current_page(mouse_x, mouse_y) != 0) {
                        printf("Error drawing countdown page\n");
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
  
  /* Unsubscribe timer interrupts */
  if (timer_unsubscribe_int() != 0) {
    printf("Error unsubscribing timer interrupts\n");
  }
  
  /* Unsubscribe mouse interrupts */
  if (mouse_disable() != 0) {
    printf("Error unsubscribing mouse interrupts\n");
  }
  
  /* Unsubscribe keyboard interrupts */
  if (kbd_unsubscribe_int() != 0) {
    printf("Error unsubscribing keyboard interrupts\n");
  }
  
  /* Exit graphics mode */
  if (exit_graphics_mode() != 0) {
    printf("Error exiting graphics mode\n");
    return 1;
  }
  
  printf("Graphics mode exited successfully\n");
  
  return 0;
}
