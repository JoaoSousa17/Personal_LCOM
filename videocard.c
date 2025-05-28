#include "videocard.h"
#include "font.h"
#include "leaderboard.h"
#include <machine/int86.h>
#include <lcom/vbe.h>
#include <string.h>
#include <stdbool.h>

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
    case STATE_LEADERBOARD:
      return draw_leaderboard_with_hover(mouse_x, mouse_y);
    case STATE_INSTRUCTIONS:
      return draw_instructions();
    default:
      return draw_main_page_with_hover(mouse_x, mouse_y);
  }
}

int draw_instructions() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "INSTRUCTIONS";
  uint16_t title_x = (h_res - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Draw instructions */
  if (draw_string_scaled(100, 150, "How to play:", white, 2) != 0) return 1;
  if (draw_string_scaled(100, 180, "- Use arrow keys to move", white, 1) != 0) return 1;
  if (draw_string_scaled(100, 200, "- Press SPACE to attack", white, 1) != 0) return 1;
  if (draw_string_scaled(100, 220, "- Defeat your opponent!", white, 1) != 0) return 1;
  
  /* Draw back instruction */
  if (draw_string_scaled(100, 400, "Press ESC to go back", white, 1) != 0) return 1;
  
  return 0;
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
  uint16_t title_x = (h_res - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Draw game setup */
  if (draw_string_scaled(100, 200, "Starting single player game...", white, 2) != 0) return 1;
  if (draw_string_scaled(100, 250, "Get ready to fight!", white, 2) != 0) return 1;
  
  /* Draw back instruction */
  if (draw_string_scaled(100, 400, "Press ESC to go back", white, 1) != 0) return 1;
  
  return 0;
}

int draw_init_mp_game() {
  /* Define colors */
  uint32_t bg_color = 0x1a1a2e;
  uint32_t orange = 0xff6b35;
  uint32_t white = 0xffffff;
  
  /* Clear screen */
  if (clear_screen(bg_color) != 0) return 1;
  
  /* Draw title */
  const char *title = "2 PLAYER MODE";
  uint16_t title_x = (h_res - strlen(title) * 8 * 3) / 2;
  if (draw_string_scaled(title_x, 50, title, orange, 3) != 0) return 1;
  
  /* Draw game setup */
  if (draw_string_scaled(100, 200, "Starting 2 player game...", white, 2) != 0) return 1;
  if (draw_string_scaled(100, 250, "Player 1 vs Player 2", white, 2) != 0) return 1;
  
  /* Draw back instruction */
  if (draw_string_scaled(100, 400, "Press ESC to go back", white, 1) != 0) return 1;
  
  return 0;
}
