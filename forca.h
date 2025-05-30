#ifndef _FORCA_H_
#define _FORCA_H_

#include <stdint.h>
#include <stdbool.h>

#define PALAVRA_FORCA "laboratorio de computadores"
#define PALAVRA_TAMANHO 26
#define MAX_VIDAS 5

/**
 * @brief Estado do jogo da forca
 */
typedef struct {
    char palavra[PALAVRA_TAMANHO + 1];     // Palavra a adivinhar
    char adivinhada[PALAVRA_TAMANHO + 1];  // Palavra com letras descobertas
    bool letras_usadas[26];                // Letras do alfabeto já usadas
    int vidas_restantes;                   // Vidas restantes (corações)
    bool jogo_ganho;                       // True se ganhou
    bool jogo_perdido;                     // True se perdeu
    bool primeiro_desenho;                 // Flag para primeiro desenho
} forca_game_t;

/**
 * @brief Inicializa o jogo da forca
 * 
 * @param game Ponteiro para o estado do jogo
 * @return 0 se sucesso, não-zero caso contrário
 */
int forca_init(forca_game_t *game);

/**
 * @brief Atualiza o estado do jogo da forca
 * 
 * @param game Ponteiro para o estado do jogo
 * @return 0 se jogo continua, 1 se terminou
 */
int forca_update(forca_game_t *game);

/**
 * @brief Desenha o jogo da forca
 * 
 * @param game Ponteiro para o estado do jogo
 * @return 0 se sucesso, não-zero caso contrário
 */
int forca_draw(forca_game_t *game);

/**
 * @brief Processa input do teclado para o jogo da forca
 * 
 * @param game Ponteiro para o estado do jogo
 * @param scancode Código da tecla pressionada
 * @return 0 se sucesso, não-zero caso contrário
 */
int forca_handle_input(forca_game_t *game, uint8_t scancode);

/**
 * @brief Limpa recursos do jogo da forca
 * 
 * @param game Ponteiro para o estado do jogo
 */
void forca_cleanup(forca_game_t *game);

/**
 * @brief Verifica se uma letra está na palavra
 * 
 * @param game Ponteiro para o estado do jogo
 * @param letra Letra a verificar
 * @return true se a letra está na palavra, false caso contrário
 */
bool forca_verificar_letra(forca_game_t *game, char letra);

/**
 * @brief Atualiza a palavra adivinhada com uma nova letra
 * 
 * @param game Ponteiro para o estado do jogo
 * @param letra Letra descoberta
 */
void forca_revelar_letra(forca_game_t *game, char letra);

/**
 * @brief Verifica se o jogo foi ganho (palavra completa)
 * 
 * @param game Ponteiro para o estado do jogo
 * @return true se ganhou, false caso contrário
 */
bool forca_jogo_ganho(forca_game_t *game);

#endif /* _FORCA_H_ */
