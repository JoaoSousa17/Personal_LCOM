#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lcom/lcf.h>
#include "utils.h"
#include "deviceDrivers/videoCard/videocard.h"
#include "deviceDrivers/timer/timer.h"
#include "deviceDrivers/keyboard/keyboard.h"
#include "/de"
#include "leaderboard.h"

// Define os estados do jogo;
typedef enum {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    EXIT
} game_state_t;

// Máscaras IRQ para variados dispositivos;
#define TIMER_IRQ_SET    BIT(0)
#define KEYBOARD_IRQ_SET BIT(1)
#define MOUSE_IRQ_SET    BIT(12)

// Ciclo principal do jogo;
int main(int argc, char *argv[]) {
    // Inicializa as definições base LCF
    lcf_set_language("PT-PT");
    lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");
    lcf_log_output("/home/lcom/labs/lab5/output.txt");
    
    if (lcf_start(argc, argv))
        return 1;

    // Inicia a placa de video e entra no modo gráfico;
    if (vg_init(0x105) != 0) { // Modo 0x105: 1024x768, 256 cores;
        printf("Failed to initialize video card\n");
        lcf_cleanup();
        return 1;
    }

    // Irq_bit para as mais variadas interrupções;
    uint8_t timer_irq_bit, keyboard_irq_bit, mouse_irq_bit;
    
    // Subscreve as interrupções do timer;
    if (timer_subscribe_int(&timer_irq_bit) != 0) {
        printf("Failed to subscribe timer interrupts\n");
        vg_exit();
        lcf_cleanup();
        return 1;
    }
    
    // Subscreve as interrupções do teclado;
    if (keyboard_subscribe_int(&keyboard_irq_bit) != 0) {
        printf("Failed to subscribe keyboard interrupts\n");
        timer_unsubscribe_int();
        vg_exit();
        lcf_cleanup();
        return 1;
    }

    // Subscreve as interrupções do rato;
    if (mouse_subscribe_int(&mouse_irq_bit) != 0) {
        printf("Failed to subscribe mouse interrupts\n");
        keyboard_unsubscribe_int();
        timer_unsubscribe_int();
        vg_exit();
        lcf_cleanup();
        return 1;
    }

    // Cria IRQ sets para verificar os interrupts;
    int timer_irq_set = BIT(timer_irq_bit);
    int keyboard_irq_set = BIT(keyboard_irq_bit);
    int mouse_irq_set = BIT(mouse_irq_bit);

    // Variàveis necessárias para controlo do ciclo e do jogo;
    game_state_t game_state = MENU;
    int ipc_status, r;
    message msg;
    
    // Variáveis de controlo do tempo no jogo;
    uint32_t timer_counter = 0;
    const uint32_t GAME_TICK_RATE = 60; // 60 Hz
    
    // Loop principal do jogo - Gestão de Interrupções
    while (game_state != EXIT) {
        // Obtém a mensagem do pedido, caso este exista;
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        
        if (is_ipc_notify(ipc_status)) { // Notificação recebida;
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE: // Notificação de interrupção de Hardware;
                    
                    // Interrupção do Timer;
                    if (msg.m_notify.interrupts & timer_irq_set) {
                        timer_int_handler();
                        timer_counter++;
                        
                        // Atualiza o ecrã a uma taxa fixa - animações
                        if (timer_counter % (60 / GAME_TICK_RATE) == 0) {
                            switch (game_state) {
                                case MENU:
                                    // Atualiza as animações do menu;

                                    break;
                                case PLAYING:
                                    // Atualiza lógica de jogo;
                                    game_update();

                                    break;
                                case PAUSED:
                                    // Atualiza animações do ecrã de pausa;

                                    break;
                                case GAME_OVER:
                                    // Atualiza ecrã de Game Over

                                    break;
                                default:
                                    break;
                            }
                        }
                        
                        // Atualiza sempre o ecrã, redesenhando-o;
                        switch (game_state) {
                            case MENU:
                                draw_menu();

                                break;
                            case PLAYING:
                                draw_game();

                                break;
                            case PAUSED:
                                draw_pause_screen();

                                break;
                            case GAME_OVER:
                                draw_game_over();

                                break;
                            default:
                                break;
                        }
                    }
                    
                    // Interrupção do Teclado;
                    if (msg.m_notify.interrupts & keyboard_irq_set) {
                        uint8_t scancode;
                        if (keyboard_int_handler(&scancode) == 0) {
                            switch (game_state) {
                                case MENU:
                                    game_state = handle_menu_keyboard(scancode);

                                    break;
                                case PLAYING:
                                    game_state = handle_game_keyboard(scancode);

                                    break;
                                case PAUSED:
                                    game_state = handle_pause_keyboard(scancode);

                                    break;
                                case GAME_OVER:
                                    game_state = handle_game_over_keyboard(scancode);

                                    break;
                                default:
                                    break;
                            }
                            
                            // Configuração específica geral para a tecla ESC - pensar como configurá-la;
                            if (scancode == 0x81) {
                                game_state = EXIT;
                            }
                        }
                    }
                    
                    // Interrupção do Rato;
                    if (msg.m_notify.interrupts & mouse_irq_set) {
                        struct packet mouse_packet;
                        if (mouse_int_handler(&mouse_packet) == 0) {
                            switch (game_state) {
                                case MENU:
                                    game_state = handle_menu_mouse(&mouse_packet);

                                    break;
                                case PLAYING:
                                    game_state = handle_game_mouse(&mouse_packet);

                                    break;
                                case PAUSED:
                                    game_state = handle_pause_mouse(&mouse_packet);

                                    break;
                                case GAME_OVER:
                                    game_state = handle_game_over_mouse(&mouse_packet);

                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    
                    break;
                    
                default:
                    break; // no other notifications expected: do nothing
            }
        } else { // received a standard message, not a notification
            // no standard messages expected: do nothing
        }
    }

    // Cleanup - deixa de subscrever a insterrupções dos mais variados dispositivos e dai do modo gráfico;
    mouse_unsubscribe_int();
    keyboard_unsubscribe_int();
    timer_unsubscribe_int();
    vg_exit();
    
    lcf_cleanup();
    return 0;
}

// Placeholder functions - these should be implemented according to your game logic
void game_update() {
    // Update game state, physics, AI, etc.
}

void draw_menu() {
    // Draw main menu
}

void draw_game() {
    // Draw game screen
}

void draw_pause_screen() {
    // Draw pause screen
}

void draw_game_over() {
    // Draw game over screen
}

game_state_t handle_menu_keyboard(uint8_t scancode) {
    // Handle keyboard input in menu
    return MENU; // placeholder
}

game_state_t handle_game_keyboard(uint8_t scancode) {
    // Handle keyboard input during game
    return PLAYING; // placeholder
}

game_state_t handle_pause_keyboard(uint8_t scancode) {
    // Handle keyboard input in pause screen
    return PAUSED; // placeholder
}

game_state_t handle_game_over_keyboard(uint8_t scancode) {
    // Handle keyboard input in game over screen
    return GAME_OVER; // placeholder
}

game_state_t handle_menu_mouse(struct packet *mouse_packet) {
    // Handle mouse input in menu
    return MENU; // placeholder
}

game_state_t handle_game_mouse(struct packet *mouse_packet) {
    // Handle mouse input during game
    return PLAYING; // placeholder
}

game_state_t handle_pause_mouse(struct packet *mouse_packet) {
    // Handle mouse input in pause screen
    return PAUSED; // placeholder
}

game_state_t handle_game_over_mouse(struct packet *mouse_packet) {
    // Handle mouse input in game over screen
    return GAME_OVER; // placeholder
}