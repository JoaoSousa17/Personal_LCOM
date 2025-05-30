#include "forca.h"
#include "videocard.h"
#include "font.h"
#include <string.h>
#include <ctype.h>

/* Scancodes das letras A-Z */
static const uint8_t letter_scancodes[26] = {
    0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
    0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
    0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
};

int forca_init(forca_game_t *game) {
    if (game == NULL) return 1;
    
    /* Inicializar palavra */
    strcpy(game->palavra, PALAVRA_FORCA);
    
    /* Inicializar palavra adivinhada com underscores e espaços */
    for (int i = 0; i < PALAVRA_TAMANHO; i++) {
        if (game->palavra[i] == ' ') {
            game->adivinhada[i] = ' '; /* Espaços são revelados automaticamente */
        } else {
            game->adivinhada[i] = '_'; /* Letras são underscores */
        }
    }
    game->adivinhada[PALAVRA_TAMANHO] = '\0';
    
    /* Resetar letras usadas */
    for (int i = 0; i < 26; i++) {
        game->letras_usadas[i] = false;
    }
    
    /* Inicializar estado do jogo */
    game->vidas_restantes = MAX_VIDAS;
    game->jogo_ganho = false;
    game->jogo_perdido = false;
    game->primeiro_desenho = true;
    
    return 0;
}

int forca_update(forca_game_t *game) {
    if (game == NULL) return 1;
    
    /* Verificar se ganhou */
    if (forca_jogo_ganho(game)) {
        game->jogo_ganho = true;
        return 1; /* Jogo terminou */
    }
    
    /* Verificar se perdeu */
    if (game->vidas_restantes <= 0) {
        game->jogo_perdido = true;
        return 1; /* Jogo terminou */
    }
    
    return 0; /* Jogo continua */
}

int forca_draw(forca_game_t *game) {
    if (game == NULL) return 1;
    
    /* Limpar tela apenas no primeiro desenho */
    if (game->primeiro_desenho) {
        if (clear_screen(0x2c3e50) != 0) return 1;
        game->primeiro_desenho = false;
    }
    
    /* Área de desenho fixa para evitar piscar */
    int area_x = 50;
    int area_y = 50;
    int area_width = 700;
    int area_height = 500;
    
    /* Limpar área de jogo */
    if (draw_filled_rectangle(area_x, area_y, area_width, area_height, 0x2c3e50) != 0) return 1;
    
    /* Título */
    if (draw_string_scaled(area_x + 200, area_y + 20, "JOGO DA FORCA", 0xf39c12, 3) != 0) return 1;
    if (draw_string_scaled(area_x + 250, area_y + 60, "Easter Egg!", 0xe74c3c, 2) != 0) return 1;
    
    /* Desenhar corações (vidas) */
    int coracao_x = area_x + 50;
    int coracao_y = area_y + 120;
    
    if (draw_string_scaled(coracao_x, coracao_y, "Vidas: ", 0xffffff, 2) != 0) return 1;
    
    for (int i = 0; i < MAX_VIDAS; i++) {
        uint32_t cor = (i < game->vidas_restantes) ? 0xe74c3c : 0x7f8c8d; /* Vermelho se vivo, cinza se morto */
        char coracao[2] = "♥";
        if (draw_string_scaled(coracao_x + 120 + (i * 40), coracao_y, coracao, cor, 3) != 0) return 1;
    }
    
    /* Desenhar palavra adivinhada */
    int palavra_y = area_y + 200;
    if (draw_string_scaled(area_x + 50, palavra_y - 30, "Palavra:", 0xffffff, 2) != 0) return 1;
    
    /* Desenhar cada letra da palavra com espaçamento */
    for (int i = 0; i < PALAVRA_TAMANHO; i++) {
        char letra_str[2] = {game->adivinhada[i], '\0'};
        int letra_x = area_x + 50 + (i * 25);
        
        uint32_t cor = 0x3498db; /* Azul para letras descobertas */
        if (game->adivinhada[i] == '_') {
            cor = 0x95a5a6; /* Cinza para letras não descobertas */
        } else if (game->adivinhada[i] == ' ') {
            cor = 0xffffff; /* Branco para espaços */
        }
        
        if (draw_string_scaled(letra_x, palavra_y, letra_str, cor, 2) != 0) return 1;
    }
    
    /* Desenhar letras já usadas */
    int letras_y = area_y + 280;
    if (draw_string_scaled(area_x + 50, letras_y, "Letras usadas:", 0xffffff, 1) != 0) return 1;
    
    int letra_usado_x = area_x + 50;
    int letra_usado_y = letras_y + 25;
    int coluna = 0;
    
    for (int i = 0; i < 26; i++) {
        if (game->letras_usadas[i]) {
            char letra_char = 'A' + i;
            char letra_str[2] = {letra_char, '\0'};
            
            /* Verificar se a letra está na palavra para determinar a cor */
            uint32_t cor = 0xe74c3c; /* Vermelho para letras erradas */
            if (forca_verificar_letra(game, letra_char)) {
                cor = 0x27ae60; /* Verde para letras certas */
            }
            
            if (draw_string_scaled(letra_usado_x + (coluna * 25), letra_usado_y, letra_str, cor, 1) != 0) return 1;
            coluna++;
            
            /* Nova linha a cada 13 letras */
            if (coluna >= 13) {
                coluna = 0;
                letra_usado_y += 20;
            }
        }
    }
    
    /* Mensagens de fim de jogo */
    if (game->jogo_ganho) {
        if (draw_string_scaled(area_x + 200, area_y + 400, "PARABENS! GANHASTE!", 0x27ae60, 2) != 0) return 1;
        if (draw_string_scaled(area_x + 220, area_y + 430, "Pressiona ESC para sair", 0xffffff, 1) != 0) return 1;
    } else if (game->jogo_perdido) {
        if (draw_string_scaled(area_x + 220, area_y + 400, "PERDESTE!", 0xe74c3c, 2) != 0) return 1;
        if (draw_string_scaled(area_x + 150, area_y + 430, "A palavra era: laboratorio de computadores", 0xf39c12, 1) != 0) return 1;
        if (draw_string_scaled(area_x + 220, area_y + 450, "Pressiona ESC para sair", 0xffffff, 1) != 0) return 1;
    } else {
        if (draw_string_scaled(area_x + 200, area_y + 400, "Digite uma letra (A-Z)", 0xffffff, 1) != 0) return 1;
        if (draw_string_scaled(area_x + 220, area_y + 420, "ESC para sair", 0x95a5a6, 1) != 0) return 1;
    }
    
    return 0;
}

