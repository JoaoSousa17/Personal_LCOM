#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <lcom/lcf.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Subscribe to mouse interrupts
 * 
 * @param bit_no Address of memory to be initialized with the bit number
 * @return 0 on success, non-zero otherwise
 */
int mouse_enable(uint8_t *bit_no);

/**
 * @brief Unsubscribe from mouse interrupts
 * 
 * @return 0 on success, non-zero otherwise
 */
int mouse_disable();

/**
 * @brief Mouse interrupt handler
 */
void mouse_ih_custom();

/**
 * @brief Get the number of processed packets
 * 
 * @return Number of packets
 */
uint32_t mouse_get_counter();

/**
 * @brief Get the last received byte
 * 
 * @return Last byte
 */
uint8_t mouse_get_last_byte();

/**
 * @brief Reset mouse counters
 */
void mouse_reset();

/**
 * @brief Get the last complete packet
 * 
 * @return Last packet
 */
struct packet mouse_get_packet();

/**
 * @brief Check if a packet is ready
 * 
 * @return True if packet is ready, false otherwise
 */
bool mouse_has_packet_ready();

/**
 * @brief Clear the packet ready flag
 */
void mouse_clear_packet_ready();

/**
 * @brief Handle mouse clicks on menu options
 * 
 * @param x Mouse x coordinate
 * @param y Mouse y coordinate
 * @param left_click True if left button was clicked
 * @return 0 on success, 1 if quit was clicked, -1 if no action
 */
int handle_menu_click(uint16_t x, uint16_t y, bool left_click);

/**
 * @brief Get current mouse X position
 * 
 * @return Mouse X coordinate
 */
uint16_t mouse_get_x();

/**
 * @brief Get current mouse Y position
 * 
 * @return Mouse Y coordinate
 */
uint16_t mouse_get_y();

/**
 * @brief Check if menu needs to be redrawn due to mouse movement
 * 
 * @return True if redraw is needed
 */
bool mouse_menu_needs_redraw();

/**
 * @brief Clear the menu redraw flag
 */
void mouse_clear_redraw_flag();

/**
 * @brief Check if we should force a page redraw
 * 
 * @return True if page should be redrawn
 */
bool mouse_should_redraw_page();

/**
 * @brief Clear the page redraw flag
 */
void mouse_clear_page_redraw_flag();

#endif /* _MOUSE_H_ */
