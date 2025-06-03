#include "videocard.h"
#include "font.h"
#include "leaderboard.h"
#include "letter_rain.h"
#include "game.h"
#include "singleplayer.h"
#include "keyboard.h"
#include "serial.h"
#include <machine/int86.h>
#include <lcom/vbe.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

void *video_mem;         /* Process (virtual) address to which VRAM is mapped */
static vbe_mode_info_t vmi_p;   /* VBE mode information */
static uint16_t h_res;          /* Horizontal resolution */
static uint16_t v_res;          /* Vertical resolution */
static uint8_t bits_per_pixel;  /* Bits per pixel */
static uint8_t bytes_per_pixel; /* Bytes per pixel */
static uint8_t red_mask_size, green_mask_size, blue_mask_size;
static uint8_t red_field_position, green_field_position, blue_field_position;

/* Game state management */
static game_state_t current_state = STATE_MAIN_MENU;
static bool is_multiplayer_mode = false;

/* Singleplayer game instance */
static bool sp_initialized = false;
singleplayer_game_t sp_game;

/* Multiplayer connection state */
static bool mp_test_initialized = false;
static int mp_test_counter = 0;
static bool mp_received_ping = false;
static int mp_connection_timer = 0;
static uint32_t mp_session_id = 0; /* Unique identifier for each multiplayer session */
static uint32_t mp_last_sent_time = 0; /* Track when we last sent a message to ignore loopback */
static char mp_vm_id = 'A'; /* This VM's identifier (A or B) */

/* Multiplayer results data */
static char mp_other_player_initials[4] = {0}; /* Other player's initials */
static int mp_other_player_score = 0; /* Other player's score */
static bool mp_results_ready = false; /* Both scores received and ready to show results */

int map_vram(uint16_t mode) {
  struct minix_mem_range mr;
  unsigned int vram_base;  /* VRAM's physical address */
  unsigned int vram_size;  /* VRAM's size */

  /* Initialize vbe_mode_info_t struct */
  if (vbe_get_mode_info(mode, &vmi_p) != OK) {
    printf("map_vram(): vbe_get_mode_info() failed\n");
    return 1;
  }

  /* Store relevant mode information */
  h_res = vmi_p.XResolution;
  v_res = vmi_p.YResolution;
  bits_per_pixel = vmi_p.BitsPerPixel;
  bytes_per_pixel = (bits_per_pixel + 7) / 8;
  
  red_mask_size = vmi_p.RedMaskSize;
  green_mask_size = vmi_p.GreenMaskSize;
  blue_mask_size = vmi_p.BlueMaskSize;
  
  red_field_position = vmi_p.RedFieldPosition;
  green_field_position = vmi_p.GreenFieldPosition;
  blue_field_position = vmi_p.BlueFieldPosition;

  /* Calculate VRAM physical address and size */
  vram_base = vmi_p.PhysBasePtr;
  vram_size = h_res * v_res * bytes_per_pixel;

  /* Allow memory mapping */
  mr.mr_base = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + vram_size;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != OK) {
    printf("map_vram(): sys_privctl (ADD_MEM) failed\n");
    return 1;
  }

  /* Map memory */
  video_mem = vm_map_phys(SELF, (void *)mr.mr_base, vram_size);

  if (video_mem == MAP_FAILED) {
    printf("map_vram(): vm_map_phys() failed\n");
    return 1;
  }

  return 0;
}

int set_graphics_mode(uint16_t mode) {
  reg86_t reg86;

  memset(&reg86, 0, sizeof(reg86));
  reg86.intno = 0x10;
  reg86.ah = 0x4F;
  reg86.al = 0x02;
  reg86.bx = (1 << 14) | mode;  /* Set bit 14 to use linear framebuffer */

  if (sys_int86(&reg86) != OK) {
    printf("set_graphics_mode(): sys_int86() failed\n");
    return 1;
  }

  return 0;
}

int exit_graphics_mode() {
  reg86_t reg86;

  memset(&reg86, 0, sizeof(reg86));
  reg86.intno = 0x10;
  reg86.ah = 0x00;
  reg86.al = 0x03;  /* Standard text mode (80x25) */

  if (sys_int86(&reg86) != OK) {
    printf("exit_graphics_mode(): sys_int86() failed\n");
    return 1;
  }

  /* Unmap VRAM if it was mapped */
  if (video_mem != NULL) {
    uint32_t vram_size = h_res * v_res * bytes_per_pixel;
    vm_unmap_phys(SELF, video_mem, vram_size);
    video_mem = NULL;
  }

  /* NÃO desinscrever as interrupções aqui - isso é feito no proj.c */
  /* REMOVIDA a linha: kbd_unsubscribe_int(); */
  
  printf("Graphics mode exited successfully\n");
  
  return 0;
}

uint16_t get_h_res() {
  return h_res;
}

uint16_t get_v_res() {
  return v_res;
}

uint8_t get_bits_per_pixel() {
  return bits_per_pixel;
}

vbe_mode_info_t *get_vmi_p() {
  return &vmi_p;
}

void* get_video_mem() {
  return video_mem;
}

int draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= h_res || y >= v_res) {
    return 1; /* Out of bounds */
  }
  
  uint8_t *pixel_ptr = (uint8_t *)video_mem + (y * h_res + x) * bytes_per_pixel;
  
  /* Write color based on bits per pixel */
  switch (bits_per_pixel) {
    case 8:
      *pixel_ptr = (uint8_t)color;
      break;
    case 15:
    case 16:
      *(uint16_t *)pixel_ptr = (uint16_t)color;
      break;
    case 24:
      pixel_ptr[0] = (uint8_t)(color & 0xFF);       /* Blue */
      pixel_ptr[1] = (uint8_t)((color >> 8) & 0xFF);  /* Green */
      pixel_ptr[2] = (uint8_t)((color >> 16) & 0xFF); /* Red */
      break;
    case 32:
      *(uint32_t *)pixel_ptr = color;
      break;
    default:
      return 1; /* Unsupported color depth */
  }
  
  return 0;
}

int clear_screen(uint32_t color) {
  for (uint16_t y = 0; y < v_res; y++) {
    for (uint16_t x = 0; x < h_res; x++) {
      if (draw_pixel(x, y, color) != 0) {
        return 1;
      }
    }
  }
  
  return 0;
}

int draw_rectangle_border(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color, uint8_t thickness) {
  /* Draw top and bottom borders */
  for (uint8_t t = 0; t < thickness; t++) {
    for (uint16_t i = 0; i < width; i++) {
      /* Top border */
      if (draw_pixel(x + i, y + t, color) != 0) return 1;
      /* Bottom border */
      if (draw_pixel(x + i, y + height - 1 - t, color) != 0) return 1;
    }
  }
  
  /* Draw left and right borders */
  for (uint8_t t = 0; t < thickness; t++) {
    for (uint16_t i = 0; i < height; i++) {
      /* Left border */
      if (draw_pixel(x + t, y + i, color) != 0) return 1;
      /* Right border */
      if (draw_pixel(x + width - 1 - t, y + i, color) != 0) return 1;
    }
  }
  
  return 0;
}

