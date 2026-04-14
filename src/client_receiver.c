#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <time.h>
#include "client_receiver.h"
#include "client.h"

#define RECV_BUFFER_SIZE (MAX_SERIALIZED * 2)

static void format_current_timestamp(char *out, size_t out_size) {
    // hora local para etiquetar lo que llega
    time_t now = time(NULL);
    struct tm tm_now;

    if (localtime_r(&now, &tm_now) == NULL) {
        snprintf(out, out_size, "00:00:00");
        return;
    }

    strftime(out, out_size, "%H:%M:%S", &tm_now);
}

void *receiver_thread_func(void *arg) {
    ClientContext *ctx = (ClientContext *)arg;
    char buffer[RECV_BUFFER_SIZE];
    int bytes_in_buffer = 0;

    while (ctx->running) {
        // Reservar un byte al final del buffer activo para evitar overflow
        int bytes_space = RECV_BUFFER_SIZE - bytes_in_buffer - 1;
        if (bytes_space <= 0) {
            // Buffer lleno y no se pudo procesar, vaciamos para evitar bloqueo final
            bytes_in_buffer = 0;
            bytes_space = RECV_BUFFER_SIZE - 1;
        }

        int bytes_read = recv(ctx->socket_fd, buffer + bytes_in_buffer, bytes_space, 0);
        
        if (bytes_read <= 0) {
            if (ctx->running) {
                pthread_mutex_lock(&ctx->console_mutex);
                printf("\n[Sistema] Desconectado del servidor.\n");
                pthread_mutex_unlock(&ctx->console_mutex);
                ctx->running = 0;
            }
            break;
        }

        bytes_in_buffer += bytes_read;
        buffer[bytes_in_buffer] = '\0'; // Terminación nula de seguridad

        // Procesar todos los mensajes completos en el buffer
        while (bytes_in_buffer > 0) {
            Message msg;
            // intenta parsear el primer mensaje completo del stream tcp
            int res = deserializar_mensaje(buffer, bytes_in_buffer, &msg);
            
            if (res == 0) {
                // Mensaje completo y valido
                pthread_mutex_lock(&ctx->console_mutex);
                
                if (strcmp(msg.operacion, "DM") == 0) {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("\n[%s] [DM de %s]: %s\n", ts, msg.origen, msg.cuerpo);
                } else if (strcmp(msg.operacion, "BROADCAST") == 0) {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("\n[%s] [General - %s]: %s\n", ts, msg.origen, msg.cuerpo);
                } else if (strcmp(msg.operacion, "LIST_RES") == 0 || strcmp(msg.operacion, "INFO_RES") == 0) {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("\n[%s] [Servidor Info]:\n%s\n", ts, msg.cuerpo);
                } else if (strcmp(msg.operacion, "ERROR") == 0) {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("\n[%s] [ERROR]: %s\n", ts, msg.cuerpo);
                } else {
                    char ts[16];
                    format_current_timestamp(ts, sizeof(ts));
                    printf("\n[%s] [%s de %s]: %s\n", ts, msg.operacion, msg.origen, msg.cuerpo);
                }
                
                // Reimprimir el prompt para mantener el UI limpio
                printf("> ");
                fflush(stdout);
                
                pthread_mutex_unlock(&ctx->console_mutex);

                // Limpiar el mensaje del buffer y hacer shift de lo que sobra (TCP Stream)
                char *salto = strchr(buffer, '\n');
                if (salto != NULL) {
                    size_t header_len = (salto - buffer) + 1;
                    size_t total_msg_len = header_len + msg.longitud;
                    
                    if (total_msg_len <= (size_t)bytes_in_buffer) {
                        // deja al inicio lo pendiente para el siguiente ciclo
                        memmove(buffer, buffer + total_msg_len, bytes_in_buffer - total_msg_len);
                        bytes_in_buffer -= total_msg_len;
                    } else {
                        bytes_in_buffer = 0; // Sanity check
                    }
                } else {
                    bytes_in_buffer = 0; // Sanity check
                }
            } else if (res == -14 || res == -2) {
                // Faltan datos (esperamos al proximo recv)
                // -2: No hay salto de línea
                // -14: Tenemos salto de linea pero falta leer partes del cuerpo según la longitud
                break;
            } else {
                // Mensaje invalido o corrupto, en este sistema simple vaciaremos el buffer
                pthread_mutex_lock(&ctx->console_mutex);
                printf("\n[Sistema] Error de protocolo (cod %d). Descartando buffer...\n", res);
                printf("> ");
                fflush(stdout);
                pthread_mutex_unlock(&ctx->console_mutex);
                
                bytes_in_buffer = 0;
                break;
            }
        }
    }
    
    return NULL;
}