#ifndef _FIGHT_LIST_H_
#define _FIGHT_LIST_H_

#include <stdint.h>
#include <stdbool.h>
#include "dicionarios.h"

#define MAX_RESPONDIDAS 50
#define MAX_ENTRADA 50
#define TIMER_INICIAL 35
#define MAX_INPUT_LENGTH 30

/**
 * @brief Estado do jogo Fight List
 */
typedef struct {
    Categoria *categoria_atual;     /* Categoria selecionada */
    char letra_obrigatoria;         /* Letra que deve estar presente nas palavras */
    int tempo_restante;             /* Tempo em segundos (35 a 0) */
    int pontuacao_total;            /* Pontuação total acumulada */
    char respondidas[MAX_RESPONDIDAS][MAX_ENTRADA]; /* Palavras já respondidas */
    int count_respondidas;          /* Número de palavras respondidas */
    char input_atual[MAX_INPUT_LENGTH]; /* Input atual do jogador */
    int input_length;               /* Comprimento do input atual */
    bool jogo_terminado;            /* Flag se o jogo terminou */
    bool todas_encontradas;         /* Flag se encontrou todas as palavras */
    uint32_t timer_counter;         /* Contador para o timer (60 interrupts = 1 segundo) */
} fight_list_t;

/**
 * @brief Inicializa o jogo Fight List
 * 
 * @param game Ponteiro para estrutura do jogo
 * @param letra Letra obrigatória capturada no letter rain
 * @return 0 em sucesso, não-zero caso contrário
 */
int fight_list_init(fight_list_t *game, char letra);

/**
 * @brief Atualiza o estado do jogo (chamado nos interrupts do timer)
 * 
 * @param game Ponteiro para estrutura do jogo
 * @return 0 se continua, 1 se jogo terminou
 */
int fight_list_update(fight_list_t *game);

/**
 * @brief Desenha a interface do jogo Fight List
 * 
 * @param game Ponteiro para estrutura do jogo
 * @return 0 em sucesso, não-zero caso contrário
 */
int fight_list_draw(fight_list_t *game);

/**
 * @brief Processa input do teclado
 * 
 * @param game Ponteiro para estrutura do jogo
 * @param scancode Código da tecla pressionada
 * @return 0 em sucesso, 1 se palavra submetida, -1 se ação inválida
 */
int fight_list_handle_input(fight_list_t *game, uint8_t scancode);

/**
 * @brief Submete a palavra atual do jogador
 * 
 * @param game Ponteiro para estrutura do jogo
 * @return Pontos ganhos (0 se inválida, -1 se já respondida)
 */
int fight_list_submit_word(fight_list_t *game);

/**
 * @brief Verifica se uma palavra contém a letra obrigatória
 * 
 * @param palavra Palavra a verificar
 * @param letra Letra obrigatória
 * @return true se contém a letra, false caso contrário
 */
bool palavra_contem_letra(const char *palavra, char letra);

/**
 * @brief Limpa recursos do jogo Fight List
 * 
 * @param game Ponteiro para estrutura do jogo
 */
void fight_list_cleanup(fight_list_t *game);

/**
 * @brief Converte scancode para caractere
 * 
 * @param scancode Código da tecla
 * @return Caractere correspondente ou 0 se inválido
 */
char scancode_to_char_fight_list(uint8_t scancode);

/**
 * @brief Seleciona uma categoria aleatória que tenha palavras com a letra
 * 
 * @param letra Letra obrigatória
 * @return Ponteiro para categoria selecionada ou NULL se não encontrar
 */
Categoria* selecionar_categoria_com_letra(char letra);

#endif /* _FIGHT_LIST_H_ */
