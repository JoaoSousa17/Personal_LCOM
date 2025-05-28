#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>

/* Expose last scancode for game logic */
extern uint8_t last_scancode;

/**
 * @brief Checks if an interrupt is from the keyboard
 * 
 * @param ipc_status IPC status from driver_receive
 * @return True if it's a keyboard interrupt, false otherwise
 */
bool is_kbd_interrupt(int ipc_status);

/**
 * @brief Checks if the ESC key was pressed
 * 
 * @return True if ESC key was pressed, false otherwise
 */
bool is_esc_key();

/**
 * @brief Subscribe to keyboard interrupts
 * 
 * @param bit_no Address of memory to be initialized with the bit number to be set in hook_id
 * @return Return 0 upon success and non-zero otherwise
 */
int kbd_subscribe_int(uint8_t *bit_no);

/**
 * @brief Unsubscribe from keyboard interrupts
 * 
 * @return Return 0 upon success and non-zero otherwise
 */
int kbd_unsubscribe_int();

/**
 * @brief Handles a keyboard interrupt by reading the scancode
 * 
 * @return Return 0 upon success and non-zero otherwise
 */
int kbd_int_handler();

#endif /* _KEYBOARD_H_ */
