#ifndef _VIDEOCARD_H_
#define _VIDEOCARD_H_

#include <lcom/lcf.h>
#include <stdint.h>
#include <stdbool.h>

/* Game states */
typedef enum {
  STATE_MAIN_MENU,
  STATE_SINGLE_PLAYER,
  STATE_MULTIPLAYER,
  STATE_LEADERBOARD,
  STATE_INSTRUCTIONS,
  STATE_SP_ENTER_INITIALS,
  STATE_SP_COUNTDOWN,
  STATE_SP_LETTER_RAIN,
  STATE_SP_PLAYING
} game_state_t;

/**
 * @brief Maps the VRAM to the process's address space
 * 
 * @param mode The video mode to set
 * @return 0 on success, non-zero otherwise
 */
int map_vram(uint16_t mode);

/**
 * @brief Sets the video card to the specified graphics mode
 * 
 * @param mode The video mode to set
 * @return 0 on success, non-zero otherwise
 */
int set_graphics_mode(uint16_t mode);

/**
 * @brief Exits graphics mode and returns to text mode
 * 
 * @return 0 on success, non-zero otherwise
 */
int exit_graphics_mode();

/**
 * @brief Gets the horizontal resolution of the current video mode
 * 
 * @return Horizontal resolution
 */
uint16_t get_h_res();

/**
 * @brief Gets the vertical resolution of the current video mode
 * 
 * @return Vertical resolution
 */
uint16_t get_v_res();

/**
 * @brief Gets the bits per pixel of the current video mode
 * 
 * @return Bits per pixel
 */
uint8_t get_bits_per_pixel();

/**
 * @brief Gets a pointer to the video_info structure
 * 
 * @return Pointer to the vmi_p structure
 */
vbe_mode_info_t *get_vmi_p();

/**
 * @brief Gets a pointer to the video memory
 * 
 * @return Pointer to video memory or NULL if not mapped
 */
void* get_video_mem();

/**
 * @brief Draw a pixel at specified coordinates
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Pixel color
 * @return 0 on success, non-zero otherwise
 */
int draw_pixel(uint16_t x, uint16_t y, uint32_t color);

/**
 * @brief Clear the screen with specified color
 * 
 * @param color Background color
 * @return 0 on success, non-zero otherwise
 */
int clear_screen(uint32_t color);

/**
 * @brief Draw a rectangle border
 * 
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color Border color
 * @param thickness Border thickness
 * @return 0 on success, non-zero otherwise
 */
int draw_rectangle_border(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color, uint8_t thickness);

/**
 * @brief Draw a menu option with border and centered text
 * 
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Rectangle width
 * @param height Rectangle height
 * @param text Text to display inside
 * @param text_color Text color
 * @param border_color Border color
 * @param scale Text scale factor
 * @return 0 on success, non-zero otherwise
 */
int draw_menu_option(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                    const char *text, uint32_t text_color, uint32_t border_color, uint8_t scale);

/**
 * @brief Draw the main page with title and menu options
 * 
 * @return 0 on success, non-zero otherwise
 */
int draw_main_page();

/**
 * @brief Draw leaderboard screen
 * 
 * @return 0 on success, non-zero otherwise
 */
int draw_leaderboard();

/**
 * @brief Draw leaderboard screen with mouse support
 * 
 * @param mouse_x Current mouse X position
 * @param mouse_y Current mouse Y position
 * @return 0 on success, non-zero otherwise
 */
int draw_leaderboard_with_hover(uint16_t mouse_x, uint16_t mouse_y);

/**
 * @brief Draw instructions screen
 * 
 * @return 0 on success, non-zero otherwise
 */
int draw_instructions();

/**
 * @brief Draw instructions screen with mouse support
 * 
 * @param mouse_x Current mouse X position
 * @param mouse_y Current mouse Y position
 * @return 0 on success, non-zero otherwise
 */
int draw_instructions_with_mouse(uint16_t mouse_x, uint16_t mouse_y);

/**
 * @brief Handle mouse clicks on instructions screen
 * 
 * @param x Mouse x coordinate
 * @param y Mouse y coordinate
 * @param left_click True if left button was clicked
 * @return 1 if back button was clicked, -1 if no action
 */
int handle_instructions_click(uint16_t x, uint16_t y, bool left_click);

/**
 * @brief Initialize single player game
 * 
 * @return 0 on success, non-zero otherwise
 */
int draw_init_sp_game();

/**
 * @brief Initialize multiplayer game
 * 
 * @return 0 on success, non-zero otherwise
 */
int draw_init_mp_game();

/**
 * @brief Reset the singleplayer game state
 * Call this when returning to main menu to ensure a new random category is chosen for the next game
 */
void reset_singleplayer();

/**
 * @brief Draw mouse cursor at specified position
 * 
 * @param x Mouse X position
 * @param y Mouse Y position
 * @param color Cursor color
 * @return 0 on success, non-zero otherwise
 */
int draw_mouse_cursor(uint16_t x, uint16_t y, uint32_t color);

/**
 * @brief Draw a filled rectangle
 * 
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Rectangle width
 * @param height Rectangle height
 * @param color Fill color
 * @return 0 on success, non-zero otherwise
 */
int draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

/**
 * @brief Draw a menu option with hover effect
 * 
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Rectangle width
 * @param height Rectangle height
 * @param text Text to display inside
 * @param text_color Text color
 * @param border_color Border color
 * @param scale Text scale factor
 * @param is_hovered True if mouse is hovering over this option
 * @return 0 on success, non-zero otherwise
 */
int draw_menu_option_hover(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                          const char *text, uint32_t text_color, uint32_t border_color, 
                          uint8_t scale, bool is_hovered);

/**
 * @brief Draw the main page with hover effects
 * 
 * @param mouse_x Current mouse X position
 * @param mouse_y Current mouse Y position
 * @return 0 on success, non-zero otherwise
 */
int draw_main_page_with_hover(uint16_t mouse_x, uint16_t mouse_y);

/**
 * @brief Get current game state
 * 
 * @return Current game state
 */
game_state_t get_game_state();

/**
 * @brief Set game state
 * 
 * @param state New game state
 */
void set_game_state(game_state_t state);

/**
 * @brief Draw current page based on game state
 * 
 * @param mouse_x Current mouse X position
 * @param mouse_y Current mouse Y position
 * @return 0 on success, non-zero otherwise
 */
int draw_current_page(uint16_t mouse_x, uint16_t mouse_y);

/**
 * @brief Check if a point is inside a rectangle
 * 
 * @param px Point X coordinate
 * @param py Point Y coordinate
 * @param rx Rectangle X coordinate
 * @param ry Rectangle Y coordinate
 * @param rw Rectangle width
 * @param rh Rectangle height
 * @return True if point is inside rectangle
 */
bool is_point_in_rect(uint16_t px, uint16_t py, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh);

// Declare video_mem as external for other modules to access
extern void *video_mem;

#endif /* _VIDEOCARD_H_ */
