#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include "client_ui.h"
#include "protocol.h"

static void format_current_timestamp(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm tm_now;

    if (localtime_r(&now, &tm_now) == NULL) {
        snprintf(out, out_size, "00:00:00");
        return;
    }

    strftime(out, out_size, "%H:%M:%S", &tm_now);
}

void mostrar_menu() {
    printf("\n==== MENU DE COMANDOS ====\n");
    printf("LIST                     -> Ver usuarios conectados\n");
    printf("INFO <usuario>           -> Ver info de un usuario\n");
    printf("STATUS <estado>          -> Cambiar estado (ACTIVO, OCUPADO, INACTIVO)\n");
    printf("MSG <usuario> <mensaje>  -> Enviar mensaje directo\n");
    printf("BROADCAST <mensaje>      -> Enviar mensaje a todos\n");
    printf("EXIT                     -> Salir del chat\n");
    printf("=========================\n");
}

int send_message(ClientContext *ctx, const char *dest, const char *op, const char *body) {
    Message msg;
    char buffer[MAX_SERIALIZED];
    size_t bytes;

    if (construir_mensaje(&msg, dest, ctx->username, op, body) < 0) {
        printf("[Error Internal] No se pudo construir el mensaje.\n");
        return -1;
    }
    
    if (serializar_mensaje(&msg, buffer, sizeof(buffer), &bytes) < 0) {
        printf("[Error Internal] No se pudo serializar el mensaje.\n");
        return -1;
    }

    if (send(ctx->socket_fd, buffer, bytes, 0) < 0) {
        perror("[Error de Red] Falló el envio");
        return -1;
    }
    return 0;
}

void start_ui(ClientContext *ctx) {
    char input[MAX_BODY + 50];
    mostrar_menu();

    while (ctx->running) {
        // Obtenemos input del usuario, fuera del lock (scanf/fgets bloquea)
        if (!fgets(input, sizeof(input), stdin)) continue;
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) continue;

        // Mutex temporal simple solo para el output extra de consola
        pthread_mutex_lock(&ctx->console_mutex);
        
        char cmd[20] = {0};
        
        // Parsear primer comando
        char *token = strtok(input, " ");
        if (!token) {
            pthread_mutex_unlock(&ctx->console_mutex);
            continue;
        }
        
        strncpy(cmd, token, sizeof(cmd) - 1);

        if (strcmp(cmd, "EXIT") == 0) {
            send_message(ctx, "SERVER", "EXIT", "");
            ctx->running = 0;
            pthread_mutex_unlock(&ctx->console_mutex);
            break;
        } 
        else if (strcmp(cmd, "LIST") == 0) {
            send_message(ctx, "SERVER", "LIST_REQ", "");
        } 
        else if (strcmp(cmd, "INFO") == 0) {
            token = strtok(NULL, "");
            if (token) {
                // Enviamos el nombre consultado en el cuerpo
                send_message(ctx, "SERVER", "INFO_REQ", token);
            } else {
                printf("Uso: INFO <usuario>\n");
            }
        } 
        else if (strcmp(cmd, "STATUS") == 0) {
            token = strtok(NULL, "");
            if (token) {
                if (es_estado_valido(token)) {
                    send_message(ctx, "SERVER", "STATUS", token);
                    printf("Estado actualizado temporalmente.\n");
                } else {
                    printf("Estado invalido. Use: ACTIVO, OCUPADO, INACTIVO\n");
                }
            } else {
                printf("Uso: STATUS <estado>\n");
            }
        } 
        else if (strcmp(cmd, "MSG") == 0) {
            char *userToken = strtok(NULL, " ");
            char *msgToken = strtok(NULL, "");
            if (userToken && msgToken) {
                if (send_message(ctx, userToken, "DM", msgToken) == 0) {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("[%s] [Enviado -> %s]: %s\n", ts, userToken, msgToken);
                }
            } else {
                printf("Uso: MSG <usuario> <mensaje>\n");
            }
        } 
        else if (strcmp(cmd, "BROADCAST") == 0) {
            token = strtok(NULL, "");
            if (token) {
                if (send_message(ctx, "ALL", "BROADCAST", token) == 0) {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("[%s] [Broadcast enviado]: %s\n", ts, token);
                }
            } else {
                printf("Uso: BROADCAST <mensaje>\n");
            }
        } 
        else {
            printf("Comando no reconocido. Escriba LIST, INFO, STATUS, MSG, BROADCAST o EXIT.\n");
        }
        
        pthread_mutex_unlock(&ctx->console_mutex);
    }
}