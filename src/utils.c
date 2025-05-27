#include <lcom/lcf.h>
#include <stdint.h>
#include "utils.h"

int global_counter = 0;

int util_get_LSB(uint16_t val, uint8_t *lsb) {
  *lsb = (uint8_t)(val & 0xFF);
  return 0;
}

int util_get_MSB(uint16_t val, uint8_t *msb) {
  *lsb = (uint8_t)((val >> 8) & 0xFF);
  return 0;
}

int util_sys_inb(int port, uint8_t *value) {
  uint32_t value32;
  if (sys_inb(port, &value32) != OK) {
    return 1; // Erro
  }
  
  *value = (uint8_t)value32;
  
  #ifdef LAB3
  global_counter++;
  #endif
  
  return 0; // Sucesso
}
