#include <lcom/lcf.h>
#include <stdint.h>
#include "i8042.h"
#include "utils.h"

static int hook_id;

static uint8_t mouse_byte;
static int mouse_bytes_counter = 0;
static uint8_t packet_bytes[3];
static uint32_t mouseCounter = 0;

// Subscreve os interrupts do rato e ativa o data reporting
int (mouse_enable)(uint8_t *bit_no) {
  hook_id = MOUSE_IRQ;
  *bit_no = hook_id;
  
  if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id)) {
    printf("Erro ao subscrever as interrupções do rato.\n");
    return 1;
  }
  
  if (mouse_enable_data_reporting_mine()) {
    printf("Erro ao ativar o data reporting do rato.\n");
    return 1;
  }
  
  return 0;
}

// Reverte todas as configurações de mouse_enable
int (mouse_disable)() {
  if (mouse_disable_data_reporting_mine()) {
    printf("Erro ao desativar o data reporting do rato.\n");
    return 1;
  }
  
  if (sys_irqrmpolicy(&hook_id)) {
    printf("Erro ao cancelar a subscrição das interrupções do rato.\n");
    return 1;
  }
  
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
void (mouse_ih)() {
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
                mouse_print_packet(&pp);
                last_packet = pp; // Guarda o último pacote completo
                packet_ready = true;


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
