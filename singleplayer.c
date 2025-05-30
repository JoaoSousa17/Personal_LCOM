#include "singleplayer.h"
#include "videocard.h"
#include "font.h"
#include "leaderboard.h"
#include "gameLogic.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

/* Scancode definitions */
#define ENTER_MAKE 0x1C
#define BACKSPACE_MAKE 0x0E
#define SPACE_MAKE 0x39

/* Forward declarations for draw functions */
int singleplayer_draw_category_intro(singleplayer_game_t *game);
int singleplayer_draw_game_interface(singleplayer_game_t *game);
int singleplayer_draw_results(singleplayer_game_t *game);

/* Simple random number generator state */
static uint32_t sp_rand_seed = 1;

/* Simple random functions */
static uint32_t sp_simple_rand() {
    sp_rand_seed = sp_rand_seed * 1103515245 + 12345;
    return (sp_rand_seed / 65536) % 32768;
}

static void sp_simple_srand(uint32_t seed) {
    sp_rand_seed = seed;
}

char singleplayer_scancode_to_char(uint8_t scancode) {
    /* First row: QWERTYUIOP */
    if (scancode >= 0x10 && scancode <= 0x19) {
        const char first_row[] = "qwertyuiop";
        return first_row[scancode - 0x10];
    }
    /* Second row: ASDFGHJKL */
    if (scancode >= 0x1E && scancode <= 0x26) {
        const char second_row[] = "asdfghjkl";
        return second_row[scancode - 0x1E];
    }
    /* Third row: ZXCVBNM */
    if (scancode >= 0x2C && scancode <= 0x32) {
        const char third_row[] = "zxcvbnm";
        return third_row[scancode - 0x2C];
    }
    return 0; /* Invalid scancode */
}

Categoria* singleplayer_get_random_category() {
    /* Initialize random seed with a more dynamic value to ensure different categories each time */
    static uint32_t last_time = 0;
    uint32_t current_time = get_h_res() + get_v_res() + time(NULL);
    
    /* Only seed if time has changed or first call */
    if (current_time != last_time) {
        last_time = current_time;
        sp_simple_srand(current_time ^ ((uint32_t)&categorias * 13) ^ 0xABCDEF);
        printf("Random seed initialized with %u\n", (unsigned)current_time);
    }
    
    /* Select random category */
    int category_index = sp_simple_rand() % TOTAL_CATEGORIAS;
    printf("Selected category index: %d\n", category_index);
    return &categorias[category_index];
}

int singleplayer_init(singleplayer_game_t *game, const char *player_initials, char caught_letter) {
    if (game == NULL) return 1;
    
    /* Clear game structure */
    memset(game, 0, sizeof(singleplayer_game_t));
    
    /* Set initial state */
    game->state = SP_STATE_STARTING;
    game->game_active = true;
    
    /* Copy player info */
    strncpy(game->player_initials, player_initials, 3);
    game->player_initials[3] = '\0';
    game->caught_letter = caught_letter;
    
    /* Select random category */
    game->current_category = singleplayer_get_random_category();
    if (game->current_category == NULL) return 1;
    
    /* Find category index for debugging */
    for (int i = 0; i < TOTAL_CATEGORIAS; i++) {
        if (&categorias[i] == game->current_category) {
            game->category_index = i;
            break;
        }
    }
    
    /* Initialize game progress */
    game->answered_count = 0;
    game->total_score = 0;
    game->input_length = 0;
    
    /* Initialize timer - 35 seconds */
    game->timer_ticks = 0;
    game->remaining_seconds = GAME_TIME_SECONDS;
    
    /* Initialize UI state */
    game->show_cursor = true;
    game->cursor_counter = 0;
    
    /* Initialize results */
    game->all_words_found = false;
    game->time_expired = false;
    
    printf("SinglePlayer initialized: Category='%s', Player='%s', Letter='%c'\n", 
           game->current_category->nome, game->player_initials, game->caught_letter);
    
    return 0;
}

