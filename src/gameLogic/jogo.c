#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "jogo.h"
#include "dicionarios.h"

int TestMode = 0;

void removerAcentos(char* str) {
    char* p = str;
    while (*p) {
        switch ((unsigned char)*p) {
            case 0xc3:
                switch ((unsigned char)*(p + 1)) {
                    case 0xa1: case 0xa0: case 0xa2: case 0xa3: *p = 'a'; break;
                    case 0xa9: case 0xaa: *p = 'e'; break;
                    case 0xad: *p = 'i'; break;
                    case 0xb3: case 0xb4: case 0xb5: *p = 'o'; break;
                    case 0xba: *p = 'u'; break;
                    case 0xa7: *p = 'c'; break;
                    default: break;
                }
                memmove(p + 1, p + 2, strlen(p + 2) + 1);
                break;
            default:
                p++;
        }
    }
}

int distanciaLevenshtein(const char* s1, const char* s2) {
    int len1 = strlen(s1), len2 = strlen(s2);
    if (abs(len1 - len2) > 1) return 2;

    int dp[len1 + 1][len2 + 1];
    for (int i = 0; i <= len1; i++) dp[i][0] = i;
    for (int j = 0; j <= len2; j++) dp[0][j] = j;

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            int insert = dp[i][j - 1] + 1;
            int del = dp[i - 1][j] + 1;
            int replace = dp[i - 1][j - 1] + cost;
            dp[i][j] = (insert < del) ? (insert < replace ? insert : replace)
                                      : (del < replace ? del : replace);
        }
    }

    return dp[len1][len2];
}

int verificarEntrada(Categoria* categoria, char respondidas[][MAX_ENTRADA], int* respondidasCount, const char* entradaOriginal) {
    char entrada[MAX_ENTRADA];
    strncpy(entrada, entradaOriginal, MAX_ENTRADA);

    for (int i = 0; entrada[i]; i++)
        entrada[i] = tolower((unsigned char)entrada[i]);
    removerAcentos(entrada);

    for (int i = 0; i < *respondidasCount; i++) {
        char comparada[MAX_ENTRADA];
        strncpy(comparada, respondidas[i], MAX_ENTRADA);
        removerAcentos(comparada);
        if (strcmp(comparada, entrada) == 0)
            return -1;
    }

    for (int i = 0; i < categoria->totalPontuacoes; i++) {
        char correta[MAX_ENTRADA];
        strncpy(correta, categoria->pontuacoes[i].palavra, MAX_ENTRADA);
        for (int j = 0; correta[j]; j++)
            correta[j] = tolower((unsigned char)correta[j]);
        removerAcentos(correta);

        if (distanciaLevenshtein(entrada, correta) <= 1) {
            strncpy(respondidas[*respondidasCount], categoria->pontuacoes[i].palavra, MAX_ENTRADA);
            (*respondidasCount)++;
            return categoria->pontuacoes[i].pontuacao;
        }
    }

    return 0;
}