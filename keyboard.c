#include "keyboard.h"

#include <lcom/lcf.h>
#include <minix/sysutil.h>

#include <stdint.h>

#define KBD_IRQ 1    /* Keyboard IRQ line */
#define ESC_MAKE 0x01  /* ESC make code */
#define ESC_BREAK 0x81  /* ESC break code */

/* KBC I/O ports */
#define KBC_CMD_REG 0x64  /* Command register */
#define KBC_ST_REG 0x64   /* Status register */
#define KBC_OUT_BUF 0x60  /* Output buffer */

/* KBC status register bits */
#define KBC_OBF BIT(0)    /* Output buffer full */
#define KBC_PARITY BIT(7) /* Parity error */
#define KBC_TIMEOUT BIT(6) /* Timeout error */

static int hook_id = KBD_IRQ;
static uint8_t scancode = 0;
static bool esc_pressed = false;
uint8_t last_scancode = 0; /* Expose last scancode for game logic */

int kbd_subscribe_int(uint8_t *bit_no) {
  *bit_no = hook_id;
  if (sys_irqsetpolicy(KBD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != OK) {
    printf("kbd_subscribe_int(): sys_irqsetpolicy() failed\n");
    return 1;
  }
  return 0;
}

int kbd_unsubscribe_int() {
  if (sys_irqrmpolicy(&hook_id) != OK) {
    printf("kbd_unsubscribe_int(): sys_irqrmpolicy() failed\n");
    return 1;
  }
  return 0;
}

bool is_kbd_interrupt(int ipc_status) {
  return (is_ipc_notify(ipc_status) && (_ENDPOINT_P(ipc_status) == HARDWARE));
}

bool is_esc_key() {
  /* Reset the flag for next check */
  bool was_pressed = esc_pressed;
  esc_pressed = false;
  return was_pressed;
}

int kbd_int_handler() {
  uint8_t status;
  
  /* Read status register */
  if (util_sys_inb(KBC_ST_REG, &status) != OK) {
    printf("kbd_int_handler(): util_sys_inb() failed reading status\n");
    return 1;
  }
  
  /* Check for errors */
  if (status & (KBC_PARITY | KBC_TIMEOUT)) {
    printf("kbd_int_handler(): KBC status error\n");
    return 1;
  }
  
  /* Check if output buffer is full */
  if (status & KBC_OBF) {
    /* Read scancode from output buffer */
    if (util_sys_inb(KBC_OUT_BUF, &scancode) != OK) {
      printf("kbd_int_handler(): util_sys_inb() failed reading scancode\n");
      return 1;
    }
    
    /* Store last scancode for game logic */
    last_scancode = scancode;
    
    /* Check if it's the ESC key */
    if (scancode == ESC_MAKE) {
      esc_pressed = true;
    }
    
    /* For debugging */
    printf("Scancode: 0x%02X\n", scancode);
  }
  
  return 0;
}