int forca_handle_input(forca_game_t *game, uint8_t scancode) {
    if (game == NULL) return 1;
    
    /* Se o jogo terminou, aceitar apenas ESC */
    if (game->jogo_ganho || game->jogo_perdido) {
        return 0; /* Deixar o ESC ser processado pelo caller */
    }
    
    /* Verificar se é uma letra A-Z */
    for (int i = 0; i < 26; i++) {
        if (scancode == letter_scancodes[i]) {
            char letra = 'A' + i;
            
            /* Verificar se letra já foi usada */
            if (game->letras_usadas[i]) {
                return 0; /* Letra já usada, ignorar */
            }
            
            /* Marcar letra como usada */
            game->letras_usadas[i] = true;
            
            /* Verificar se letra está na palavra */
            if (forca_verificar_letra(game, letra)) {
                /* Letra correta - revelar todas as ocorrências */
                forca_revelar_letra(game, letra);
            } else {
                /* Letra incorreta - perder uma vida */
                game->vidas_restantes--;
            }
            
            return 0;
        }
    }
    
    return 0; /* Tecla não reconhecida */
}

void forca_cleanup(forca_game_t *game) {
    if (game == NULL) return;
    
    /* Limpar tela */
    clear_screen(0x1a1a2e);
    
    /* Reset do estado do jogo */
    game->jogo_ganho = false;
    game->jogo_perdido = false;
    game->primeiro_desenho = true;
}

bool forca_verificar_letra(forca_game_t *game, char letra) {
    if (game == NULL) return false;
    
    /* Converter para minúscula para comparação */
    char letra_lower = tolower(letra);
    
    for (int i = 0; i < PALAVRA_TAMANHO; i++) {
        if (tolower(game->palavra[i]) == letra_lower) {
            return true;
        }
    }
    
    return false;
}

void forca_revelar_letra(forca_game_t *game, char letra) {
    if (game == NULL) return;
    
    /* Converter para minúscula para comparação */
    char letra_lower = tolower(letra);
    
    for (int i = 0; i < PALAVRA_TAMANHO; i++) {
        if (tolower(game->palavra[i]) == letra_lower) {
            game->adivinhada[i] = game->palavra[i]; /* Manter a capitalização original */
        }
    }
}

bool forca_jogo_ganho(forca_game_t *game) {
    if (game == NULL) return false;
    
    for (int i = 0; i < PALAVRA_TAMANHO; i++) {
        if (game->palavra[i] != ' ' && game->adivinhada[i] == '_') {
            return false; /* Ainda há letras por descobrir */
        }
    }
    
    return true; /* Todas as letras foram descobertas */
}
