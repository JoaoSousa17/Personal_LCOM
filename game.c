#include "game.h"
#include "videocard.h"
#include "font.h"
#include "letter_rain.h"
#include "singleplayer.h"
#include <string.h>
#include <ctype.h>

/* Global game instance */
static jogo_t current_game;

/* Scancode definitions for keyboard input */
#define ENTER_MAKE 0x1C
#define BACKSPACE_MAKE 0x0E

/* Scancode to character mapping - more accurate */
static char scancode_to_char(uint8_t scancode) {
  /* First row: QWERTYUIOP */
  if (scancode >= 0x10 && scancode <= 0x19) {
    const char first_row[] = "QWERTYUIOP";
    return first_row[scancode - 0x10];
  }
  /* Second row: ASDFGHJKL */
  if (scancode >= 0x1E && scancode <= 0x26) {
    const char second_row[] = "ASDFGHJKL";
    return second_row[scancode - 0x1E];
  }
  /* Third row: ZXCVBNM */
  if (scancode >= 0x2C && scancode <= 0x32) {
    const char third_row[] = "ZXCVBNM";
    return third_row[scancode - 0x2C];
  }
  return 0; /* Invalid scancode */
}

void game_init(jogo_t *game) {
  memset(game->nome, 0, MAX_INITIALS);
  game->letra = 'A';
  game->pontuacao = 0;
  game->state = GAME_STATE_ENTER_INITIALS;
  game->countdown = 3;
  game->timer_counter = 0;
  
  /* Initialize letter rain game */
  memset(&game->letter_rain_game, 0, sizeof(letter_rain_t));
}

jogo_t* get_current_game() {
  return &current_game;
}

int game_add_initial(jogo_t *game, char c) {
  int len = strlen(game->nome);
  if (len >= MAX_INITIALS - 1) {
    return 1; /* Initials are full */
  }
  
  /* Convert to uppercase - cast to unsigned char first to avoid signed char issues */
  c = (char)toupper((unsigned char)c);
  
  /* Only allow letters */
  if (c >= 'A' && c <= 'Z') {
    game->nome[len] = c;
    game->nome[len + 1] = '\0';
    return 0;
  }
  
  return 1; /* Invalid character */
}

void game_remove_initial(jogo_t *game) {
  int len = strlen(game->nome);
  if (len > 0) {
    game->nome[len - 1] = '\0';
  }
}

bool game_validate_initials(jogo_t *game) {
  return strlen(game->nome) > 0;
}

void game_start_countdown(jogo_t *game) {
  game->state = GAME_STATE_COUNTDOWN;
  game->countdown = 3;
  game->timer_counter = 0;
}

bool game_update_countdown(jogo_t *game) {
  /* Increment timer counter */
  game->timer_counter++;
  
  /* Timer runs at 60Hz, so 60 interrupts = 1 second */
  if (game->timer_counter >= 60) {
    game->timer_counter = 0;
    
    if (game->countdown > 0) {
      game->countdown--;
      return false; /* Still counting down, number changed */
    } else {
      /* countdown reached 0, GO phase finished */
      return true; /* Countdown finished, ready to move to letter rain */
    }
  }
  
  return false; /* Countdown still running, no change */
}

