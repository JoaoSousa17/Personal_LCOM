#include <lcom/lcf.h>
#include <lcom/liblm.h>
#include <lcom/proj.h>

#include <stdbool.h>
#include <stdint.h>

#include "videocard.h"
#include "keyboard.h"

// Define the video mode (mode 0x105 is 1024x768, 256 colors)
#define VIDEO_MODE 0x105

int(proj_main_loop)(int argc, char* argv[]) {
  /* Initialize service */
  uint8_t kbd_bit_no;
  
  /* Subscribe to keyboard interrupts */
  if (kbd_subscribe_int(&kbd_bit_no) != 0) {
    printf("Failed to subscribe to keyboard interrupts\n");
    return 1;
  }
  
  /* Initialize video card in graphics mode */
  if (map_vram(VIDEO_MODE) != 0) {
    printf("Failed to map VRAM\n");
    return 1;
  }
  
  if (set_graphics_mode(VIDEO_MODE) != 0) {
    printf("Failed to set graphics mode\n");
    return 1;
  }
  
  printf("Initialized graphics mode 0x%X: %dx%d, %d bits per pixel\n", 
         VIDEO_MODE, get_h_res(), get_v_res(), get_bits_per_pixel());
  
  /* Run until interrupted */
  bool running = true;
  
  while (running) {
    /* Get event from any interrupt handler */
    int ipc_status;
    message msg;
    
    if (driver_receive(ANY, &msg, &ipc_status) != 0) {
      printf("driver_receive failed\n");
      continue;
    }
    
    /* Handle the event, update state, etc. */
    if (is_kbd_interrupt(ipc_status)) {
      // Check if it's the keyboard interrupt by checking the bit
      if (msg.m_notify.interrupts & BIT(kbd_bit_no)) {
        // Handle keyboard interrupt
        if (kbd_int_handler() != 0) {
          printf("Failed to handle keyboard interrupt\n");
        }
        
        // Check if ESC key was pressed
        if (is_esc_key()) {
          printf("ESC key pressed. Exiting...\n");
          running = false;
        }
      }
    }
    
    /* Draw updated state to screen */
    // Draw code here
  }

  /* Cleanup resources */
  if (vg_exit() != OK) {
    printf("Failed to exit graphics mode\n");
    return 1;
  }
  
  /* Unsubscribe from keyboard interrupts */
  if (kbd_unsubscribe_int() != 0) {
    printf("Failed to unsubscribe from keyboard interrupts\n");
    return 1;
  }
  
  return 0;
} 
