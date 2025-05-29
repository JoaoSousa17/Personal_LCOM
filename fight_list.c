#include "fight_list.h"
#include "videocard.h"
#include "font.h"
#include "gameLogic.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Definições de scancodes */
#define ENTER_MAKE 0x1C
#define BACKSPACE_MAKE 0x0E
#define SPACE_MAKE 0x39

/* Variável estática para seed do random */
static uint32_t fight_list_seed = 1;

/* Função simples de random */
static uint32_t fight_list_rand() {
    fight_list_seed = fight_list_seed * 1103515245 + 12345;
    return (fight_list_seed / 65536) % 32768;
}

/* Inicializa seed do random */
static void fight_list_srand(uint32_t seed) {
    fight_list_seed = seed;
}

char scancode_to_char_fight_list(uint8_t scancode) {
    /* Primeira linha: QWERTYUIOP */
    if (scancode >= 0x10 && scancode <= 0x19) {
        const char primeira_linha[] = "QWERTYUIOP";
        return primeira_linha[scancode - 0x10];
    }
    /* Segunda linha: ASDFGHJKL */
    if (scancode >= 0x1E && scancode <= 0x26) {
        const char segunda_linha[] = "ASDFGHJKL";
        return segunda_linha[scancode - 0x1E];
    }
    /* Terceira linha: ZXCVBNM */
    if (scancode >= 0x2C && scancode <= 0x32) {
        const char terceira_linha[] = "ZXCVBNM";
        return terceira_linha[scancode - 0x2C];
    }
    return 0; /* Scancode inválido */
}

bool palavra_contem_letra(const char *palavra, char letra) {
    if (palavra == NULL) return false;
    
    char letra_lower = tolower((unsigned char)letra);
    char letra_upper = toupper((unsigned char)letra);
    
    for (int i = 0; palavra[i] != '\0'; i++) {
        if (palavra[i] == letra_lower || palavra[i] == letra_upper) {
            return true;
        }
    }
    return false;
}

Categoria* selecionar_categoria_com_letra(char letra) {
    /* Array para guardar categorias válidas */
    int categorias_validas[TOTAL_CATEGORIAS];
    int count_validas = 0;
    
    /* Procurar categorias que tenham pelo menos uma palavra com a letra */
    for (int i = 0; i < TOTAL_CATEGORIAS; i++) {
        for (int j = 0; j < categorias[i].totalPontuacoes; j++) {
            if (palavra_contem_letra(categorias[i].pontuacoes[j].palavra, letra)) {
                categorias_validas[count_validas] = i;
                count_validas++;
                break; /* Encontrou pelo menos uma, pode passar à próxima categoria */
            }
        }
    }
    
    if (count_validas == 0) {
        /* Fallback: se não encontrar nenhuma, retorna a primeira categoria */
        return &categorias[0];
    }
    
    /* Escolher categoria aleatória das válidas */
    int indice_escolhido = categorias_validas[fight_list_rand() % count_validas];
    return &categorias[indice_escolhido];
}

int fight_list_init(fight_list_t *game, char letra) {
    if (game == NULL) return 1;
    
    /* Inicializar seed com base no endereço do jogo */
    fight_list_srand((uint32_t)game + letra);
    
    /* Limpar estrutura */
    memset(game, 0, sizeof(fight_list_t));
    
    /* Configurar parâmetros iniciais */
    game->letra_obrigatoria = toupper((unsigned char)letra);
    game->tempo_restante = TIMER_INICIAL;
    game->pontuacao_total = 0;
    game->count_respondidas = 0;
    game->input_length = 0;
    game->jogo_terminado = false;
    game->todas_encontradas = false;
    game->timer_counter = 0;
    
    /* Selecionar categoria que tenha palavras com a letra */
    game->categoria_atual = selecionar_categoria_com_letra(letra);
    if (game->categoria_atual == NULL) {
        return 1;
    }
    
    printf("Fight List initialized with letter '%c', category: %s\n", 
           game->letra_obrigatoria, game->categoria_atual->nome);
    
    return 0;
}

int fight_list_update(fight_list_t *game) {
    if (game == NULL || game->jogo_terminado) return 1;
    
    /* Incrementar contador do timer */
    game->timer_counter++;
    
    /* Timer roda a 60Hz, então 60 interrupts = 1 segundo */
    if (game->timer_counter >= 60) {
        game->timer_counter = 0;
        
        if (game->tempo_restante > 0) {
            game->tempo_restante--;
            
            if (game->tempo_restante == 0) {
                /* Tempo esgotado */
                game->jogo_terminado = true;
                return 1;
            }
        }
    }
    
    /* Verificar se encontrou todas as palavras possíveis */
    int palavras_possiveis = 0;
    for (int i = 0; i < game->categoria_atual->totalPontuacoes; i++) {
        if (palavra_contem_letra(game->categoria_atual->pontuacoes[i].palavra, 
                                game->letra_obrigatoria)) {
            palavras_possiveis++;
        }
    }
    
    if (game->count_respondidas >= palavras_possiveis) {
        game->todas_encontradas = true;
        game->jogo_terminado = true;
        return 1;
    }
    
    return 0; /* Jogo continua */
}

