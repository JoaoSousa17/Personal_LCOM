#include <lcom/lcf.h>
#include "i8042.h"
#include "utils.h"

int (mouse_enable_data_reporting_mine)() {
  uint8_t ack = 0;

  do {
    if (kbc_write_command(KBC_WRITE_TO_MOUSE)) return 1;
    if (kbc_write_argument(MOUSE_ENABLE_DATA_REPORT)) return 1;
    if (kbc_read_output(&ack)) return 1;
  } while (ack == MOUSE_NACK);

  return (ack == MOUSE_ACK) ? 0 : 1;
}

int (mouse_disable_data_reporting_mine)() {
  uint8_t ack = 0;

  do {
    if (kbc_write_command(KBC_WRITE_TO_MOUSE)) return 1;
    if (kbc_write_argument(MOUSE_DISABLE_DATA_REPORT)) return 1;
    if (kbc_read_output(&ack)) return 1;
  } while (ack == MOUSE_NACK);

  return (ack == MOUSE_ACK) ? 0 : 1;
}

int (kbc_write_command)(uint8_t cmd) {
  uint8_t status;
  for (int i = 0; i < 10; i++) { // tenta no máximo 10 vezes
    if (util_sys_inb(KBC_STATUS_REG, &status)) return 1;
    if (!(status & KBC_IBF)) { 
      if (sys_outb(KBC_CMD_REG, cmd)) return 1;
      return 0;
    }
    tickdelay(micros_to_ticks(20000)); // espera 20ms
  }
  return 1;
}

int (kbc_write_argument)(uint8_t arg) {
  uint8_t status;
  for (int i = 0; i < 10; i++) {
    if (util_sys_inb(KBC_STATUS_REG, &status)) return 1;
    if (!(status & KBC_IBF)) { 
      if (sys_outb(KBC_IN_BUF, arg)) return 1;
      return 0;
    }
    tickdelay(micros_to_ticks(20000));
  }
  return 1;
}

int (kbc_read_output)(uint8_t *data) {
  uint8_t status;
  for (int i = 0; i < 10; i++) {
    if (util_sys_inb(KBC_STATUS_REG, &status)) return 1;
    if (status & KBC_OBF) {
      if (util_sys_inb(KBC_OUT_BUF, data)) return 1;
      if ((status & (KBC_PARITY_ERROR | KBC_TIMEOUT_ERROR)) == 0) return 0;
      else return 1;
    }
    tickdelay(micros_to_ticks(20000));
  }
  return 1;
}

// Função usada na Primeira Função do Lab
int (util_sys_inb)(int port, uint8_t *value) {
  uint32_t res;
  if (sys_inb(port, &res) != 0) {
    printf("Erro ao ler a porta do Timer");
    return 1;
  }
  *value = res;
  return 0;
}

// Funções usadas na Segunda Função do Lab
int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
  *lsb = (uint8_t) (val & 0xFF);
  return 0;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  *msb = (uint8_t) ((val & 0xFF00) >> 8);
  return 0;
}
