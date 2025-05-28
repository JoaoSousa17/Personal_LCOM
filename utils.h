#ifndef _UTILS_H_
#define _UTILS_H_

#include <lcom/lcf.h>

// Funções para escrever e ler do KBC
int (kbc_write_command)(uint8_t cmd);
int (kbc_write_argument)(uint8_t arg);
int (kbc_read_output)(uint8_t *data);

// Funções para controlar o data reporting do rato
int (mouse_enable_data_reporting_mine)();
int (mouse_disable_data_reporting_mine)();
int (util_sys_inb)(int port, uint8_t *value);

#endif /* _UTILS_H_ */