int singleplayer_update(singleplayer_game_t *game) {
    if (game == NULL || !game->game_active) return 1;
    
    /* Update cursor blinking */
    game->cursor_counter++;
    if (game->cursor_counter >= 30) { /* Blink every half second at 60Hz */
        game->show_cursor = !game->show_cursor;
        game->cursor_counter = 0;
    }
    
    /* Update timer only during gameplay */
    if (game->state == SP_STATE_PLAYING) {
        game->timer_ticks++;
        
        /* Update remaining seconds every 60 ticks (1 second at 60Hz) */
        if (game->timer_ticks >= TIMER_FREQUENCY) {
            game->timer_ticks = 0;
            
            if (game->remaining_seconds > 0) {
                game->remaining_seconds--;
            } else {
                /* Time expired */
                game->time_expired = true;
                game->state = SP_STATE_FINISHED;
                game->game_active = false;
                printf("Time expired! Final score: %d\n", game->total_score);
                return 1; /* Game finished */
            }
        }
        
        /* Check if all words found */
        if (singleplayer_all_words_found(game)) {
            game->all_words_found = true;
            game->state = SP_STATE_FINISHED;
            game->game_active = false;
            printf("All words found! Final score: %d\n", game->total_score);
            return 1; /* Game finished */
        }
    }
    
    return 0; /* Game continues */
}

int singleplayer_draw(singleplayer_game_t *game) {
    if (game == NULL) return 1;
    
    /* Define colors */
    uint32_t bg_color = 0x1a1a2e;      /* Dark blue background */
    
    /* Clear screen */
    if (clear_screen(bg_color) != 0) return 1;
    
    switch (game->state) {
        case SP_STATE_STARTING:
            /* Show category introduction */
            if (singleplayer_draw_category_intro(game) != 0) return 1;
            break;
            
        case SP_STATE_PLAYING:
            /* Show main game interface */
            if (singleplayer_draw_game_interface(game) != 0) return 1;
            break;
            
        case SP_STATE_FINISHED:
            /* Show results screen */
            if (singleplayer_draw_results(game) != 0) return 1;
            break;
            
        default:
            return 1;
    }
    
    return 0;
}

int singleplayer_draw_category_intro(singleplayer_game_t *game) {
    uint32_t orange = 0xff6b35;
    uint32_t white = 0xffffff;
    uint32_t yellow = 0xffd700;
    uint32_t green = 0x00ff88;
    uint32_t light_blue = 0x16537e;
    
    /* Draw title */
    const char *title = "CATEGORIA SELECIONADA";
    uint8_t title_scale = 3;
    uint16_t title_width = strlen(title) * 8 * title_scale;
    uint16_t title_x = (get_h_res() - title_width) / 2;
    uint16_t title_y = 60;
    
    if (draw_string_scaled(title_x, title_y, title, orange, title_scale) != 0) return 1;
    
    /* Draw decorative line */
    uint16_t line_width = title_width + 40;
    uint16_t line_x = (get_h_res() - line_width) / 2;
    uint16_t line_y = title_y + title_scale * 8 + 15;
    
    for (uint16_t i = 0; i < line_width; i++) {
        for (uint8_t thickness = 0; thickness < 3; thickness++) {
            if (draw_pixel(line_x + i, line_y + thickness, light_blue) != 0) return 1;
        }
    }
    
    /* Draw category name */
    uint8_t cat_scale = 4;
    uint16_t cat_width = strlen(game->current_category->nome) * 8 * cat_scale;
    uint16_t cat_x = (get_h_res() - cat_width) / 2;
    uint16_t cat_y = line_y + 80;
    
    if (draw_string_scaled(cat_x, cat_y, game->current_category->nome, yellow, cat_scale) != 0) return 1;
    
    /* Draw player info */
    char player_info[100];
    sprintf(player_info, "Jogador: %s | Letra: %c", game->player_initials, game->caught_letter);
    uint16_t info_x = (get_h_res() - strlen(player_info) * 8 * 2) / 2;
    if (draw_string_scaled(info_x, cat_y + 100, player_info, white, 2) != 0) return 1;
    
    /* Draw instructions */
    const char *instr1 = "Escreva palavras desta categoria";
    const char *instr2 = "Tem 35 segundos para encontrar o maximo possivel";
    const char *instr3 = "Prima ENTER para comecar";
    
    uint16_t instr_y = cat_y + 180;
    uint16_t instr1_x = (get_h_res() - strlen(instr1) * 8 * 2) / 2;
    uint16_t instr2_x = (get_h_res() - strlen(instr2) * 8 * 1) / 2;
    uint16_t instr3_x = (get_h_res() - strlen(instr3) * 8 * 2) / 2;
    
    if (draw_string_scaled(instr1_x, instr_y, instr1, white, 2) != 0) return 1;
    if (draw_string_scaled(instr2_x, instr_y + 30, instr2, white, 1) != 0) return 1;
    if (draw_string_scaled(instr3_x, instr_y + 70, instr3, green, 2) != 0) return 1;
    
    /* Draw some example words */
    const char *examples_title = "Exemplos de palavras:";
    uint16_t ex_title_x = (get_h_res() - strlen(examples_title) * 8 * 1) / 2;
    if (draw_string_scaled(ex_title_x, instr_y + 120, examples_title, yellow, 1) != 0) return 1;
    
    /* Show first 3 words as examples */
    char examples[200] = "";
    int shown = 0;
    for (int i = 0; i < game->current_category->totalPalavras && shown < 3; i++) {
        if (shown > 0) strcat(examples, ", ");
        strcat(examples, game->current_category->palavras[i]);
        shown++;
    }
    strcat(examples, "...");
    
    uint16_t ex_x = (get_h_res() - strlen(examples) * 8 * 1) / 2;
    if (draw_string_scaled(ex_x, instr_y + 140, examples, white, 1) != 0) return 1;
    
    return 0;
}