int fight_list_submit_word(fight_list_t *game) {
    if (game == NULL || game->input_length == 0) return 0;
    
    /* Verificar se a palavra contém a letra obrigatória */
    if (!palavra_contem_letra(game->input_atual, game->letra_obrigatoria)) {
        /* Limpar input */
        memset(game->input_atual, 0, sizeof(game->input_atual));
        game->input_length = 0;
        return 0; /* Palavra não contém a letra obrigatória */
    }
    
    /* Usar função de verificação do gameLogic */
    int pontos = verificarEntrada(game->categoria_atual, 
                                 game->respondidas, 
                                 &game->count_respondidas, 
                                 game->input_atual);
    
    /* Limpar input */
    memset(game->input_atual, 0, sizeof(game->input_atual));
    game->input_length = 0;
    
    if (pontos > 0) {
        game->pontuacao_total += pontos;
        return pontos;
    } else if (pontos == -1) {
        return -1; /* Palavra já respondida */
    }
    
    return 0; /* Palavra inválida */
}

int fight_list_handle_input(fight_list_t *game, uint8_t scancode) {
    if (game == NULL || game->jogo_terminado) return -1;
    
    /* Enter - submeter palavra */
    if (scancode == ENTER_MAKE) {
        return fight_list_submit_word(game);
    }
    
    /* Backspace - apagar último caractere */
    if (scancode == BACKSPACE_MAKE) {
        if (game->input_length > 0) {
            game->input_length--;
            game->input_atual[game->input_length] = '\0';
        }
        return 0;
    }
    
    /* Espaço */
    if (scancode == SPACE_MAKE) {
        if (game->input_length < MAX_INPUT_LENGTH - 1) {
            game->input_atual[game->input_length] = ' ';
            game->input_length++;
            game->input_atual[game->input_length] = '\0';
        }
        return 0;
    }
    
    /* Teclas de letras */
    char c = scancode_to_char_fight_list(scancode);
    if (c != 0 && game->input_length < MAX_INPUT_LENGTH - 1) {
        /* Converter para minúscula */
        c = tolower((unsigned char)c);
        game->input_atual[game->input_length] = c;
        game->input_length++;
        game->input_atual[game->input_length] = '\0';
        return 0;
    }
    
    return -1; /* Input inválido */
}

