#include <lcom/lcf.h>
#include <stdint.h>
#include <stdbool.h>
#include "i8042.h"
#include "utils.h"
#include "mouse.h"
#include "videocard.h"

static int hook_id;
static uint8_t mouse_byte;
static int mouse_bytes_counter = 0;
static uint8_t packet_bytes[3];
static uint32_t mouseCounter = 0;

// Mouse position tracking
static uint16_t mouse_x = 400;  // Start at center
static uint16_t mouse_y = 300;
static bool menu_needs_redraw = true;
static bool page_needs_redraw = false;

// Subscreve os interrupts do rato e ativa o data reporting
int (mouse_enable)(uint8_t *bit_no) {
  /* Reset hook_id to original value */
  hook_id = MOUSE_IRQ;
  *bit_no = hook_id;
  
  if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id)) {
    printf("mouse_enable(): sys_irqsetpolicy() failed\n");
    return 1;
  }
  
  if (mouse_enable_data_reporting_mine()) {
    printf("mouse_enable(): mouse_enable_data_reporting_mine() failed\n");
    sys_irqrmpolicy(&hook_id);
    return 1;
  }
  
  printf("mouse_enable(): Successfully subscribed with hook_id=%d\n", hook_id);
  return 0;
}

// Reverte todas as configurações de mouse_enable
int (mouse_disable)() {
  if (mouse_disable_data_reporting_mine()) {
    printf("mouse_disable(): mouse_disable_data_reporting_mine() failed\n");
  }
  
  if (sys_irqrmpolicy(&hook_id)) {
    printf("mouse_disable(): sys_irqrmpolicy() failed\n");
    return 1;
  }
  
  printf("mouse_disable(): Successfully unsubscribed\n");
  return 0;
}

// Constrói manualmente o struct packet a partir dos 3 bytes recebidos
static void (build_packet)(struct packet *pp, uint8_t *bytes) {
    pp->bytes[0] = bytes[0];
    pp->bytes[1] = bytes[1];
    pp->bytes[2] = bytes[2];
    pp->lb = (bytes[0] & BIT(0)) != 0;
    pp->rb = (bytes[0] & BIT(1)) != 0;
    pp->mb = (bytes[0] & BIT(2)) != 0;
    pp->x_ov = (bytes[0] & BIT(6)) != 0;
    pp->y_ov = (bytes[0] & BIT(7)) != 0;
    // Verificar sinais
    pp->delta_x = (bytes[0] & BIT(4)) ? (int16_t)(bytes[1] | 0xFF00) : (int16_t)(bytes[1]);
    pp->delta_y = (bytes[0] & BIT(5)) ? (int16_t)(bytes[2] | 0xFF00) : (int16_t)(bytes[2]);
}

static struct packet last_packet;
static bool packet_ready = false;

// Interrupt handler do rato
void (mouse_ih_custom)() {
    uint8_t status;
    util_sys_inb(KBC_STATUS_REG, &status);
    if (status & KBC_OBF) {
        if (status & KBC_AUX) { // Garantir que é do rato
            util_sys_inb(KBC_OUT_BUF, &mouse_byte);
            // Se for o primeiro byte, confirmar que bit 3 está a 1
            if (mouse_bytes_counter == 0 && !(mouse_byte & BIT(3))) {
                return; // Não sincronizado, ignorar
            }
            packet_bytes[mouse_bytes_counter] = mouse_byte;
            mouse_bytes_counter++;
            if (mouse_bytes_counter == 3) { // Pacote completo
                struct packet pp;
                build_packet(&pp, packet_bytes);
                /* Removed mouse_print_packet for cleaner output */
                last_packet = pp; // Guarda o último pacote completo
                packet_ready = true;
                
                // Update mouse position
                mouse_x += pp.delta_x;
                mouse_y -= pp.delta_y; // Y is inverted
                
                // Keep mouse within screen bounds
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_x >= get_h_res()) mouse_x = get_h_res() - 1;
                if (mouse_y >= get_v_res()) mouse_y = get_v_res() - 1;
                
                // Mark that menu needs redraw due to mouse movement
                if (get_game_state() == STATE_MAIN_MENU) {
                    menu_needs_redraw = true;
                    page_needs_redraw = true;
                } else {
                    // For other states, only redraw if there was movement
                    if (pp.delta_x != 0 || pp.delta_y != 0) {
                        menu_needs_redraw = true; 
                    }
                }
                
                mouse_bytes_counter = 0; // Reset contador
                mouseCounter++; // Mais um pacote processado
            }
        }
    }
}