int draw_enter_initials_page(uint16_t mouse_x, uint16_t mouse_y) {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
  uint32_t orange = 0xff6b35;        /* Orange for title */
  uint32_t text_white = 0xffffff;    /* White for text */
  uint32_t light_blue = 0x16537e;    /* Light blue for accent */
  uint32_t yellow = 0xffd700;        /* Yellow for input field */
  uint32_t green = 0x00ff88;         /* Green for done button */
  
  /* Variables for positioning */
  const char *title;
  uint8_t title_scale;
  uint16_t title_width, title_x, title_y;
  uint16_t line_width, line_x, line_y;
  const char *instr1, *instr2;
  uint16_t instr_x;
  uint16_t field_width, field_height, field_x, field_y;
  jogo_t *game;
  uint16_t text_x, text_y;
  static uint32_t cursor_counter = 0;
  uint16_t cursor_x;
  uint16_t button_width, button_height, button_x, button_y;
  bool button_hovered, button_enabled;
  uint32_t button_color, button_bg, text_color;
  const char *button_text;
  uint16_t btn_text_x, btn_text_y;
  const char *bottom_instr;
  uint16_t bottom_x, bottom_y;
  uint16_t corner_size;
  uint16_t i;
  uint8_t thickness;
  
  /* Clear screen with dark background */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  title = "INSIRA AS SUAS INICIAIS";
  title_scale = 3;
  title_width = strlen(title) * 8 * title_scale;
  title_x = (get_h_res() - title_width) / 2;
  title_y = 80;
  
  if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
  
  /* Draw decorative line under title */
  line_width = title_width + 40;
  line_x = (get_h_res() - line_width) / 2;
  line_y = title_y + title_scale * 8 + 15;
  
  for (i = 0; i < line_width; i++) {
    for (thickness = 0; thickness < 3; thickness++) {
      if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
    }
  }
  
  /* Draw instructions */
  instr1 = "Digite 1-3 letras para as suas iniciais";
  instr2 = "Use o teclado para escrever";
  instr_x = (get_h_res() - strlen(instr1) * 8 * 2) / 2;
  
  if (draw_string_scaled(instr_x, line_y + 60, instr1, text_white, 2) != 0) return 1;
  
  instr_x = (get_h_res() - strlen(instr2) * 8 * 1) / 2;
  if (draw_string_scaled(instr_x, line_y + 90, instr2, text_white, 1) != 0) return 1;
  
  /* Draw input field */
  field_width = 200;
  field_height = 60;
  field_x = (get_h_res() - field_width) / 2;
  field_y = line_y + 140;
  
  /* Input field background */
  if (draw_filled_rectangle(field_x, field_y, field_width, field_height, 0x2a2a4e) != 0) return 1;
  
  /* Input field border */
  if (draw_rectangle_border(field_x, field_y, field_width, field_height, yellow, 3) != 0) return 1;
  
  /* Draw current initials in the field */
  game = get_current_game();
  text_x = field_x + 10;
  text_y = field_y + 15;
  
  if (strlen(game->nome) > 0) {
    if (draw_string_scaled(text_x, text_y, game->nome, text_white, 3) != 0) return 1;
  }
  
  /* Draw cursor (blinking effect) */
  cursor_counter++;
  if ((cursor_counter / 30) % 2 == 0) { /* Blink every 30 frames */
    cursor_x = text_x + strlen(game->nome) * 8 * 3;
    if (draw_string_scaled(cursor_x, text_y, "_", yellow, 3) != 0) return 1;
  }
  
  /* Draw Done button */
  button_width = 120;
  button_height = 50;
  button_x = (get_h_res() - button_width) / 2;
  button_y = field_y + 100;
  
  /* Check if mouse is hovering over button */
  button_hovered = (mouse_x >= button_x && mouse_x <= button_x + button_width &&
                   mouse_y >= button_y && mouse_y <= button_y + button_height);
  
  /* Only enable button if we have valid initials */
  button_enabled = game_validate_initials(game);
  
  button_color = button_enabled ? (button_hovered ? 0x00cc66 : green) : 0x666666;
  button_bg = button_enabled ? (button_hovered ? 0x2a4a2e : 0x1a3a1e) : 0x2a2a2a;
  text_color = button_enabled ? text_white : 0x999999;
  
  /* Draw button background */
  if (draw_filled_rectangle(button_x, button_y, button_width, button_height, button_bg) != 0) return 1;
  
  /* Draw button border */
  if (draw_rectangle_border(button_x, button_y, button_width, button_height, button_color, 2) != 0) return 1;
  
  /* Draw button text */
  button_text = "FEITO!";
  btn_text_x = button_x + (button_width - strlen(button_text) * 8 * 2) / 2;
  btn_text_y = button_y + (button_height - 8 * 2) / 2;
  
  if (draw_string_scaled(btn_text_x, btn_text_y, button_text, text_color, 2) != 0) return 1;
  
  /* Draw instructions at bottom */
  bottom_instr = "Press ENTER ou clique em FEITO para continuar";
  bottom_x = (get_h_res() - strlen(bottom_instr) * 8 * 1) / 2;
  bottom_y = get_v_res() - 60;
  
  if (draw_string_scaled(bottom_x, bottom_y, bottom_instr, text_white, 1) != 0) return 1;
  
  /* Add decorative corners */
  corner_size = 30;
  
  /* Top-left corner */
  if (draw_rectangle_border(20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Top-right corner */
  if (draw_rectangle_border(get_h_res() - corner_size - 20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-left corner */
  if (draw_rectangle_border(20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-right corner */
  if (draw_rectangle_border(get_h_res() - corner_size - 20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Draw mouse cursor */
  if (draw_mouse_cursor(mouse_x, mouse_y, 0xffffff) != 0) return 1;
  
  return 0;
}

int draw_countdown_page() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
  uint32_t orange = 0xff6b35;        /* Orange for title */
  uint32_t light_blue = 0x16537e;    /* Light blue for accent */
  uint32_t red = 0xff4444;           /* Red for countdown */
  
  /* Variables for positioning */
  const char *title;
  uint8_t title_scale;
  uint16_t title_width, title_x, title_y;
  uint16_t line_width, line_x, line_y;
  jogo_t *game;
  char countdown_str[2];
  uint8_t countdown_scale;
  uint16_t countdown_width, countdown_x, countdown_y;
  const char *go_text;
  uint8_t go_scale;
  uint16_t go_width, go_x, go_y;
  uint16_t corner_size;
  uint16_t i;
  uint8_t thickness;
  
  /* Clear screen with dark background */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  title = "PREPARE-SE PARA JOGAR...";
  title_scale = 3;
  title_width = strlen(title) * 8 * title_scale;
  title_x = (get_h_res() - title_width) / 2;
  title_y = 150;
  
  if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
  
  /* Draw decorative line under title */
  line_width = title_width + 40;
  line_x = (get_h_res() - line_width) / 2;
  line_y = title_y + title_scale * 8 + 15;
  
  for (i = 0; i < line_width; i++) {
    for (thickness = 0; thickness < 3; thickness++) {
      if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
    }
  }
  
  /* Draw countdown number */
  game = get_current_game();
  if (game->countdown > 0) {
    countdown_str[0] = '0' + game->countdown;
    countdown_str[1] = '\0';
    
    countdown_scale = 8;
    countdown_width = 8 * countdown_scale;
    countdown_x = (get_h_res() - countdown_width) / 2;
    countdown_y = line_y + 100;
    
    if (draw_string_scaled(countdown_x, countdown_y, countdown_str, red, countdown_scale) != 0) return 1;
  } else {
    /* Show "GO!" when countdown reaches 0 */
    go_text = "GO!";
    go_scale = 6;
    go_width = strlen(go_text) * 8 * go_scale;
    go_x = (get_h_res() - go_width) / 2;
    go_y = line_y + 100;
    
    if (draw_string_scaled(go_x, go_y, go_text, 0x00ff00, go_scale) != 0) return 1; /* Green for GO */
  }
  
  /* Add some decorative elements */
  corner_size = 30;
  
  /* Top-left corner */
  if (draw_rectangle_border(20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Top-right corner */
  if (draw_rectangle_border(get_h_res() - corner_size - 20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-left corner */
  if (draw_rectangle_border(20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-right corner */
  if (draw_rectangle_border(get_h_res() - corner_size - 20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  return 0;
}

int handle_initials_click(uint16_t x, uint16_t y, bool left_click) {
  jogo_t *game;
  uint16_t button_width, button_height, button_x, button_y;
  
  if (!left_click) return -1;  /* Only handle left clicks */
  
  game = get_current_game();
  
  /* Check if initials are valid before allowing button click */
  if (!game_validate_initials(game)) {
    return -1; /* Button is disabled */
  }
  
  /* Calculate button position (same as in draw function) */
  button_width = 120;
  button_height = 50;
  button_x = (get_h_res() - button_width) / 2;
  button_y = 80 + 3 * 8 + 15 + 140 + 100; /* title_y + title_height + line spacing + field offset + button offset */
  
  if (x >= button_x && x <= button_x + button_width &&
      y >= button_y && y <= button_y + button_height) {
    return 1; /* Done button clicked */
  }
  
  return -1; /* No action */
}

int handle_initials_keyboard(uint8_t scancode) {
  jogo_t *game = get_current_game();
  char c;
  
  /* Handle ENTER key */
  if (scancode == ENTER_MAKE) {
    if (game_validate_initials(game)) {
      return 1; /* Confirm initials */
    }
    return -1; /* Invalid initials */
  }
  
  /* Handle BACKSPACE */
  if (scancode == BACKSPACE_MAKE) {
    game_remove_initial(game);
    return 0; /* Character removed */
  }
  
  /* Handle letter keys */
  c = scancode_to_char(scancode);
  if (c != 0) {
    if (game_add_initial(game, c) == 0) {
      return 0; /* Character added */
    }
  }
  
  return -1; /* No action */
}

int game_start_letter_rain(jogo_t *game) {
  if (game == NULL) {
    printf("game_start_letter_rain: game is NULL\n");
    return 1;
  }
  
  printf("Initializing letter rain game...\n");
  printf("Game state before: %d\n", game->state);
  
  /* Limpa o estado anterior do letter rain game */
  memset(&game->letter_rain_game, 0, sizeof(letter_rain_t));
  
  /* Initialize the letter rain mini-game */
  int init_result = letter_rain_init(&game->letter_rain_game);
  printf("letter_rain_init returned: %d\n", init_result);
  
  if (init_result != 0) {
    printf("game_start_letter_rain: letter_rain_init failed with code %d\n", init_result);
    
    /* Tenta uma segunda inicialização */
    printf("Attempting second initialization...\n");
    memset(&game->letter_rain_game, 0, sizeof(letter_rain_t));
    init_result = letter_rain_init(&game->letter_rain_game);
    
    if (init_result != 0) {
      printf("Second initialization also failed with code %d\n", init_result);
      return 1;
    }
  }
  
  printf("Letter rain initialized successfully\n");
  game->state = GAME_STATE_LETTER_RAIN;
  printf("Game state after: %d\n", game->state);
  
  return 0;
}

int game_update_letter_rain(jogo_t *game) {
  if (game == NULL || game->state != GAME_STATE_LETTER_RAIN)
    return 1;
  
  /* Update the letter rain game */
  int result = letter_rain_update(&game->letter_rain_game);
  
  if (result == 1) {
    /* Letter rain game finished */
    if (game->letter_rain_game.caught_letter != 0) {
      /* A letter was caught - transition to singleplayer IMMEDIATELY */
      game->letra = game->letter_rain_game.caught_letter;
      
      printf("Letter caught: %c, transitioning to singleplayer...\n", game->letra);
      
      // Cleanup letter rain FIRST
      game_cleanup_letter_rain(game);
      
      // Initialize singleplayer game directly
      if (singleplayer_init(&game->singleplayer_game, game->nome, game->letra) == 0) {
        game->state = GAME_STATE_SINGLEPLAYER;  // Set internal state
        printf("Singleplayer initialized and state set to SINGLEPLAYER\n");
        return 1; /* Signal transition complete */
      } else {
        printf("Error initializing singleplayer, going to finished state\n");
        game->state = GAME_STATE_FINISHED;
        return 1;
      }
    } else {
      /* Game over (no letter caught) */
      printf("No letter caught, game over\n");
      game_cleanup_letter_rain(game);
      game->state = GAME_STATE_FINISHED;
      return 1;
    }
  }
  
  return 0; /* Continue letter rain */
}

int game_draw_letter_rain(jogo_t *game) {
  if (game == NULL || game->state != GAME_STATE_LETTER_RAIN)
    return 1;
  
  /* Draw the letter rain game */
  if (letter_rain_draw(&game->letter_rain_game) != 0)
    return 1;
  
  /* Draw game info */
  char info[100];
  sprintf(info, "Jogador: %s", game->nome);
  if (draw_string_scaled(get_h_res() - 200, 20, info, 0xFFD700, 2) != 0)
    return 1;
  
  return 0;
}

int game_handle_letter_rain_input(jogo_t *game, uint8_t scancode) {
  if (game == NULL || game->state != GAME_STATE_LETTER_RAIN)
    return 1;
  
  /* Pass input to letter rain handler */
  return letter_rain_handle_input(&game->letter_rain_game, scancode);
}

void game_cleanup_letter_rain(jogo_t *game) {
  if (game == NULL)
    return;
  
  /* Cleanup letter rain resources */
  letter_rain_cleanup(&game->letter_rain_game);
}

int game_start_singleplayer(jogo_t *game) {
  if (game == NULL) {
    return 1;
  }
  
  printf("Starting singleplayer game with initials=%s, letter=%c\n", 
         game->nome, game->letra);
  
  // Initialize singleplayer game
  if (singleplayer_init(&game->singleplayer_game, game->nome, game->letra) != 0) {
    printf("Failed to initialize singleplayer game\n");
    return 1;
  }
  
  game->state = GAME_STATE_SINGLEPLAYER;
  printf("Game state set to SINGLEPLAYER\n");
  return 0;
}

int game_update_singleplayer(jogo_t *game) {
  if (game == NULL || game->state != GAME_STATE_SINGLEPLAYER)
    return 1;
  
  /* Update singleplayer game */
  int result = singleplayer_update(&game->singleplayer_game);
  
  if (result == 1) {
    /* Singleplayer game finished */
    game->pontuacao = game->singleplayer_game.total_score;
    game->state = GAME_STATE_FINISHED;
    printf("Singleplayer finished with score: %d\n", game->pontuacao);
    return 1; /* Game finished */
  }
  
  return 0; /* Continue singleplayer */
}

int game_draw_singleplayer(jogo_t *game) {
  if (game == NULL || game->state != GAME_STATE_SINGLEPLAYER)
    return 1;
  
  /* Draw singleplayer game */
  return singleplayer_draw(&game->singleplayer_game);
}

int game_handle_singleplayer_input(jogo_t *game, uint8_t scancode) {
  if (game == NULL || game->state != GAME_STATE_SINGLEPLAYER)
    return 1;
  
  /* Pass input to singleplayer handler */
  return singleplayer_handle_input(&game->singleplayer_game, scancode);
}

void game_cleanup_singleplayer(jogo_t *game) {
  if (game == NULL)
    return;
  
  /* Cleanup singleplayer resources */
  singleplayer_cleanup(&game->singleplayer_game);
}
