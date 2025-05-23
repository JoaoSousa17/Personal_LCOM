#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>
#include "i8254.h"

int timerCounter;
static int hook_id;

// Funções aux para Primeira Função do Lab
int (timer_get_conf)(uint8_t timer, uint8_t *st) {
  uint8_t code = TIMER_RB_CMD | TIMER_RB_SEL(timer) | TIMER_RB_COUNT_;

  // Escreve o comando para a porta de controlo 0x43
  if(sys_outb(TIMER_CTRL, code) != 0){
      printf("Erro ao escrever o código para leitura do Timer");
      return 1;
    };

  // Lê o resultado do comando, na porta do timer
  switch (timer){
  case 0:
    if(util_sys_inb(TIMER_0, st) != 0){
      printf("Erro ao ler da Porta do Timer");
      return 1;
    };
    break;
  case 1:
    if(util_sys_inb(TIMER_1, st) != 0){
      printf("Erro ao ler da Porta do Timer");
      return 1;
    };
    break;
  case 2:
    if(util_sys_inb(TIMER_2, st) != 0){
      printf("Erro ao ler da Porta do Timer");
      return 1;
    };
    break;
  }
  return 0;
}

int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {  
  union timer_status_field_val res;
  switch(field){
    case tsf_all:
      res.byte = st;
      break;
    case tsf_initial:
      res.in_mode = (st & (BIT(4) | BIT(5))) >> 4;
      break;
    case tsf_mode:
      res.count_mode = (st & (BIT(1) | BIT(2) | BIT(3))) >> 1;
      if (res.count_mode > 5) res.count_mode &= 0x03;
      break;
    case tsf_base:
      res.bcd = st & 0x01;
  }
  return timer_print_config(timer, field, res);
}

// Função aux para Segunda Função do Lab
int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  // Preparação para escrita na porta de Controlo - Código de Configuração
  uint8_t st;
  timer_get_conf(timer, &st);
  uint8_t code = (st & 0x0F);
  code = code | BIT(4) | BIT(5);

  // Preparação para escrita na porta do Timer - Alterar Frequência
  uint16_t period = TIMER_FREQ / freq;
  uint8_t lsb, msb;
  util_get_LSB(period, &lsb);
  util_get_MSB(period, &msb);

  // Escrita efetiva nas respetivas portas + finalização do codigo a enviar para a porta de Controlo
  switch (timer){
    case 0:
      sys_outb(TIMER_CTRL, code);
      sys_outb(TIMER_0, lsb);
      sys_outb(TIMER_0, msb);
      return 0;
    case 1:
      code = code & BIT(6);
      sys_outb(TIMER_CTRL, code);
      sys_outb(TIMER_1, lsb);
      sys_outb(TIMER_1, msb);
      return 0;
    case 2:
      code = code & BIT(7);
      sys_outb(TIMER_CTRL, code);
      sys_outb(TIMER_2, lsb);
      sys_outb(TIMER_2, msb);
      return 0;
  }
  return 1;
}

// Funções auxiliares para a Terceira Função do Lab
// Subscreve os interrupts do Timer0
int (timer_subscribe_int)(uint8_t *bit_no) {
  hook_id = TIMER0_IRQ;
  *bit_no = hook_id;
  if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id)){
    printf("Erro ao obter código do processo de Interrupt");
    return 1;
  }
  return 0;
}

// Deixa de subscrever os interrupts do Timer0
int (timer_unsubscribe_int)() {
  if(sys_irqrmpolicy(&hook_id)){
    printf("Erro ao cancelar a subscrição das interrupções.");
    return 1;
  }
  return 1;
}

// Incrementa o timer;
void (timer_int_handler)() {
  timerCounter++;
}