// Devolve quantos pacotes já foram processados
uint32_t (mouse_get_counter)() {
    return mouseCounter;
}

uint8_t (mouse_get_last_byte)() {
  return mouse_byte;
}

// Reseta os contadores internos
void (mouse_reset)() {
    mouseCounter = 0;
    mouse_bytes_counter = 0;
}

struct packet (mouse_get_packet)() {
    return last_packet;
}

bool (mouse_has_packet_ready)() {
    return packet_ready;
}

void (mouse_clear_packet_ready)() {
    packet_ready = false;
}

int handle_menu_click(uint16_t x, uint16_t y, bool left_click) {
    if (!left_click) return -1;  // Only handle left clicks
    
    uint16_t screen_width = get_h_res();
    uint16_t center_x = screen_width / 2;
    
    // Menu options dimensions (match draw_main_page_with_hover)
    uint16_t option_width = 230;
    uint16_t option_height = 60;
    uint16_t spacing = 20;
    
    // Calculate starting positions
    uint16_t start_y = 140; // Approximate position after title
    
    // First row positions
    uint16_t row1_y = start_y;
    uint16_t single_x = center_x - option_width - spacing/2;
    uint16_t multi_x = center_x + spacing/2;
    
    // Second row positions
    uint16_t row2_y = row1_y + option_height + spacing + 10;
    
    // Third row positions (Quit)
    uint16_t row3_y = row2_y + option_height + spacing + 20;
    uint16_t quit_width = option_width * 2 + spacing;
    uint16_t quit_x = center_x - quit_width/2;
    
    // Check Single Player
    if (x >= single_x && x <= single_x + option_width &&
        y >= row1_y && y <= row1_y + option_height) {
        printf("Single Player clicked!\n");
        set_game_state(STATE_SINGLE_PLAYER);
        page_needs_redraw = true;
        return 0;
    }
    
    // Check 2 Player
    if (x >= multi_x && x <= multi_x + option_width &&
        y >= row1_y && y <= row1_y + option_height) {
        printf("2 Player clicked!\n");
        set_game_state(STATE_MULTIPLAYER);
        page_needs_redraw = true;
        return 0;
    }
    
    // Check Leaderboard
    if (x >= single_x && x <= single_x + option_width &&
        y >= row2_y && y <= row2_y + option_height) {
        printf("Leaderboard clicked!\n");
        set_game_state(STATE_LEADERBOARD);
        page_needs_redraw = true;
        return 0;
    }
    
    // Check Instructions
    if (x >= multi_x && x <= multi_x + option_width &&
        y >= row2_y && y <= row2_y + option_height) {
        printf("Instructions clicked!\n");
        set_game_state(STATE_INSTRUCTIONS);
        page_needs_redraw = true;
        return 0;
    }
    
    // Check Quit
    if (x >= quit_x && x <= quit_x + quit_width &&
        y >= row3_y && y <= row3_y + option_height) {
        printf("Quit game requested!\n");
        return 1; // Signal quit
    }
    
    return -1; // No menu option clicked
}

uint16_t mouse_get_x() {
    return mouse_x;
}

uint16_t mouse_get_y() {
    return mouse_y;
}

bool mouse_menu_needs_redraw() {
    return menu_needs_redraw;
}

void mouse_clear_redraw_flag() {
    menu_needs_redraw = false;
}

bool mouse_should_redraw_page() {
    return page_needs_redraw;
}

void mouse_clear_page_redraw_flag() {
    page_needs_redraw = false;
}
