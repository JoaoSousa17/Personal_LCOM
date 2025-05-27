/*#include <lcom/lab3.h>
#include <lcom/lcf.h>

#include <stdint.h>
#include "i8042.h"*/

#include <lcom/lcf.h>

#include <stdint.h>

int global_counter = 0;

extern int(util_get_LSB)(uint16_t val, uint8_t *lsb){
  val = (0x00FF & val);
  *lsb = (uint8_t) val;
  return 0;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  val = (val & 0xFF00);
  *msb = val >> 8;
  return 0;
}

int(util_sys_inb)(int port, uint8_t *value) {
  uint32_t value2 = 0x00000000;
  if (sys_inb(port, &value2) == OK)
  {
    *value = (uint8_t) value2;
    
    
    //#define LAB3
    #ifdef LAB3
    global_counter++;
    #endif
    return 1;
  }
  return 0;
}
