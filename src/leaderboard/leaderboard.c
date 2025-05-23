#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leaderboard.h"
#include "utils.h"

#define MAX_ENTRADAS 5
#define FICHEIRO "storedData/leaderboard.txt"

typedef struct {
    char iniciais[4];
    char categoria[50];
    int pontuacao;
} EntradaLeaderboard;

void guardarPontuacao(int pontuacao, const char* iniciais, const char* categoria) {
    EntradaLeaderboard entradas[MAX_ENTRADAS + 1];
    int count = 0;

    FILE* f = fopen(FICHEIRO, "r");
    if (f) {
        while (count < MAX_ENTRADAS && fscanf(f, "%3s \"%49[^\"]\" %d", entradas[count].iniciais, entradas[count].categoria, &entradas[count].pontuacao) == 3) {
            count++;
        }
        fclose(f);
    }

    // Se ainda temos menos de 5 entradas, entra diretamente
    if (count < MAX_ENTRADAS) {
        strncpy(entradas[count].iniciais, iniciais, 3);
        entradas[count].iniciais[3] = '\0';

        // Limpa aspas da categoria
        char categoriaLimpa[50];
        int j = 0;
        for (int i = 0; categoria[i] && j < 49; i++) {
            if (categoria[i] != '"') {
                categoriaLimpa[j++] = categoria[i];
            }
        }
        categoriaLimpa[j] = '\0';

        strncpy(entradas[count].categoria, categoriaLimpa, 49);
        entradas[count].categoria[49] = '\0';

        entradas[count].pontuacao = pontuacao;
        count++;
    }
    // Se já temos 5, só substitui se for melhor que o pior
    else {
        int piorIndex = 0;
        for (int i = 1; i < count; i++) {
            if (entradas[i].pontuacao < entradas[piorIndex].pontuacao) {
                piorIndex = i;
            }
        }

        if (pontuacao > entradas[piorIndex].pontuacao) {
            strncpy(entradas[piorIndex].iniciais, iniciais, 3);
            entradas[piorIndex].iniciais[3] = '\0';

            char categoriaLimpa[50];
            int j = 0;
            for (int i = 0; categoria[i] && j < 49; i++) {
                if (categoria[i] != '"') {
                    categoriaLimpa[j++] = categoria[i];
                }
            }
            categoriaLimpa[j] = '\0';

            strncpy(entradas[piorIndex].categoria, categoriaLimpa, 49);
            entradas[piorIndex].categoria[49] = '\0';

            entradas[piorIndex].pontuacao = pontuacao;
        } else {
            return; // Não entra no top 5
        }
    }

    // Ordenar por pontuação (descendente)
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (entradas[j].pontuacao > entradas[i].pontuacao) {
                EntradaLeaderboard tmp = entradas[i];
                entradas[i] = entradas[j];
                entradas[j] = tmp;
            }
        }
    }

    // Escrever os top 5
    f = fopen(FICHEIRO, "w");
    if (!f) return;

    for (int i = 0; i < count && i < MAX_ENTRADAS; i++) {
        fprintf(f, "%s \"%s\" %d\n", entradas[i].iniciais, entradas[i].categoria, entradas[i].pontuacao);
    }

    fclose(f);
}

void mostrarLeaderboard() {
    FILE* f = fopen(FICHEIRO, "r");
    if (!f) {
        printf("Nenhum registo encontrado.\n");
        return;
    }

    drawBar();
    printf("\033[1;36mLEADERBOARD - TOP 5\033[0m\n");
    drawBar();

    char linha[128];
    char iniciais[4], categoria[50];
    int pontos, pos = 1;

while (fgets(linha, sizeof(linha), f)) {
    linha[strcspn(linha, "\r\n")] = '\0';

    // Lê categoria entre aspas
    if (sscanf(linha, "%3s \"%49[^\"]\" %d", iniciais, categoria, &pontos) == 3) {
        printf("%d. %s - %s - %d pontos\n", pos++, iniciais, categoria, pontos);
    } else {
        printf("Linha inválida: %s\n", linha);
    }
}


    drawBar();
    fclose(f);
}