int singleplayer_draw_game_interface(singleplayer_game_t *game) {
    uint32_t white = 0xffffff;
    uint32_t yellow = 0xffd700;
    uint32_t green = 0x00ff88;
    uint32_t red = 0xff4444;
    uint32_t light_blue = 0x16537e;
    
    /* Draw category title centered on first line */
    uint16_t title_y = 50;
    uint16_t category_width = strlen(game->current_category->nome) * 8 * 2;
    uint16_t category_x = (get_h_res() - category_width) / 2;
    if (draw_string_scaled(category_x, title_y, game->current_category->nome, yellow, 2) != 0) return 1;
    
    /* Draw time and points on second line */
    char timer_info[50];
    sprintf(timer_info, "Tempo: %02d:%02d | Pontos: %d", 
            game->remaining_seconds / 60, 
            game->remaining_seconds % 60,
            game->total_score);
    
    uint16_t timer_info_width = strlen(timer_info) * 8 * 2;
    uint16_t timer_info_x = (get_h_res() - timer_info_width) / 2;
    uint16_t timer_info_y = title_y + 40; /* 40px below the title */
    
    if (draw_string_scaled(timer_info_x, timer_info_y, timer_info, white, 2) != 0) return 1;
    
    /* Draw timer bar below time info */
    uint16_t timer_bar_width = 300;
    uint16_t timer_bar_height = 15;
    uint16_t timer_bar_x = (get_h_res() - timer_bar_width) / 2;
    uint16_t timer_bar_y = timer_info_y + 30; /* 30px below time info */
    
    /* Timer bar background */
    if (draw_filled_rectangle(timer_bar_x, timer_bar_y, timer_bar_width, timer_bar_height, 0x333333) != 0) return 1;
    
    /* Timer bar fill */
    uint16_t fill_width = (timer_bar_width * game->remaining_seconds) / GAME_TIME_SECONDS;
    uint32_t timer_color = (game->remaining_seconds > 10) ? green : red;
    if (fill_width > 0) {
        if (draw_filled_rectangle(timer_bar_x, timer_bar_y, fill_width, timer_bar_height, timer_color) != 0) return 1;
    }
    
    /* Draw input field below timer bar */
    uint16_t input_field_width = 400;
    uint16_t input_field_x = (get_h_res() - input_field_width) / 2; /* Center horizontally */
    uint16_t input_field_y = timer_bar_y + 40; /* 40px below timer bar */
    uint16_t input_field_height = 50;
    
    /* Input field background */
    if (draw_filled_rectangle(input_field_x, input_field_y, input_field_width, input_field_height, 0x2a2a4e) != 0) return 1;
    
    /* Input field border */
    if (draw_rectangle_border(input_field_x, input_field_y, input_field_width, input_field_height, yellow, 2) != 0) return 1;
    
    /* Draw current input */
    if (game->input_length > 0) {
        if (draw_string_scaled(input_field_x + 10, input_field_y + 15, game->current_input, white, 2) != 0) return 1;
    }
    
    /* Draw cursor */
    if (game->show_cursor) {
        uint16_t cursor_x = input_field_x + 10 + game->input_length * 8 * 2;
        if (draw_string_scaled(cursor_x, input_field_y + 15, "_", yellow, 2) != 0) return 1;
    }
    
    /* Draw instruction */
    const char *input_instr = "Digite uma palavra e prima ENTER";
    uint16_t instr_width = strlen(input_instr) * 8; /* Width of text at scale 1 */
    uint16_t instr_x = (get_h_res() - instr_width) / 2; /* Center horizontally */
    if (draw_string_scaled(instr_x, input_field_y + input_field_height + 10, input_instr, white, 1) != 0) return 1;
    
    /* Draw answered words heading */
    uint16_t words_start_y = input_field_y + input_field_height + 40; /* 40px below input field instructions */
    if (draw_string_scaled(50, words_start_y, "Palavras encontradas:", yellow, 2) != 0) return 1;
    
    /* Show answered words in columns */
    uint16_t col_width = 200;
    uint16_t words_per_col = 12;
    uint16_t word_y = words_start_y + 30;
    
    for (int i = 0; i < game->answered_count; i++) {
        uint16_t col = i / words_per_col;
        uint16_t row = i % words_per_col;
        uint16_t word_x = 50 + col * col_width;
        uint16_t draw_y = word_y + row * 18;
        
        char word_with_score[50];
        /* Find score for this word */
        int word_score = 0;
        for (int j = 0; j < game->current_category->totalPontuacoes; j++) {
            if (strcmp(game->answered_words[i], game->current_category->pontuacoes[j].palavra) == 0) {
                word_score = game->current_category->pontuacoes[j].pontuacao;
                break;
            }
        }
        
        /* Format display text based on if word contains the caught letter */
        if (game->word_contains_letter[i]) {
            sprintf(word_with_score, "%s (+%d)", game->answered_words[i], word_score);
            /* Normal words in green */
            if (draw_string_scaled(word_x, draw_y, word_with_score, green, 1) != 0) return 1;
        } else {
            sprintf(word_with_score, "%s (+0)", game->answered_words[i]);
            /* Words without the caught letter in red */
            if (draw_string_scaled(word_x, draw_y, word_with_score, red, 1) != 0) return 1;
        }
    }
    
    /* Draw progress */
    char progress[100];
    sprintf(progress, "Progresso: %d/%d palavras", game->answered_count, game->current_category->totalPontuacoes);
    if (draw_string_scaled(50, get_v_res() - 60, progress, light_blue, 1) != 0) return 1;
    
    return 0;
}