int draw_menu_option(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                    const char *text, uint32_t text_color, uint32_t border_color, uint8_t scale) {
  /* Draw border */
  if (draw_rectangle_border(x, y, width, height, border_color, 2) != 0) {
    return 1;
  }
  
  /* Calculate text position to center it */
  uint16_t text_width = strlen(text) * 8 * scale;
  uint16_t text_height = 8 * scale;
  
  uint16_t text_x = x + (width - text_width) / 2;
  uint16_t text_y = y + (height - text_height) / 2;
  
  /* Draw text */
  if (draw_string_scaled(text_x, text_y, text, text_color, scale) != 0) {
    return 1;
  }
  
  return 0;
}

int draw_main_page() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
  uint32_t orange = 0xff6b35;        /* Orange for title */
  uint32_t yellow = 0xffd700;        /* Yellow for borders */
  uint32_t white = 0xffffff;         /* White for text */
  uint32_t light_blue = 0x16537e;    /* Light blue for accent */
  
  /* Clear screen with dark background */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title "Fight List" centered at top */
  const char *title = "FIGHT LIST";
  uint8_t title_scale = 4;
  uint16_t title_width = strlen(title) * 8 * title_scale;
  uint16_t title_x = (h_res - title_width) / 2;
  uint16_t title_y = 40;
  
  if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
  
  /* Draw decorative line under title */
  uint16_t line_width = title_width + 40;
  uint16_t line_x = (h_res - line_width) / 2;
  uint16_t line_y = title_y + title_scale * 8 + 15;
  
  for (uint16_t i = 0; i < line_width; i++) {
    for (uint8_t thickness = 0; thickness < 3; thickness++) {
      if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
    }
  }
  
  /* Menu options dimensions */
  uint16_t option_width = 280;
  uint16_t option_height = 60;
  uint16_t spacing = 20;
  
  /* Calculate starting positions */
  uint16_t start_y = line_y + 60;
  uint16_t center_x = h_res / 2;
  
  /* First row: Single Player and 2 Player (side by side) */
  uint16_t row1_y = start_y;
  uint16_t single_x = center_x - option_width - spacing/2;
  uint16_t multi_x = center_x + spacing/2;
  
  if (draw_menu_option(single_x, row1_y, option_width, option_height, 
                      "Single Player", white, yellow, 2) != 0) return 1;
  
  if (draw_menu_option(multi_x, row1_y, option_width, option_height, 
                      "2 Player", white, yellow, 2) != 0) return 1;
  
  /* Second row: Leaderboard and Instructions (side by side) */
  uint16_t row2_y = row1_y + option_height + spacing + 10;
  
  if (draw_menu_option(single_x, row2_y, option_width, option_height, 
                      "Leaderboard", white, yellow, 2) != 0) return 1;
  
  if (draw_menu_option(multi_x, row2_y, option_width, option_height, 
                      "Instructions", white, yellow, 2) != 0) return 1;
  
  /* Third row: Quit (full width) */
  uint16_t row3_y = row2_y + option_height + spacing + 20;
  uint16_t quit_width = option_width * 2 + spacing;
  uint16_t quit_x = center_x - quit_width/2;
  
  if (draw_menu_option(quit_x, row3_y, quit_width, option_height, 
                      "QUIT", white, 0xff4444, 3) != 0) return 1; /* Red border for quit */
  
  /* Add some decorative elements */
  /* Corner decorations */
  uint16_t corner_size = 30;
  
  /* Top-left corner */
  if (draw_rectangle_border(20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Top-right corner */
  if (draw_rectangle_border(h_res - corner_size - 20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-left corner */
  if (draw_rectangle_border(20, v_res - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-right corner */
  if (draw_rectangle_border(h_res - corner_size - 20, v_res - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  return 0;
}

int draw_leaderboard() {
  /* Use the new graphics leaderboard function with mouse support */
  return draw_leaderboard_graphics();
}

int draw_leaderboard_with_hover(uint16_t mouse_x, uint16_t mouse_y) {
  /* Use the leaderboard function with mouse support */
  return draw_leaderboard_with_mouse(mouse_x, mouse_y);
}

int draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for (uint16_t row = 0; row < height; row++) {
    for (uint16_t col = 0; col < width; col++) {
      if (draw_pixel(x + col, y + row, color) != 0) {
        return 1;
      }
    }
  }
  return 0;
}

int draw_mouse_cursor(uint16_t x, uint16_t y, uint32_t color) {
  /* Simple arrow cursor - 11x16 pixels */
  static const uint8_t cursor_pattern[16] = {
    0x80, // 1.......
    0xC0, // 11......
    0xE0, // 111.....
    0xF0, // 1111....
    0xF8, // 11111...
    0xFC, // 111111..
    0xFE, // 1111111.
    0xFF, // 11111111
    0xF8, // 11111...
    0xF8, // 11111...
    0xD8, // 11.11...
    0x8C, // 1...11..
    0x0C, // ....11..
    0x06, // .....11.
    0x06, // .....11.
    0x00  // ........
  };
  
  for (int row = 0; row < 16; row++) {
    uint8_t pattern = cursor_pattern[row];
    for (int col = 0; col < 8; col++) {
      if (pattern & (0x80 >> col)) {
        if (x + col < h_res && y + row < v_res) {
          draw_pixel(x + col, y + row, color);
        }
      }
    }
  }
  
  return 0;
}

int draw_menu_option_hover(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                          const char *text, uint32_t text_color, uint32_t border_color, 
                          uint8_t scale, bool is_hovered) {
  /* Draw background fill if hovered */
  if (is_hovered) {
    uint32_t hover_color = 0x2a2a4e; /* Lighter blue-gray for hover */
    if (draw_filled_rectangle(x + 2, y + 2, width - 4, height - 4, hover_color) != 0) {
      return 1;
    }
  }
  
  /* Draw border */
  uint32_t final_border_color = is_hovered ? 0xffa500 : border_color; /* Orange when hovered */
  if (draw_rectangle_border(x, y, width, height, final_border_color, 2) != 0) {
    return 1;
  }
  
  /* Calculate text position to center it */
  uint16_t text_width = strlen(text) * 8 * scale;
  uint16_t text_height = 8 * scale;
  
  uint16_t text_x = x + (width - text_width) / 2;
  uint16_t text_y = y + (height - text_height) / 2;
  
  /* Draw text with enhanced color when hovered */
  uint32_t final_text_color = is_hovered ? 0xffffff : text_color; /* Brighter white when hovered */
  if (draw_string_scaled(text_x, text_y, text, final_text_color, scale) != 0) {
    return 1;
  }
  
  return 0;
}

bool is_point_in_rect(uint16_t px, uint16_t py, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh) {
  return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

int draw_main_page_with_hover(uint16_t mouse_x, uint16_t mouse_y) {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
  uint32_t orange = 0xff6b35;        /* Orange for title */
  uint32_t yellow = 0xffd700;        /* Yellow for borders */
  uint32_t white = 0xffffff;         /* White for text */
  uint32_t light_blue = 0x16537e;    /* Light blue for accent */
  
  /* Clear screen with dark background */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title "Fight List" centered at top */
  const char *title = "FIGHT LIST";
  uint8_t title_scale = 4;
  uint16_t title_width = strlen(title) * 8 * title_scale;
  uint16_t title_x = (h_res - title_width) / 2;
  uint16_t title_y = 40;
  
  if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
  
  /* Draw decorative line under title */
  uint16_t line_width = title_width + 40;
  uint16_t line_x = (h_res - line_width) / 2;
  uint16_t line_y = title_y + title_scale * 8 + 15;
  
  for (uint16_t i = 0; i < line_width; i++) {
    for (uint8_t thickness = 0; thickness < 3; thickness++) {
      if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
    }
  }
  
  /* Menu options dimensions */
  uint16_t option_width = 230;
  uint16_t option_height = 60;
  uint16_t spacing = 20;
  
  /* Calculate starting positions */
  uint16_t start_y = line_y + 60;
  uint16_t center_x = h_res / 2;
  
  /* First row: Single Player and 2 Player (side by side) */
  uint16_t row1_y = start_y;
  uint16_t single_x = center_x - option_width - spacing/2;
  uint16_t multi_x = center_x + spacing/2;
  
  /* Check hover states */
  bool single_hovered = is_point_in_rect(mouse_x, mouse_y, single_x, row1_y, option_width, option_height);
  bool multi_hovered = is_point_in_rect(mouse_x, mouse_y, multi_x, row1_y, option_width, option_height);
  
  if (draw_menu_option_hover(single_x, row1_y, option_width, option_height, 
                            "Single Player", white, yellow, 2, single_hovered) != 0) return 1;
  
  if (draw_menu_option_hover(multi_x, row1_y, option_width, option_height, 
                            "2 Player", white, yellow, 2, multi_hovered) != 0) return 1;
  
  /* Second row: Leaderboard and Instructions (side by side) */
  uint16_t row2_y = row1_y + option_height + spacing + 10;
  
  bool leader_hovered = is_point_in_rect(mouse_x, mouse_y, single_x, row2_y, option_width, option_height);
  bool instr_hovered = is_point_in_rect(mouse_x, mouse_y, multi_x, row2_y, option_width, option_height);
  
  if (draw_menu_option_hover(single_x, row2_y, option_width, option_height, 
                            "Leaderboard", white, yellow, 2, leader_hovered) != 0) return 1;
  
  if (draw_menu_option_hover(multi_x, row2_y, option_width, option_height, 
                            "Instructions", white, yellow, 2, instr_hovered) != 0) return 1;
  
  /* Third row: Quit (full width) */
  uint16_t row3_y = row2_y + option_height + spacing + 20;
  uint16_t quit_width = option_width * 2 + spacing;
  uint16_t quit_x = center_x - quit_width/2;
  
  bool quit_hovered = is_point_in_rect(mouse_x, mouse_y, quit_x, row3_y, quit_width, option_height);
  
  if (draw_menu_option_hover(quit_x, row3_y, quit_width, option_height, 
                            "QUIT", white, 0xff4444, 3, quit_hovered) != 0) return 1; /* Red border for quit */
  
  /* Add some decorative elements */
  /* Corner decorations */
  uint16_t corner_size = 30;
  
  /* Top-left corner */
  if (draw_rectangle_border(20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Top-right corner */
  if (draw_rectangle_border(h_res - corner_size - 20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-left corner */
  if (draw_rectangle_border(20, v_res - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Bottom-right corner */
  if (draw_rectangle_border(h_res - corner_size - 20, v_res - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
  
  /* Draw mouse cursor */
  if (draw_mouse_cursor(mouse_x, mouse_y, 0xffffff) != 0) return 1;
  
  return 0;
}

game_state_t get_game_state() {
  return current_state;
}

void set_game_state(game_state_t state) {
  current_state = state;
}

int draw_current_page(uint16_t mouse_x, uint16_t mouse_y) {
  switch (current_state) {
    case STATE_MAIN_MENU:
      return draw_main_page_with_hover(mouse_x, mouse_y);
    case STATE_SINGLE_PLAYER:
      return draw_init_sp_game();
    case STATE_MULTIPLAYER:
      return draw_init_mp_game();
    case STATE_MULTIPLAYER_TEST:
      return draw_multiplayer_test_screen();
    case STATE_MP_WAITING_FOR_OTHER_PLAYER:
      return draw_mp_waiting_for_other_player();
    case STATE_MP_RESULTS:
      return draw_mp_results_screen();
    case STATE_LEADERBOARD:
      return draw_leaderboard_with_hover(mouse_x, mouse_y);
    case STATE_INSTRUCTIONS:
      return draw_instructions_with_mouse(mouse_x, mouse_y);
    case STATE_SP_ENTER_INITIALS:
      return draw_enter_initials_page(mouse_x, mouse_y);
    case STATE_SP_COUNTDOWN:
      return draw_countdown_page(); /* No mouse support */
    case STATE_SP_LETTER_RAIN:
      return game_draw_letter_rain(get_current_game()); /* No mouse support */
    case STATE_SP_PLAYING:
      /* Initialize and use singleplayer game */
      if (!sp_initialized) {
        /* Get player info from letter_rain phase */
        jogo_t *game = get_current_game();
        singleplayer_init(&sp_game, game->nome, game->letra);
        sp_initialized = true;
        printf("Singleplayer game initialized with player=%s, letter=%c\n", 
               game->nome, game->letra);
      }
      
      /* Update and draw singleplayer game */
      singleplayer_update(&sp_game);
      return singleplayer_draw(&sp_game);
    default:
      return draw_main_page_with_hover(mouse_x, mouse_y);
  }
}

int draw_instructions() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
  uint32_t orange = 0xff6b35;        /* Orange for title */
  uint32_t white = 0xffffff;         /* White for text */
  uint32_t light_blue = 0x16537e;    /* Light blue for accent */
  uint32_t yellow = 0xffd700;        /* Yellow for highlights */
  uint32_t green = 0x00ff00;         /* Green for positive info */
  
  /* Clear screen with dark background */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "REGRAS DO JOGO";
  uint8_t title_scale = 3;
  uint16_t title_width = strlen(title) * 8 * title_scale;
  uint16_t title_x = (get_h_res() - title_width) / 2;
  uint16_t title_y = 30;
  
  if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
  
  /* Draw decorative line under title */
  uint16_t line_width = title_width + 40;
  uint16_t line_x = (get_h_res() - line_width) / 2;
  uint16_t line_y = title_y + title_scale * 8 + 10;
  
  for (uint16_t i = 0; i < line_width; i++) {
    for (uint8_t thickness = 0; thickness < 3; thickness++) {
      if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
    }
  }
  
  /* Draw decorative border around the rules */
  uint16_t border_x = 60;
  uint16_t border_y = line_y + 20;
  uint16_t border_width = get_h_res() - 120;
  uint16_t border_height = 360;
  
  if (draw_rectangle_border(border_x, border_y, border_width, border_height, light_blue, 2) != 0) return 1;
  
  /* Draw game rules - smaller text to fit better */
  uint16_t start_y = border_y + 20;
  uint16_t line_spacing = 20;
  uint16_t text_x = border_x + 20;
  
  /* Rule 1 */
  if (draw_string_scaled(text_x, start_y, "1. Vai chover letras! Apanha duas letras iguais para escolher", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 15, "a letra com que vais jogar!", white, 1) != 0) return 1;

  start_y += line_spacing + 25;
  /* Rule 2 */
  if (draw_string_scaled(text_x, start_y, "2. O jogo escolhe uma categoria aleatoria", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 15, "(Exemplo: Animais, Paises, Profissoes...)", yellow, 1) != 0) return 1;
  
  /* Rule 3 */
  start_y += line_spacing + 25;
  if (draw_string_scaled(text_x, start_y, "3. Tens 35 segundos para escrever palavras dessa categoria,", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 15, "usando a letra conquistada", white, 1) != 0) return 1;
  
  /* Rule 4 */
  start_y += line_spacing + 25;
  if (draw_string_scaled(text_x, start_y, "4. Pontuacao por palavra:", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 15, "- Palavras curtas (3-5 letras): 1 ponto", green, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 28, "- Palavras medias (6-8 letras): 2 pontos", green, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 41, "- Palavras longas (9+ letras): 3 pontos", green, 1) != 0) return 1;
  
  /* Rule 5 */
  start_y += line_spacing + 55;
  if (draw_string_scaled(text_x, start_y, "5. O jogo termina quando:", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 15, "- Acertas todas as palavras possiveis", yellow, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 28, "- OU quando o tempo acaba", yellow, 1) != 0) return 1;
  
  /* Controls section */
  start_y += line_spacing + 40;
  if (draw_string_scaled(text_x, start_y, "CONTROLOS:", orange, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 18, "- Use o teclado para escrever palavras", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 31, "- Press ENTER para submeter palavra", white, 1) != 0) return 1;
  if (draw_string_scaled(text_x + 20, start_y + 44, "- Press ESC para voltar ao menu", white, 1) != 0) return 1;
  
  /* Add decorative corners like in main menu */
  uint16_t corner_size = 30;
  
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

int draw_instructions_with_mouse(uint16_t mouse_x, uint16_t mouse_y) {
  /* Draw the instructions content first */
  if (draw_instructions() != 0) return 1;
  
  /* Draw back arrow button in top-left */
  uint16_t arrow_x = 30;
  uint16_t arrow_y = 30;
  uint16_t arrow_width = 80;
  uint16_t arrow_height = 30;
  
  /* Check if mouse is hovering over back button */
  bool back_hovered = (mouse_x >= arrow_x && mouse_x <= arrow_x + arrow_width &&
                      mouse_y >= arrow_y && mouse_y <= arrow_y + arrow_height);
  
  /* Draw back button */
  uint32_t button_color = back_hovered ? 0xffa500 : 0xffd700; /* Orange when hovered, yellow otherwise */
  uint32_t bg_color = back_hovered ? 0x2a2a4e : 0x1a1a2e;
  
  /* Draw button background */
  if (draw_filled_rectangle(arrow_x, arrow_y, arrow_width, arrow_height, bg_color) != 0) return 1;
  
  /* Draw button border */
  if (draw_rectangle_border(arrow_x, arrow_y, arrow_width, arrow_height, button_color, 2) != 0) return 1;
  
  /* Draw back arrow and text */
  const char *back_text = "<- BACK";
  uint16_t text_x = arrow_x + 8;
  uint16_t text_y = arrow_y + 8;
  uint32_t text_color = back_hovered ? 0xffffff : 0xffd700;
  
  if (draw_string_scaled(text_x, text_y, back_text, text_color, 1) != 0) return 1;
  
  /* Draw mouse cursor */
  if (draw_mouse_cursor(mouse_x, mouse_y, 0xffffff) != 0) return 1;
  
  return 0;
}

int handle_instructions_click(uint16_t x, uint16_t y, bool left_click) {
  if (!left_click) return -1;  /* Only handle left clicks */
  
  /* Check back button */
  uint16_t arrow_x = 30;
  uint16_t arrow_y = 30;
  uint16_t arrow_width = 80;
  uint16_t arrow_height = 30;
  
  if (x >= arrow_x && x <= arrow_x + arrow_width &&
      y >= arrow_y && y <= arrow_y + arrow_height) {
    return 1; /* Back button clicked */
  }
  
  return -1; /* No action */
}

int draw_init_sp_game() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "SINGLE PLAYER";
  uint16_t title_x = (get_h_res() - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Draw game setup */
  if (draw_string_scaled(100, 200, "Iniciando modo single player...", white, 2) != 0) return 1;
  
  /* Initialize game and move to initials entry IMMEDIATELY */
  jogo_t *game = get_current_game();
  game_init(game);
  set_game_state(STATE_SP_ENTER_INITIALS);
  
  /* Don't draw the "preparing" message - go straight to initials */
  return 0;
}

int draw_init_mp_game() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  
  /* Reset multiplayer state for clean start */
  reset_multiplayer_connection();
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "2 PLAYER MODE";
  uint16_t title_x = (h_res - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Initialize serial communication */
  if (serial_init() == 0) {
    if (draw_string_scaled(100, 200, "Serial port initialized successfully!", white, 2) != 0) return 1;
    if (draw_string_scaled(100, 250, "Starting connection test...", white, 2) != 0) return 1;
    
    /* Transition to test state */
    set_game_state(STATE_MULTIPLAYER_TEST);
  } else {
    if (draw_string_scaled(100, 200, "ERROR: Failed to initialize serial port!", 0xff4444, 2) != 0) return 1;
    if (draw_string_scaled(100, 250, "Check your VM serial port configuration", white, 1) != 0) return 1;
  }
  
  /* Draw back instruction */
  if (draw_string_scaled(100, 400, "Press ESC to go back", white, 1) != 0) return 1;
  
  return 0;
}

/* Reset the singleplayer game state when returning to main menu */
void reset_singleplayer() {
  sp_initialized = false;
  is_multiplayer_mode = false;
  printf("Singleplayer game state reset for new game\n");
}

/* Reset multiplayer connection state */
void reset_multiplayer_connection() {
  printf("=== RESETTING MULTIPLAYER CONNECTION STATE ===\n");
  mp_test_initialized = false;
  mp_test_counter = 0;
  mp_received_ping = false;
  mp_connection_timer = 0;
  is_multiplayer_mode = false;
  
  /* Reset multiplayer results data */
  memset(mp_other_player_initials, 0, sizeof(mp_other_player_initials));
  mp_other_player_score = 0;
  mp_results_ready = false;
  
  /* Generate unique session ID for this multiplayer session */
  /* Use a combination of time, process ID-like value, and random to ensure uniqueness */
  static uint32_t vm_instance_id = 0;
  if (vm_instance_id == 0) {
    vm_instance_id = (uint32_t)time(NULL) % 1000; /* Use last 3 digits of time as VM identifier */
  }
  vm_instance_id++; /* Increment for each session */
  
  mp_session_id = ((uint32_t)time(NULL) * 1000) + vm_instance_id + (uint32_t)rand() % 1000;
  
  /* Determine VM identifier based on last digit of session ID to ensure uniqueness */
  mp_vm_id = ((mp_session_id % 100) < 50) ? 'A' : 'B';
  
  printf("Generated unique session ID: %u (VM instance: %u, VM ID: %c)\n", mp_session_id, vm_instance_id, mp_vm_id);
  
  /* Also reset any static variables in waiting state by clearing serial buffer */
  if (serial_init() == 0) {
    char dummy;
    int clear_count = 0;
    while ((dummy = serial_read_char()) != 0 && clear_count < 100) {
      clear_count++;
    }
    if (clear_count > 0) {
      printf("Cleared %d stale messages during reset\n", clear_count);
    }
  }
}

bool is_in_multiplayer_mode() {
  return is_multiplayer_mode;
}

int draw_multiplayer_test_screen() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  uint32_t green = 0x00ff00;
  uint32_t yellow = 0xffff00;
  
  static int total_chars_received = 0;
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "2 PLAYER MODE";
  uint16_t title_x = (h_res - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Initialize test on first run */
  if (!mp_test_initialized) {
    printf("=== MULTIPLAYER MODE STARTED ===\n");
    printf("Waiting for connection with other player...\n");
    
    /* Clear any stray data from serial buffer */
    char dummy;
    int clear_count = 0;
    while ((dummy = serial_read_char()) != 0 && clear_count < 100) {
      clear_count++;
    }
    if (clear_count > 0) {
      printf("Cleared %d stray characters from serial buffer\n", clear_count);
    }
    
    /* Send initial test message */
    printf("=== SENDING INITIAL TEST MESSAGE ===\n");
    int test_result = serial_send_string("CONNECT_TEST\n");
    if (test_result == 0) {
      printf("Initial CONNECT_TEST message sent successfully\n");
    } else {
      printf("ERROR: Failed to send initial CONNECT_TEST message! Error code: %d\n", test_result);
    }
    
    mp_test_counter = 0;
    mp_received_ping = false;
    mp_connection_timer = 0;
    total_chars_received = 0;
    mp_test_initialized = true;
  }
  
  /* Always send ping messages every 30 timer ticks (about 0.5 seconds) */
  mp_test_counter++;
  if (mp_test_counter >= 30) {
    printf("=== SENDING CONNECT_PING MESSAGE ===\n");
    int send_result = serial_send_string("CONNECT_PING\n");
    if (send_result == 0) {
      printf("CONNECT_PING sent successfully (received_ping=%s)\n", mp_received_ping ? "true" : "false");
    } else {
      printf("ERROR: Failed to send CONNECT_PING! Error code: %d\n", send_result);
    }
    mp_test_counter = 0;
  }
  
  /* Check for incoming messages */
  char c;
  static char msg_buffer[64] = {0};
  static int msg_pos = 0;
  
  while ((c = serial_read_char()) != 0) {
    total_chars_received++;
    printf("Received char #%d: '%c' (0x%02X)\n", total_chars_received, 
           (c >= 32 && c <= 126) ? c : '?', (unsigned char)c);
    
    if (c == '\n' || c == '\r') {
      /* Complete message received */
      msg_buffer[msg_pos] = '\0';
      if (strlen(msg_buffer) > 0) {
        printf("=== COMPLETE MESSAGE RECEIVED ===\n");
        printf("Message: '%s' (length: %d)\n", msg_buffer, (int)strlen(msg_buffer));
        
        if (strstr(msg_buffer, "CONNECT_PING") != NULL) {
          printf("*** CONNECT_PING DETECTED! Setting received_ping = true ***\n");
          mp_received_ping = true;
        } else if (strstr(msg_buffer, "CONNECT_TEST") != NULL) {
          printf("*** CONNECT_TEST MESSAGE DETECTED! Basic communication working ***\n");
        } else if (strstr(msg_buffer, "GAME_FINISHED") != NULL) {
          printf("*** IGNORING GAME_FINISHED message - other player finished but we're still connecting ***\n");
        } else if (strstr(msg_buffer, "CONFIRMED_FINISHED") != NULL) {
          printf("*** IGNORING CONFIRMED_FINISHED message - game coordination while connecting ***\n");
        } else if (strstr(msg_buffer, "GAME_FINISHED_") != NULL && strlen(msg_buffer) > 14) {
          char sender_vm_id = msg_buffer[14]; /* Extract VM ID from message */
          printf("*** DEBUG: Message sender VM ID=%c ***\n", sender_vm_id);
          
          if (sender_vm_id != mp_vm_id) { /* Accept any message NOT from our own VM */
            printf("*** VALID GAME_FINISHED FROM OTHER VM %c DETECTED! ***\n", sender_vm_id);
            printf("*** Other player has finished their game! ***\n");
            
            /* Parse the message to extract initials and score */
            /* Format: GAME_FINISHED_X_SESSION_INITIALS_SCORE */
            char msg_copy[64];
            strcpy(msg_copy, msg_buffer);
            char *token = strtok(msg_copy, "_");
            int token_count = 0;
            char temp_initials[4] = {0};
            int temp_score = 0;
            
            printf("*** DEBUG: Parsing message '%s' ***\n", msg_buffer);
            
            while (token != NULL && token_count < 6) {
              printf("*** DEBUG: Token %d = '%s' ***\n", token_count, token);
              if (token_count == 4) { /* Initials are the 5th token (index 4) */
                strncpy(temp_initials, token, 3);
                temp_initials[3] = '\0';
                printf("*** DEBUG: Extracted initials = '%s' ***\n", temp_initials);
              } else if (token_count == 5) { /* Score is the 6th token (index 5) */
                temp_score = atoi(token);
                printf("*** DEBUG: Extracted score = %d ***\n", temp_score);
              }
              token = strtok(NULL, "_");
              token_count++;
            }
            
            /* Store other player's data only if we got valid initials */
            if (strlen(temp_initials) > 0) {
              strcpy(mp_other_player_initials, temp_initials);
              mp_other_player_score = temp_score;
              mp_results_ready = true;
              
              printf("*** Other player: %s with score %d ***\n", mp_other_player_initials, mp_other_player_score);
            } else {
              printf("*** ERROR: Failed to parse other player's initials from message ***\n");
            }
            
            /* Send confirmation with our VM ID */
            char confirm_msg[64];
            sprintf(confirm_msg, "CONFIRMED_FINISHED_%c_%u\n", mp_vm_id, mp_session_id);
            serial_send_string(confirm_msg);
            printf("*** Sent CONFIRMED_FINISHED from VM %c to VM %c ***\n", mp_vm_id, sender_vm_id);
          } else if (sender_vm_id == mp_vm_id) {
            printf("*** LOOPBACK: Ignoring our own GAME_FINISHED message ***\n");
          } else {
            printf("*** UNKNOWN VM ID %c in GAME_FINISHED message ***\n", sender_vm_id);
          }
        }
        else if (strstr(msg_buffer, "CONFIRMED_FINISHED_") != NULL && strlen(msg_buffer) > 18) {
          char sender_vm_id = msg_buffer[18]; /* Extract VM ID from confirmation message */
          printf("*** DEBUG: Confirmation sender VM ID=%c ***\n", sender_vm_id);
          
          if (sender_vm_id == mp_vm_id) {
            printf("*** VALID CONFIRMED_FINISHED FROM OTHER VM %c DETECTED! ***\n", sender_vm_id);
            printf("*** Other VM confirmed they received our GAME_FINISHED! ***\n");
            mp_results_ready = true;
          } else if (sender_vm_id == mp_vm_id) {
            printf("*** LOOPBACK: Ignoring our own CONFIRMED_FINISHED message ***\n");
          } else {
            printf("*** UNKNOWN VM ID %c in CONFIRMED_FINISHED message ***\n", sender_vm_id);
          }
        }
        else {
          printf("*** IGNORING UNKNOWN MESSAGE: '%s' ***\n", msg_buffer);
        }
      } else {
        printf("Empty message received (just newline)\n");
      }
      
      /* Reset buffer */
      msg_pos = 0;
      memset(msg_buffer, 0, sizeof(msg_buffer));
    } else if ((size_t)msg_pos < sizeof(msg_buffer) - 1) {
      msg_buffer[msg_pos++] = c;
      printf("Added to buffer. Buffer now: '%s' (pos: %d)\n", msg_buffer, msg_pos);
    } else {
      printf("Buffer full! Dropping character.\n");
    }
  }
  
  /* Show status every 60 ticks */
  static int status_counter = 0;
  status_counter++;
  if (status_counter >= 60) {
    printf("=== STATUS UPDATE ===\n");
    printf("Total characters received so far: %d\n", total_chars_received);
    printf("Received ping flag: %s\n", mp_received_ping ? "true" : "false");
    printf("Connection timer: %d\n", mp_connection_timer);
    status_counter = 0;
  }
  
  /* Once we've received a ping, start countdown to connection */
  if (mp_received_ping) {
    mp_connection_timer++;
    printf("Connection timer: %d\n", mp_connection_timer);
    
    /* Wait 60 ticks (1 second) after receiving first ping to ensure stability */
    if (mp_connection_timer >= 60) {
      printf("Connection established! Both players detected\n");
      
      /* Set multiplayer mode flag */
      is_multiplayer_mode = true;
      
      /* Initialize game and transition to initials entry */
      jogo_t *game = get_current_game();
      game_init(game);
      set_game_state(STATE_SP_ENTER_INITIALS);
      
      /* Force immediate redraw of new state */
      uint16_t mouse_x = 400; /* Default mouse position */
      uint16_t mouse_y = 300;
      draw_current_page(mouse_x, mouse_y);
      
      return 0;
    }
  }
  
  /* Draw waiting screen */
  uint16_t center_x = h_res / 2;
  uint16_t center_y = v_res / 2;
  
  /* Main waiting message */
  const char *waiting_msg = "Waiting for connection...";
  uint16_t waiting_width = strlen(waiting_msg) * 8 * 2;
  uint16_t waiting_x = center_x - waiting_width / 2;
  if (draw_string_scaled(waiting_x, center_y - 60, waiting_msg, white, 2) != 0) return 1;
  
  /* Connection status */
  const char *status_msg = "Make sure both VMs are running this screen";
  uint16_t status_width = strlen(status_msg) * 8;
  uint16_t status_x = center_x - status_width / 2;
  if (draw_string_scaled(status_x, center_y - 20, status_msg, yellow, 1) != 0) return 1;
  
  /* Animated dots to show it's working */
  static int dot_counter = 0;
  dot_counter++;
  if (dot_counter >= 60) dot_counter = 0; /* Reset every second */
  
  char dots_msg[10] = "Searching";
  int num_dots = (dot_counter / 15) + 1; /* 1-4 dots */
  for (int i = 0; i < num_dots && i < 4; i++) {
    strcat(dots_msg, ".");
  }
  
  uint16_t dots_width = strlen(dots_msg) * 8;
  uint16_t dots_x = center_x - dots_width / 2;
  if (draw_string_scaled(dots_x, center_y + 20, dots_msg, green, 1) != 0) return 1;
  
  /* Serial port status */
  const char *serial_msg = "Serial Port: /dev/tty00 - ACTIVE";
  uint16_t serial_width = strlen(serial_msg) * 8;
  uint16_t serial_x = center_x - serial_width / 2;
  if (draw_string_scaled(serial_x, center_y + 60, serial_msg, green, 1) != 0) return 1;
  
  /* Instructions */
  const char *instr_msg = "Press ESC to cancel and return to main menu";
  uint16_t instr_width = strlen(instr_msg) * 8;
  uint16_t instr_x = center_x - instr_width / 2;
  if (draw_string_scaled(instr_x, center_y + 120, instr_msg, white, 1) != 0) return 1;
  
  /* Add decorative border */
  uint16_t border_width = 400;
  uint16_t border_height = 250;
  uint16_t border_x = center_x - border_width / 2;
  uint16_t border_y = center_y - 100;
  if (draw_rectangle_border(border_x, border_y, border_width, border_height, 0x16537e, 2) != 0) return 1;
  
  return 0;
}

int draw_mp_waiting_for_other_player() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  uint32_t green = 0x00ff00;
  uint32_t yellow = 0xffff00;
  
  static bool waiting_initialized = false;
  static bool other_player_finished = false;
  static bool sent_our_finished = false;
  static bool received_their_finished = false;
  static int message_counter = 0;
  static int confirmation_wait_timer = 0;
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "MULTIPLAYER GAME FINISHED";
  uint16_t title_x = (h_res - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Initialize waiting state */
  if (!waiting_initialized) {
    printf("=== PLAYER FINISHED - WAITING FOR OTHER PLAYER ===\n");
    other_player_finished = false;
    sent_our_finished = false;
    received_their_finished = false;
    message_counter = 0;
    confirmation_wait_timer = 0;
    waiting_initialized = true;
    
    /* AGGRESSIVELY clear any stale messages from serial buffer */
    printf("=== AGGRESSIVELY CLEARING STALE MESSAGES ===\n");
    char dummy;
    int clear_count = 0;
    
    /* Clear in multiple passes with delays */
    for (int pass = 0; pass < 5; pass++) {
      int pass_count = 0;
      while ((dummy = serial_read_char()) != 0 && clear_count < 500) {
        clear_count++;
        pass_count++;
      }
      printf("Pass %d: cleared %d messages\n", pass + 1, pass_count);
      
      /* Small delay between passes to let any in-transit messages arrive */
      for (int delay = 0; delay < 10000; delay++) {
        /* Simple delay loop */
      }
    }
    
    printf("Total cleared: %d stale messages from serial buffer\n", clear_count);
    printf("=== BUFFER CLEARING COMPLETE ===\n");
  }
  
  /* Send "FINISHED" message periodically every 60 ticks (1 second) to ensure delivery */
  message_counter++;
  if (message_counter >= 60) {
    printf("=== SENDING GAME_FINISHED MESSAGE ===\n");
    
    /* Include session ID, VM ID, score and initials */
    extern singleplayer_game_t sp_game;
    char session_msg[128];
    sprintf(session_msg, "GAME_FINISHED_%c_%u_%s_%d\n", 
            mp_vm_id, mp_session_id, sp_game.player_initials, sp_game.total_score);
    printf("Sending exact message from VM %c: '%s'\n", mp_vm_id, session_msg);
    
    int send_result = serial_send_string(session_msg);
    if (send_result == 0) {
      printf("*** GAME_FINISHED sent successfully from VM %c with session ID %u ***\n", mp_vm_id, mp_session_id);
      sent_our_finished = true;
      mp_last_sent_time = (uint32_t)time(NULL); /* Track when we sent this message */
    } else {
      printf("ERROR: Failed to send GAME_FINISHED message! Error code: %d\n", send_result);
    }
    message_counter = 0;
  }
  
  /* Check for messages from other player */
  char c;
  static char msg_buffer[64] = {0};
  static int msg_pos = 0;
  
  while ((c = serial_read_char()) != 0) {
    if (c == '\n' || c == '\r') {
      /* Complete message received */
      msg_buffer[msg_pos] = '\0';
      if (strlen(msg_buffer) > 0) {
        printf("=== WAITING STATE MESSAGE ANALYSIS ===\n");
        printf("Raw message: '%s' (length: %d)\n", msg_buffer, (int)strlen(msg_buffer));
        
        /* ONLY process GAME_FINISHED and CONFIRMED_FINISHED messages with current session ID */
        char expected_game_finished[64];
        char expected_confirmed[64];
        char other_vm_id = (mp_vm_id == 'A') ? 'B' : 'A'; /* Get the OTHER VM's ID */
        sprintf(expected_game_finished, "GAME_FINISHED_%c_%u", other_vm_id, mp_session_id);
        sprintf(expected_confirmed, "CONFIRMED_FINISHED_%c_%u", other_vm_id, mp_session_id);
        
        printf("*** DEBUG: Our VM ID=%c, Other VM ID=%c, Our Session=%u ***\n", mp_vm_id, other_vm_id, mp_session_id);
        printf("*** DEBUG: Expected GAME_FINISHED='%s' ***\n", expected_game_finished);
        printf("*** DEBUG: Expected CONFIRMED='%s' ***\n", expected_confirmed);
        
        /* Check if this is a GAME_FINISHED message from the other VM (ignore session ID) */
        if (strstr(msg_buffer, "GAME_FINISHED_") != NULL && strlen(msg_buffer) > 14) {
          char sender_vm_id = msg_buffer[14]; /* Extract VM ID from message */
          printf("*** DEBUG: Message sender VM ID=%c ***\n", sender_vm_id);
          
          if (sender_vm_id != mp_vm_id) { /* Accept any message NOT from our own VM */
            printf("*** VALID GAME_FINISHED FROM OTHER VM %c DETECTED! ***\n", sender_vm_id);
            printf("*** Other player has finished their game! ***\n");
            
            /* Parse the message to extract initials and score */
            /* Format: GAME_FINISHED_X_SESSION_INITIALS_SCORE */
            char msg_copy[64];
            strcpy(msg_copy, msg_buffer);
            char *token = strtok(msg_copy, "_");
            int token_count = 0;
            char temp_initials[4] = {0};
            int temp_score = 0;
            
            printf("*** DEBUG: Parsing message '%s' ***\n", msg_buffer);
            
            while (token != NULL && token_count < 6) {
              printf("*** DEBUG: Token %d = '%s' ***\n", token_count, token);
              if (token_count == 4) { /* Initials are the 5th token (index 4) */
                strncpy(temp_initials, token, 3);
                temp_initials[3] = '\0';
                printf("*** DEBUG: Extracted initials = '%s' ***\n", temp_initials);
              } else if (token_count == 5) { /* Score is the 6th token (index 5) */
                temp_score = atoi(token);
                printf("*** DEBUG: Extracted score = %d ***\n", temp_score);
              }
              token = strtok(NULL, "_");
              token_count++;
            }
            
            /* Store other player's data only if we got valid initials */
            if (strlen(temp_initials) > 0) {
              strcpy(mp_other_player_initials, temp_initials);
              mp_other_player_score = temp_score;
              received_their_finished = true;
              
              printf("*** Other player: %s with score %d ***\n", mp_other_player_initials, mp_other_player_score);
            } else {
              printf("*** ERROR: Failed to parse other player's initials from message ***\n");
            }
            
            /* Send confirmation with our VM ID */
            char confirm_msg[64];
            sprintf(confirm_msg, "CONFIRMED_FINISHED_%c_%u\n", mp_vm_id, mp_session_id);
            serial_send_string(confirm_msg);
            printf("*** Sent CONFIRMED_FINISHED from VM %c to VM %c ***\n", mp_vm_id, sender_vm_id);
          } else if (sender_vm_id == mp_vm_id) {
            printf("*** LOOPBACK: Ignoring our own GAME_FINISHED message ***\n");
          } else {
            printf("*** UNKNOWN VM ID %c in GAME_FINISHED message ***\n", sender_vm_id);
          }
        }
        /* Check if this is a CONFIRMED_FINISHED message from the other VM */
        else if (strstr(msg_buffer, "CONFIRMED_FINISHED_") != NULL && strlen(msg_buffer) > 18) {
          char sender_vm_id = msg_buffer[18]; /* Extract VM ID from confirmation message */
          printf("*** DEBUG: Confirmation sender VM ID=%c ***\n", sender_vm_id);
          
          if (sender_vm_id != mp_vm_id) { /* Accept any message NOT from our own VM */
            printf("*** VALID CONFIRMED_FINISHED FROM OTHER VM %c DETECTED! ***\n", sender_vm_id);
            printf("*** Other VM confirmed they received our GAME_FINISHED! ***\n");
            received_their_finished = true;
          } else if (sender_vm_id == mp_vm_id) {
            printf("*** LOOPBACK: Ignoring our own CONFIRMED_FINISHED message ***\n");
          } else {
            printf("*** UNKNOWN VM ID %c in CONFIRMED_FINISHED message ***\n", sender_vm_id);
          }
        }
        else {
          printf("*** IGNORING UNKNOWN MESSAGE: '%s' ***\n", msg_buffer);
        }
        
        printf("Current state: sent_our_finished=%s, received_their_finished=%s\n", 
               sent_our_finished ? "true" : "false", 
               received_their_finished ? "true" : "false");
        printf("===========================================\n");
      }
      
      /* Reset buffer */
      msg_pos = 0;
      memset(msg_buffer, 0, sizeof(msg_buffer));
    } else if ((size_t)msg_pos < sizeof(msg_buffer) - 1) {
      msg_buffer[msg_pos++] = c;
    }
  }
  
  /* Check if both players are done and we have confirmation */
  if (sent_our_finished && received_their_finished) {
    confirmation_wait_timer++;
    printf("Both players finished! Waiting for stability... (%d/60)\n", confirmation_wait_timer);
    
    /* Wait for 60 ticks (1 second) to ensure both players are synchronized */
    if (confirmation_wait_timer >= 60) {
      printf("=== BOTH PLAYERS CONFIRMED FINISHED - SHOWING MULTIPLAYER RESULTS ===\n");
      
      /* Set results ready flag */
      mp_results_ready = true;
      
      /* Reset multiplayer mode flag but keep results data */
      is_multiplayer_mode = false;
      waiting_initialized = false;
      
      /* Transition to multiplayer results screen */
      set_game_state(STATE_MP_RESULTS);
      
      return 0;
    }
  }
  
  /* Draw waiting screen */
  uint16_t center_x = h_res / 2;
  uint16_t center_y = v_res / 2;
  
  /* Main waiting message */
  const char *waiting_msg = "Waiting for other player to finish...";
  uint16_t waiting_width = strlen(waiting_msg) * 8 * 2;
  uint16_t waiting_x = center_x - waiting_width / 2;
  if (draw_string_scaled(waiting_x, center_y - 80, waiting_msg, white, 2) != 0) return 1;
  
  /* Show player's completion status */
  const char *your_status = "You have completed the game!";
  uint16_t your_width = strlen(your_status) * 8;
  uint16_t your_x = center_x - your_width / 2;
  if (draw_string_scaled(your_x, center_y - 40, your_status, green, 1) != 0) return 1;
  
  /* Show other player status with more detail */
  if (received_their_finished) {
    if (confirmation_wait_timer > 0) {
      const char *sync_status = "Both players finished! Synchronizing results...";
      uint32_t sync_color = green;
      uint16_t sync_width = strlen(sync_status) * 8;
      uint16_t sync_x = center_x - sync_width / 2;
      if (draw_string_scaled(sync_x, center_y - 10, sync_status, sync_color, 1) != 0) return 1;
    } else {
      const char *ready_status = "Other player has also finished!";
      uint32_t ready_color = green;
      uint16_t ready_width = strlen(ready_status) * 8;
      uint16_t ready_x = center_x - ready_width / 2;
      if (draw_string_scaled(ready_x, center_y - 10, ready_status, ready_color, 1) != 0) return 1;
    }
  } else {
    const char *waiting_status = "Other player is still playing...";
    uint32_t waiting_color = yellow;
    uint16_t waiting_status_width = strlen(waiting_status) * 8;
    uint16_t waiting_status_x = center_x - waiting_status_width / 2;
    if (draw_string_scaled(waiting_status_x, center_y - 10, waiting_status, waiting_color, 1) != 0) return 1;
  }
  
  /* Show connection status */
  char status_msg[100];
  if (sent_our_finished && received_their_finished) {
    sprintf(status_msg, "Synchronization: %d/60", confirmation_wait_timer);
  } else if (sent_our_finished) {
    sprintf(status_msg, "Sent completion signal, waiting for response...");
  } else {
    sprintf(status_msg, "Sending completion signal...");
  }
  
  uint16_t status_width = strlen(status_msg) * 8;
  uint16_t status_x = center_x - status_width / 2;
  if (draw_string_scaled(status_x, center_y + 30, status_msg, white, 1) != 0) return 1;
  
  /* Animated dots to show waiting */
  static int dot_counter = 0;
  dot_counter++;
  if (dot_counter >= 60) dot_counter = 0; /* Reset every second */
  
  char dots_msg[20] = "Please wait";
  int num_dots = (dot_counter / 15) + 1; /* 1-4 dots */
  for (int i = 0; i < num_dots && i < 4; i++) {
    strcat(dots_msg, ".");
  }
  
  uint16_t dots_width = strlen(dots_msg) * 8;
  uint16_t dots_x = center_x - dots_width / 2;
  if (draw_string_scaled(dots_x, center_y + 60, dots_msg, yellow, 1) != 0) return 1;
  
  /* ESC instruction */
  const char *esc_msg = "Press ESC to return to main menu";
  uint16_t esc_width = strlen(esc_msg) * 8;
  uint16_t esc_x = center_x - esc_width / 2;
  if (draw_string_scaled(esc_x, center_y + 100, esc_msg, white, 1) != 0) return 1;
  
  /* Add decorative border */
  uint16_t border_width = 500;
  uint16_t border_height = 280;
  uint16_t border_x = center_x - border_width / 2;
  uint16_t border_y = center_y - 120;
  if (draw_rectangle_border(border_x, border_y, border_width, border_height, 0x16537e, 2) != 0) return 1;
  
  return 0;
}

int draw_mp_results_screen() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  uint32_t green = 0x00ff00;
  uint32_t yellow = 0xffd700;
  uint32_t gold = 0xffaa00;
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Get our player data */
  extern singleplayer_game_t sp_game;
  char our_initials[4];
  strncpy(our_initials, sp_game.player_initials, 3);
  our_initials[3] = '\0';
  int our_score = sp_game.total_score;
  
  /* Determine winner/loser/tie */
  bool we_won = our_score > mp_other_player_score;
  bool tie = our_score == mp_other_player_score;
  
  uint16_t center_x = h_res / 2;
  uint16_t center_y = v_res / 2;
  
  if (tie) {
    /* Draw TIE screen */
    const char *title = "IT'S A TIE!";
    uint8_t title_scale = 4;
    uint16_t title_width = strlen(title) * 8 * title_scale;
    uint16_t title_x = center_x - title_width / 2;
    if (draw_string_scaled(title_x, 60, title, yellow, title_scale) != 0) return 1;
    
    /* Both players' scores */
    char tie_msg[100];
    sprintf(tie_msg, "Both players scored %d points!", our_score);
    uint16_t tie_width = strlen(tie_msg) * 8 * 2;
    uint16_t tie_x = center_x - tie_width / 2;
    if (draw_string_scaled(tie_x, 150, tie_msg, white, 2) != 0) return 1;
    
    /* Show both players */
    char players_msg[100];
    sprintf(players_msg, "%s  vs  %s", our_initials, mp_other_player_initials);
    uint16_t players_width = strlen(players_msg) * 8 * 3;
    uint16_t players_x = center_x - players_width / 2;
    if (draw_string_scaled(players_x, 200, players_msg, gold, 3) != 0) return 1;
    
  } else {
    /* Draw WINNER/LOSER screen */
    const char *winner_initials = we_won ? our_initials : mp_other_player_initials;
    const char *loser_initials = we_won ? mp_other_player_initials : our_initials;
    int winner_score = we_won ? our_score : mp_other_player_score;
    int loser_score = we_won ? mp_other_player_score : our_score;
    
    /* Title */
    const char *title = "GAME RESULTS";
    uint8_t title_scale = 3;
    uint16_t title_width = strlen(title) * 8 * title_scale;
    uint16_t title_x = center_x - title_width / 2;
    if (draw_string_scaled(title_x, 40, title, orange, title_scale) != 0) return 1;
    
    /* Winner section */
    const char *winner_label = "WINNER";
    uint16_t winner_label_width = strlen(winner_label) * 8 * 3;
    uint16_t winner_label_x = center_x - winner_label_width / 2;
    if (draw_string_scaled(winner_label_x, 120, winner_label, green, 3) != 0) return 1;
    
    /* Winner name and score */
    char winner_info[50];
    sprintf(winner_info, "%s - %d points", winner_initials, winner_score);
    uint16_t winner_info_width = strlen(winner_info) * 8 * 4;
    uint16_t winner_info_x = center_x - winner_info_width / 2;
    if (draw_string_scaled(winner_info_x, 170, winner_info, gold, 4) != 0) return 1;
    
    /* Separator line */
    uint16_t line_width = 400;
    uint16_t line_x = center_x - line_width / 2;
    uint16_t line_y = 240;
    for (uint16_t i = 0; i < line_width; i++) {
      for (uint8_t thickness = 0; thickness < 2; thickness++) {
        if (draw_pixel(line_x + i, line_y + thickness, white) != 0) return 1;
      }
    }
    
    /* Loser section */
    char loser_info[50];
    sprintf(loser_info, "%s - %d points", loser_initials, loser_score);
    uint16_t loser_info_width = strlen(loser_info) * 8 * 2;
    uint16_t loser_info_x = center_x - loser_info_width / 2;
    if (draw_string_scaled(loser_info_x, 270, loser_info, white, 2) != 0) return 1;
  }
  
  /* Instructions */
  const char *esc_msg = "Press ESC to return to main menu";
  uint16_t esc_width = strlen(esc_msg) * 8;
  uint16_t esc_x = center_x - esc_width / 2;
  if (draw_string_scaled(esc_x, v_res - 40, esc_msg, white, 1) != 0) return 1;
  
  /* Add decorative border */
  uint16_t border_width = 600;
  uint16_t border_height = 400;
  uint16_t border_x = center_x - border_width / 2;
  uint16_t border_y = center_y - 150;
  if (draw_rectangle_border(border_x, border_y, border_width, border_height, 0x16537e, 3) != 0) return 1;
  
  return 0;
}