int fight_list_draw(fight_list_t *game) {
    if (game == NULL) return 1;
    
    /* Cores */
    uint32_t bg_color = 0x1a1a2e;      /* Fundo azul escuro */
    uint32_t orange = 0xff6b35;        /* Laranja para título */
    uint32_t white = 0xffffff;         /* Branco para texto */
    uint32_t yellow = 0xffd700;        /* Amarelo para destaque */
    uint32_t green = 0x00ff88;         /* Verde para pontuação */
    uint32_t red = 0xff4444;           /* Vermelho para timer baixo */
    uint32_t light_blue = 0x16537e;    /* Azul claro para acentos */
    uint32_t input_bg = 0x2a2a4e;      /* Fundo do campo de input */
    
    /* Limpar ecrã */
    if (clear_screen(bg_color) != 0) return 1;
    
    /* Título Fight List */
    const char *titulo = "FIGHT LIST";
    uint16_t titulo_x = (get_h_res() - strlen(titulo) * 8 * 2) / 2;
    if (draw_string_scaled(titulo_x, 20, titulo, orange, 2) != 0) return 1;
    
    /* Categoria */
    char categoria_text[100];
    snprintf(categoria_text, sizeof(categoria_text), "Categoria: %s", game->categoria_atual->nome);
    uint16_t categoria_x = (get_h_res() - strlen(categoria_text) * 8 * 1) / 2;
    if (draw_string_scaled(categoria_x, 55, categoria_text, white, 1) != 0) return 1;
    
    /* Letra obrigatória */
    char letra_text[50];
    snprintf(letra_text, sizeof(letra_text), "Letra obrigatoria: %c", game->letra_obrigatoria);
    uint16_t letra_x = (get_h_res() - strlen(letra_text) * 8 * 2) / 2;
    if (draw_string_scaled(letra_x, 80, letra_text, yellow, 2) != 0) return 1;
    
    /* Timer */
    char timer_text[20];
    snprintf(timer_text, sizeof(timer_text), "Tempo: %02d", game->tempo_restante);
    uint32_t timer_color = (game->tempo_restante <= 10) ? red : white;
    if (draw_string_scaled(50, 120, timer_text, timer_color, 2) != 0) return 1;
    
    /* Pontuação */
    char pontuacao_text[30];
    snprintf(pontuacao_text, sizeof(pontuacao_text), "Pontos: %d", game->pontuacao_total);
    uint16_t pont_x = get_h_res() - strlen(pontuacao_text) * 8 * 2 - 50;
    if (draw_string_scaled(pont_x, 120, pontuacao_text, green, 2) != 0) return 1;
    
    /* Campo de input */
    uint16_t input_y = 160;
    uint16_t input_width = 400;
    uint16_t input_height = 40;
    uint16_t input_x = (get_h_res() - input_width) / 2;
    
    /* Fundo do campo de input */
    if (draw_filled_rectangle(input_x, input_y, input_width, input_height, input_bg) != 0) return 1;
    
    /* Borda do campo de input */
    if (draw_rectangle_border(input_x, input_y, input_width, input_height, yellow, 2) != 0) return 1;
    
    /* Texto do input */
    if (game->input_length > 0) {
        if (draw_string_scaled(input_x + 10, input_y + 10, game->input_atual, white, 2) != 0) return 1;
    }
    
    /* Cursor piscante */
    static uint32_t cursor_counter = 0;
    cursor_counter++;
    if ((cursor_counter / 30) % 2 == 0) {
        uint16_t cursor_x = input_x + 10 + game->input_length * 8 * 2;
        if (draw_string_scaled(cursor_x, input_y + 10, "_", yellow, 2) != 0) return 1;
    }
    
    /* Lista de palavras respondidas */
    uint16_t lista_y = 220;
    if (draw_string_scaled(50, lista_y, "Palavras encontradas:", white, 1) != 0) return 1;
    
    /* Mostrar palavras respondidas em colunas */
    int coluna = 0;
    int linha = 0;
    for (int i = 0; i < game->count_respondidas; i++) {
        uint16_t palavra_x = 50 + coluna * 200;
        uint16_t palavra_y = lista_y + 25 + linha * 20;
        
        /* Verificar se cabe na tela */
        if (palavra_y > get_v_res() - 50) break;
        
        char palavra_with_points[100];
        /* Encontrar pontuação da palavra */
        int pontos = 1; /* Default */
        for (int j = 0; j < game->categoria_atual->totalPontuacoes; j++) {
            if (strcmp(game->respondidas[i], game->categoria_atual->pontuacoes[j].palavra) == 0) {
                pontos = game->categoria_atual->pontuacoes[j].pontuacao;
                break;
            }
        }
        
        snprintf(palavra_with_points, sizeof(palavra_with_points), "%s (%dp)", 
                game->respondidas[i], pontos);
        
        if (draw_string_scaled(palavra_x, palavra_y, palavra_with_points, green, 1) != 0) return 1;
        
        linha++;
        if (linha >= 8) { /* Máximo 8 linhas por coluna */
            linha = 0;
            coluna++;
            if (coluna >= 3) break; /* Máximo 3 colunas */
        }
    }
    
    /* Instruções */
    const char *instrucao = "Digite palavras da categoria que contenham a letra";
    uint16_t instr_x = (get_h_res() - strlen(instrucao) * 8 * 1) / 2;
    if (draw_string_scaled(instr_x, get_v_res() - 60, instrucao, white, 1) != 0) return 1;
    
    const char *instrucao2 = "Press ENTER para submeter | ESC para sair";
    uint16_t instr2_x = (get_h_res() - strlen(instrucao2) * 8 * 1) / 2;
    if (draw_string_scaled(instr2_x, get_v_res() - 40, instrucao2, white, 1) != 0) return 1;
    
    /* Cantos decorativos */
    uint16_t corner_size = 30;
    if (draw_rectangle_border(20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    if (draw_rectangle_border(get_h_res() - corner_size - 20, 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    if (draw_rectangle_border(20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    if (draw_rectangle_border(get_h_res() - corner_size - 20, get_v_res() - corner_size - 20, corner_size, corner_size, light_blue, 2) != 0) return 1;
    
    return 0;
}

void fight_list_cleanup(fight_list_t *game) {
    if (game == NULL) return;
    
    /* Limpar estrutura */
    memset(game, 0, sizeof(fight_list_t));
}