int singleplayer_draw_results(singleplayer_game_t *game) {
    uint32_t white = 0xffffff;
    uint32_t yellow = 0xffd700;
    uint32_t green = 0x00ff88;
    uint32_t red = 0xff4444;
    uint32_t light_blue = 0x16537e;
    
    /* Draw title */
    const char *title = (game->all_words_found) ? "PARABENS! COMPLETOU TUDO!" : "TEMPO ESGOTADO!";
    uint32_t title_color = (game->all_words_found) ? green : red;
    uint8_t title_scale = 3;
    uint16_t title_width = strlen(title) * 8 * title_scale;
    uint16_t title_x = (get_h_res() - title_width) / 2;
    uint16_t title_y = 50;
    
    if (draw_string_scaled(title_x, title_y, title, title_color, title_scale) != 0) return 1;
    
    /* Draw final score */
    char score_text[100];
    sprintf(score_text, "PONTUACAO FINAL: %d", game->total_score);
    uint16_t score_width = strlen(score_text) * 8 * 4;
    uint16_t score_x = (get_h_res() - score_width) / 2;
    if (draw_string_scaled(score_x, title_y + 80, score_text, yellow, 4) != 0) return 1;
    
    /* Draw statistics */
    char stats[200];
    sprintf(stats, "Categoria: %s | Jogador: %s", game->current_category->nome, game->player_initials);
    uint16_t stats_x = (get_h_res() - strlen(stats) * 8 * 2) / 2;
    if (draw_string_scaled(stats_x, title_y + 150, stats, white, 2) != 0) return 1;
    
    char progress[100];
    sprintf(progress, "Palavras encontradas: %d de %d", game->answered_count, game->current_category->totalPontuacoes);
    uint16_t progress_x = (get_h_res() - strlen(progress) * 8 * 2) / 2;
    if (draw_string_scaled(progress_x, title_y + 180, progress, light_blue, 2) != 0) return 1;
    
    /* Draw instructions */
    const char *instr = "Prima ESC para voltar ao menu principal";
    uint16_t instr_x = (get_h_res() - strlen(instr) * 8 * 2) / 2;
    if (draw_string_scaled(instr_x, get_v_res() - 100, instr, white, 2) != 0) return 1;
    
    /* Save score to leaderboard */
    static bool score_saved = false;
    if (!score_saved) {
        singleplayer_save_score(game);
        score_saved = true;
    }
    
    return 0;
}

