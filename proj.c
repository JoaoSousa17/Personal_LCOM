#include <lcom/lcf.h>
#include <stdint.h>
#include <stdio.h>
#include "videocard.h"
#include "keyboard.h"

uint16_t mode;
uint8_t kbd_bit_no = 0;

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
          if (msg.m_notify.interrupts & BIT(kbd_bit_no)) {
            /* Handle keyboard interrupt */
            kbd_int_handler();
            
            /* Check if ESC key was pressed */
            if (is_esc_key()) {
              printf("ESC key detected, exiting graphics mode...\n");
              running = false;
            }
          }
          break;
        default:
          break;
      }
    }
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

