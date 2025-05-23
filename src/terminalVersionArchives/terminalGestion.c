#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "utils.h"
#include "jogo.h"
#include "dicionarios.h"
#include "leaderboard.h"

void mostrarMenu() {
    drawBar();
    printf("\033[1;32mFIGHT LIST - TERMINAL EDITION\033[0m\n");
    drawBar();
    printf("1. Jogar\n");
    printf("2. Regras\n");
    printf("3. Leaderboard\n");
    printf("4. Sair\n");
    drawBar();
    printf("Seleciona a opção: ");
}

void mostrarRegras() {
    drawBar();
    printf("\033[1;33mRegras do Jogo:\033[0m\n");
    printf("-> O jogo escolhe uma categoria aleatória.\n");
    printf("-> Tens 35 segundos para escrever o máximo de palavras dessa categoria.\n");
    printf("-> Cada palavra tem uma pontuação de 1 a 3.\n");
    printf("-> O jogo termina quando acertas todas ou acaba o tempo.\n");
    drawBar();
}

void jogarTerminal() {
    system("clear");

    Categoria* categoria = NULL;
    if (TestMode) {
        for (int i = 0; i < categoriasCount; i++) {
            if (strcmp(categorias[i].nome, "Animais") == 0) {
                categoria = &categorias[i];
                break;
            }
        }
    } else {
        srand(time(NULL));
        int categoriaIndex = rand() % categoriasCount;
        categoria = &categorias[categoriaIndex];
    }

    drawBar();
    printf("Categoria: \033[1;34m%s\033[0m\n", categoria->nome);
    drawBar();
    printf("Tens 35 segundos! Escreve palavras (pressiona Enter após cada uma).\n");
    printf("Nota: o tempo termina após carregares Enter, mesmo que os 35 segundos já tenham passado.\n");

    char entrada[MAX_ENTRADA];
    char respondidas[MAX_RESPONDIDAS][MAX_ENTRADA];
    int respondidasCount = 0;
    int pontuacao = 0;

    time_t inicio = time(NULL);

    while (difftime(time(NULL), inicio) < 35 && respondidasCount < categoria->totalPalavras) {
        printf("> ");
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) break;
        entrada[strcspn(entrada, "\n")] = '\0';

        for (int i = 0; entrada[i]; i++)
            entrada[i] = tolower((unsigned char)entrada[i]);

        int pontos = verificarEntrada(categoria, respondidas, &respondidasCount, entrada);

        if (pontos == -1) {
            printf("Já respondeste essa!\n");
        } else if (pontos > 0) {
            pontuacao += pontos;
            printf("✔ Correto! +%d pontos.\n", pontos);
        } else {
            printf("✘ Palavra inválida.\n");
        }
    }

    drawBar();
    printf("\033[1;32mFim de jogo!\033[0m\n");
    printf("Palavras corretas: %d\n", respondidasCount);
    printf("Pontuação final: \033[1;35m%d\033[0m\n", pontuacao);
    drawBar();

    char iniciais[4];
    printf("Insere as tuas iniciais (3 letras): ");
    fgets(iniciais, sizeof(iniciais), stdin);
    iniciais[strcspn(iniciais, "\n")] = '\0';

    // limpar stdin depois da leitura importante
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    guardarPontuacao(pontuacao, iniciais, categoria->nome);
}