int singleplayer_handle_input(singleplayer_game_t *game, uint8_t scancode) {
    if (game == NULL) return 1;
    
    switch (game->state) {
        case SP_STATE_STARTING:
            if (scancode == ENTER_MAKE) {
                game->state = SP_STATE_PLAYING;
                printf("Starting gameplay phase\n");
                return 0;
            }
            break;
            
        case SP_STATE_PLAYING:
            /* Handle ENTER key */
            if (scancode == ENTER_MAKE) {
                int score = singleplayer_submit_word(game);
                if (score > 0) {
                    printf("Word accepted! Score: %d\n", score);
                } else {
                    printf("Word rejected or already answered\n");
                }
                return 0;
            }
            
            /* Handle BACKSPACE */
            if (scancode == BACKSPACE_MAKE) {
                singleplayer_remove_char(game);
                return 0;
            }
            
            /* Handle letter keys */
            char c = singleplayer_scancode_to_char(scancode);
            if (c != 0) {
                if (singleplayer_add_char(game, c) == 0) {
                    return 0; /* Character added */
                }
            }
            break;
            
        case SP_STATE_FINISHED:
            /* Game finished, input handled elsewhere (ESC to exit) */
            break;
            
        default:
            break;
    }
    
    return 0;
}

int singleplayer_add_char(singleplayer_game_t *game, char c) {
    if (game->input_length >= MAX_INPUT_LENGTH - 1) {
        return 1; /* Input is full */
    }
    
    /* Convert to lowercase */
    c = (char)tolower((unsigned char)c);
    
    /* Only allow letters */
    if (c >= 'a' && c <= 'z') {
        game->current_input[game->input_length] = c;
        game->input_length++;
        game->current_input[game->input_length] = '\0';
        return 0;
    }
    
    return 1; /* Invalid character */
}

void singleplayer_remove_char(singleplayer_game_t *game) {
    if (game->input_length > 0) {
        game->input_length--;
        game->current_input[game->input_length] = '\0';
    }
}

