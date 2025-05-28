#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <stdint.h>
#include <stdbool.h>

void guardarPontuacao(int pontuacao, const char* iniciais, const char* categoria);
void mostrarLeaderboard();
int draw_leaderboard_graphics(); // Função para desenhar leaderboard básico
int draw_leaderboard_with_mouse(uint16_t mouse_x, uint16_t mouse_y); // Função para desenhar com mouse e botão back
int handle_leaderboard_click(uint16_t x, uint16_t y, bool left_click); // Função para lidar com cliques

#endif
