#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "leaderboard.h"
#include "utils.h"
#include "videocard.h"
#include "font.h"

#define MAX_ENTRADAS 5
#define FICHEIRO "leaderboard.txt"

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
    // Função mantida para compatibilidade com terminal
    FILE* f = fopen(FICHEIRO, "r");
    if (!f) {
        printf("Nenhum registo encontrado.\n");
        return;
    }

    printf("\033[1;36mLEADERBOARD - TOP 5\033[0m\n");

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

    fclose(f);
}

int draw_leaderboard_graphics() {
    /* Define colors */
    uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
    uint32_t orange = 0xff6b35;        /* Orange for title */
    uint32_t white = 0xffffff;         /* White for text */
    uint32_t gold = 0xffd700;          /* Gold for first place */
    uint32_t silver = 0xc0c0c0;        /* Silver for second place */
    uint32_t bronze = 0xcd7f32;        /* Bronze for third place */
    uint32_t light_blue = 0x16537e;    /* Light blue for accent */
    
    /* Clear screen with dark background */
    if (clear_screen(bg_color) != 0) return 1;
    
    /* Draw title */
    const char *title = "LEADERBOARD - TOP 5";
    uint8_t title_scale = 3;
    uint16_t title_width = strlen(title) * 8 * title_scale;
    uint16_t title_x = (get_h_res() - title_width) / 2;
    uint16_t title_y = 50;
    
    if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
    
    /* Draw decorative line under title */
    uint16_t line_width = title_width + 40;
    uint16_t line_x = (get_h_res() - line_width) / 2;
    uint16_t line_y = title_y + title_scale * 8 + 15;
    
    for (uint16_t i = 0; i < line_width; i++) {
        for (uint8_t thickness = 0; thickness < 3; thickness++) {
            if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
        }
    }
    
    /* Try to read leaderboard file */
    FILE* f = fopen(FICHEIRO, "r");
    
    if (!f) {
        /* Try alternative paths */
        f = fopen("./leaderboard.txt", "r");
        if (!f) {
            f = fopen("../leaderboard.txt", "r");
        }
        if (!f) {
            f = fopen("/home/lcom/labs/grupo_2leic10_2/proj/src/leaderboard.txt", "r");
        }
    }
    
    if (!f) {
        /* No leaderboard file found - show empty message */
        const char *empty_msg = "Nenhum registo encontrado.";
        uint16_t empty_x = (get_h_res() - strlen(empty_msg) * 8 * 2) / 2;
        if (draw_string_scaled(empty_x, 200, empty_msg, white, 2) != 0) return 1;
    } else {
        /* Read and display leaderboard entries */
        char linha[128];
        char iniciais[4], categoria[50];
        int pontos, pos = 1;
        uint16_t entry_y = line_y + 60;
        bool found_entries = false;
        
        while (fgets(linha, sizeof(linha), f) && pos <= MAX_ENTRADAS) {
            linha[strcspn(linha, "\r\n")] = '\0';
            
            /* Initialize variables */
            memset(iniciais, 0, sizeof(iniciais));
            memset(categoria, 0, sizeof(categoria));
            pontos = 0;
            
            /* Parse line with quoted category - try exact format first */
            int parsed = sscanf(linha, "%3s \"%49[^\"]\" %d", iniciais, categoria, &pontos);
            
            if (parsed == 3) {
                found_entries = true;
                
                /* Format entry string */
                char entry_text[200];
                snprintf(entry_text, sizeof(entry_text), "%d. %s - %s - %d pontos", pos, iniciais, categoria, pontos);
                
                /* Choose color based on position */
                uint32_t entry_color = white;
                if (pos == 1) entry_color = gold;
                else if (pos == 2) entry_color = silver;
                else if (pos == 3) entry_color = bronze;
                
                /* Draw entry */
                uint16_t entry_x = 100;
                if (draw_string_scaled(entry_x, entry_y, entry_text, entry_color, 2) != 0) {
                    fclose(f);
                    return 1;
                }
                
                entry_y += 40; /* Move to next line */
                pos++;
            } else {
                /* Try alternative parsing without quotes */
                parsed = sscanf(linha, "%3s %49s %d", iniciais, categoria, &pontos);
                if (parsed == 3) {
                    found_entries = true;
                    
                    /* Remove quotes from categoria if present */
                    if (categoria[0] == '"') {
                        memmove(categoria, categoria + 1, strlen(categoria));
                    }
                    if (categoria[strlen(categoria) - 1] == '"') {
                        categoria[strlen(categoria) - 1] = '\0';
                    }
                    
                    /* Format entry string */
                    char entry_text[200];
                    snprintf(entry_text, sizeof(entry_text), "%d. %s - %s - %d pontos", pos, iniciais, categoria, pontos);
                    
                    /* Choose color based on position */
                    uint32_t entry_color = white;
                    if (pos == 1) entry_color = gold;
                    else if (pos == 2) entry_color = silver;
                    else if (pos == 3) entry_color = bronze;
                    
                    /* Draw entry */
                    uint16_t entry_x = 100;
                    if (draw_string_scaled(entry_x, entry_y, entry_text, entry_color, 2) != 0) {
                        fclose(f);
                        return 1;
                    }
                    
                    entry_y += 40; /* Move to next line */
                    pos++;
                }
            }
        }
        
        fclose(f);
        
        /* If no entries were found, show message */
        if (!found_entries) {
            const char *empty_msg = "Nenhum registo encontrado.";
            uint16_t empty_x = (get_h_res() - strlen(empty_msg) * 8 * 2) / 2;
            if (draw_string_scaled(empty_x, 200, empty_msg, white, 2) != 0) return 1;
        }
    }
    
    /* Draw back instruction */
    const char *back_msg = "Press ESC to go back";
    uint16_t back_x = 100;
    uint16_t back_y = get_v_res() - 80;
    if (draw_string_scaled(back_x, back_y, back_msg, white, 1) != 0) return 1;
    
    /* Add decorative corners like in main menu */
    uint16_t corner_size = 30;
    
    /* Top-left corner */
    if (draw_rectangle_border(20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    
    /* Top-right corner */
    if (draw_rectangle_border(get_h_res() - corner_size - 20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    
    /* Bottom-left corner */
    if (draw_rectangle_border(20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    
    /* Bottom-right corner */
    if (draw_rectangle_border(get_h_res() - corner_size - 20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    
    return 0;
}

int draw_leaderboard_with_mouse(uint16_t mouse_x, uint16_t mouse_y) {
    /* Draw the leaderboard content first */
    if (draw_leaderboard_graphics() != 0) return 1;
    
    /* Draw back arrow button in top-left */
    uint16_t arrow_x = 30;
    uint16_t arrow_y = 30;
    uint16_t arrow_width = 80;
    uint16_t arrow_height = 30;
    
    /* Check if mouse is hovering over back button */
    bool back_hovered = (mouse_x >= arrow_x && mouse_x <= arrow_x + arrow_width &&
                        mouse_y >= arrow_y && mouse_y <= arrow_y + arrow_height);
    
    /* Draw back button */
    uint32_t button_color = back_hovered ? 0xffa500 : 0xffd700; /* Orange when hovered, yellow otherwise */
    uint32_t bg_color = back_hovered ? 0x2a2a4e : 0x1a1a2e;
    
    /* Draw button background */
    if (draw_filled_rectangle(arrow_x, arrow_y, arrow_width, arrow_height, bg_color) != 0) return 1;
    
    /* Draw button border */
    if (draw_rectangle_border(arrow_x, arrow_y, arrow_width, arrow_height, button_color, 2) != 0) return 1;
    
    /* Draw back arrow and text */
    const char *back_text = "<- BACK";
    uint16_t text_x = arrow_x + 8;
    uint16_t text_y = arrow_y + 8;
    uint32_t text_color = back_hovered ? 0xffffff : 0xffd700;
    
    if (draw_string_scaled(text_x, text_y, back_text, text_color, 1) != 0) return 1;
    
    /* Draw mouse cursor */
    if (draw_mouse_cursor(mouse_x, mouse_y, 0xffffff) != 0) return 1;
    
    return 0;
}

int handle_leaderboard_click(uint16_t x, uint16_t y, bool left_click) {
    if (!left_click) return -1;  /* Only handle left clicks */
    
    /* Check back button */
    uint16_t arrow_x = 30;
    uint16_t arrow_y = 30;
    uint16_t arrow_width = 80;
    uint16_t arrow_height = 30;
    
    if (x >= arrow_x && x <= arrow_x + arrow_width &&
        y >= arrow_y && y <= arrow_y + arrow_height) {
        return 1; /* Back button clicked */
    }
    
    return -1; /* No action */
}