int singleplayer_submit_word(singleplayer_game_t *game) {
    if (game->input_length == 0) {
        return 0; /* Empty input */
    }
    
    /* Check if the word is in the dictionary of the theme */
    int score = singleplayer_check_word(game, game->current_input);
    
    if (score > 0) {
        /* Check if the word contains the caught letter */
        char caught_letter = tolower((unsigned char)game->caught_letter);
        bool contains_letter = false;
        
        /* Convert input to lowercase for comparison */
        char input_lower[MAX_INPUT_LENGTH];
        strncpy(input_lower, game->current_input, MAX_INPUT_LENGTH - 1);
        input_lower[MAX_INPUT_LENGTH - 1] = '\0';
        
        /* Convert to lowercase */
        for (int j = 0; input_lower[j]; j++) {
            input_lower[j] = tolower((unsigned char)input_lower[j]);
            
            /* Check if this character matches the caught letter */
            if (input_lower[j] == caught_letter) {
                contains_letter = true;
                break;
            }
        }
        
        /* Find the correct word in the category to add to answered list */
        /* We need to find which word in the category matches our input */
        for (int i = 0; i < game->current_category->totalPontuacoes; i++) {
            char categoria_word[MAX_TAMANHO_PALAVRA];
            strncpy(categoria_word, game->current_category->pontuacoes[i].palavra, MAX_TAMANHO_PALAVRA - 1);
            categoria_word[MAX_TAMANHO_PALAVRA - 1] = '\0';
            
            /* Convert both to lowercase for comparison */
            char cat_lower[MAX_TAMANHO_PALAVRA];
            
            strncpy(cat_lower, categoria_word, MAX_TAMANHO_PALAVRA - 1);
            cat_lower[MAX_TAMANHO_PALAVRA - 1] = '\0';
            
            /* Convert to lowercase */
            for (int j = 0; cat_lower[j]; j++) {
                cat_lower[j] = tolower((unsigned char)cat_lower[j]);
            }
            
            /* Remove accents from both */
            removerAcentos(input_lower);
            removerAcentos(cat_lower);
            
            /* Check if they match (with distance <= 1) */
            if (distanciaLevenshtein(input_lower, cat_lower) <= 1) {
                /* Add the original word from category to answered list */
                strncpy(game->answered_words[game->answered_count], categoria_word, MAX_TAMANHO_PALAVRA - 1);
                game->answered_words[game->answered_count][MAX_TAMANHO_PALAVRA - 1] = '\0';
                
                /* Store whether the word contains the caught letter */
                game->word_contains_letter[game->answered_count] = contains_letter;
                
                /* Only add points if the word contains the caught letter */
                if (contains_letter) {
                    game->total_score += score;
                    printf("Word '%s' accepted as '%s', score: %d, total: %d\n", 
                          game->current_input, categoria_word, score, game->total_score);
                } else {
                    printf("Word '%s' accepted as '%s', but +0 points (missing letter '%c')\n", 
                          game->current_input, categoria_word, game->caught_letter);
                }
                
                game->answered_count++;
                break;
            }
        }
    }
    
    /* Clear input */
    game->input_length = 0;
    game->current_input[0] = '\0';
    
    return score;
}

int singleplayer_check_word(singleplayer_game_t *game, const char *word) {
    if (game == NULL || word == NULL || game->current_category == NULL) {
        return 0;
    }
    
    /* Check if word was already answered */
    for (int i = 0; i < game->answered_count; i++) {
        if (strcmp(game->answered_words[i], word) == 0) {
            return 0; /* Already answered */
        }
    }
    
    /* Create a temporary copy of answered words for verification */
    char temp_answered[MAX_ANSWERED_WORDS][MAX_ENTRADA];
    int temp_count = game->answered_count;
    
    /* Copy current answered words to temp array */
    for (int i = 0; i < game->answered_count; i++) {
        strncpy(temp_answered[i], game->answered_words[i], MAX_ENTRADA - 1);
        temp_answered[i][MAX_ENTRADA - 1] = '\0';
    }
    
    /* Use the game logic from gameLogic.c to verify the word */
    int score = verificarEntrada(game->current_category, temp_answered, &temp_count, word);
    
    /* If the word is valid and was added to temp_answered, we know it's correct */
    /* But we don't update game->answered_words here - that's done in submit_word */
    
    return score;
}

bool singleplayer_all_words_found(singleplayer_game_t *game) {
    if (game == NULL || game->current_category == NULL) {
        return false;
    }
    
    return game->answered_count >= game->current_category->totalPontuacoes;
}

uint32_t singleplayer_get_time_remaining(singleplayer_game_t *game) {
    if (game == NULL) return 0;
    return game->remaining_seconds;
}

void singleplayer_save_score(singleplayer_game_t *game) {
    if (game == NULL || game->current_category == NULL) return;
    
    guardarPontuacao(game->total_score, game->player_initials, game->current_category->nome);
    printf("Score saved: %d points for %s in category %s\n", 
           game->total_score, game->player_initials, game->current_category->nome);
}

void singleplayer_cleanup(singleplayer_game_t *game) {
    if (game == NULL) return;
    
    /* Reset game state */
    game->game_active = false;
    game->state = SP_STATE_CLEANUP;
    
    /* Clear sensitive data */
    memset(game->current_input, 0, sizeof(game->current_input));
    memset(game->answered_words, 0, sizeof(game->answered_words));
    
    printf("SinglePlayer game cleaned up\n");
}

