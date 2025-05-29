#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#define MAX_RESPONDIDAS 100
#define MAX_ENTRADA 100

#include "dicionarios.h"

extern int TestMode;

/**
 * @brief Verifica se uma entrada é válida para a categoria
 * 
 * @param categoria Ponteiro para categoria
 * @param respondidas Array de palavras já respondidas
 * @param respondidasCount Ponteiro para contador de respondidas
 * @param entrada Palavra a verificar
 * @return Pontos se válida, -1 se já respondida, 0 se inválida
 */
int verificarEntrada(Categoria* categoria, char respondidas[][MAX_ENTRADA], int* respondidasCount, const char* entrada);

/**
 * @brief Verifica se uma entrada é válida e contém a letra obrigatória
 * 
 * @param categoria Ponteiro para categoria
 * @param respondidas Array de palavras já respondidas
 * @param respondidasCount Ponteiro para contador de respondidas
 * @param entrada Palavra a verificar
 * @param letraObrigatoria Letra que deve estar presente na palavra
 * @return Pontos se válida, -1 se já respondida, 0 se inválida
 */
int verificarEntradaComLetra(Categoria* categoria, char respondidas[][MAX_ENTRADA], int* respondidasCount, const char* entrada, char letraObrigatoria);

/**
 * @brief Verifica se uma palavra contém uma letra específica
 * 
 * @param palavra Palavra a verificar
 * @param letra Letra a procurar
 * @return 1 se contém a letra, 0 caso contrário
 */
int palavraContemLetra(const char* palavra, char letra);

/**
 * @brief Calcula distância de Levenshtein entre duas strings
 * 
 * @param s1 Primeira string
 * @param s2 Segunda string
 * @return Distância de Levenshtein
 */
int distanciaLevenshtein(const char* s1, const char* s2);

/**
 * @brief Remove acentos de uma string
 * 
 * @param str String a processar (modificada in-place)
 */
void removerAcentos(char* str);

#endif