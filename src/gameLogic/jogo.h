#ifndef JOGO_H
#define JOGO_H

#define MAX_RESPONDIDAS 100
#define MAX_ENTRADA 100

#include "dicionarios.h"

extern int TestMode;

int verificarEntrada(Categoria* categoria, char respondidas[][MAX_ENTRADA], int* respondidasCount, const char* entrada);
int distanciaLevenshtein(const char* s1, const char* s2);
void removerAcentos(char* str);

#endif