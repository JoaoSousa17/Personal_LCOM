#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>
#include "i8042.h"

#define LAB3

static int hook_id;
bool done = false;
uint8_t byte_arr[5];
int size = 0;
bool make = true;

int (keyboard_subscribe_int)(uint8_t *bit_no) {
    hook_id = KEYBOARD_IRQ;
    *bit_no = hook_id;
    if (sys_irqsetpolicy(KEYBOARD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id)){
      printf("Erro ao obter código do processo de Interrupt");
      return 1;
    }
    return 0;
}

int (kbd_read_data)(){
    uint8_t data;
    uint8_t stat = 0;
    uint8_t st;
    while( 1 ) {
        if(util_sys_inb(STATUS_REG, &st)){
            printf("Erro ao ler o Status Register.");
            return -2;
        };
        /* loop while 8042 output buffer is empty */
        if( stat & KBC_OBF ) {
            util_sys_inb(OUTPUT_BUF, &data); /* ass. it returns OK */
            if ( (stat &(KBC_PAR_ERR | KBC_TO_ERR)) == 0 )
                return data;
            else{
                printf("Erro ao ler o byte do ScanCode.");
                return -1;
            }
                
        }
        tickdelay(WAIT_KBC); // e.g. tickdelay()
        }
}

void (kbc_ih)(){
    uint8_t st = kbd_read_data();
    if (st < 0) return;
    byte_arr[size] = st;
    size++;
    if (st & BIT(7)) make = false;
    else make = true;
    if (st == 0xE0) done = false;
    else done = true;
}
