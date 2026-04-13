#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "client_receiver.h"
#include "client.h"

#define RECV_BUFFER_SIZE (MAX_SERIALIZED * 2)

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
            int res = deserializar_mensaje(buffer, bytes_in_buffer, &msg);
            
            if (res == 0) {
                // Mensaje completo y valido
                pthread_mutex_lock(&ctx->console_mutex);
                
                if (strcmp(msg.operacion, "DM") == 0) {
                    printf("\n[DM de %s]: %s\n", msg.origen, msg.cuerpo);
                } else if (strcmp(msg.operacion, "BROADCAST") == 0) {
                    printf("\n[General - %s]: %s\n", msg.origen, msg.cuerpo);
                } else if (strcmp(msg.operacion, "LIST_RES") == 0 || strcmp(msg.operacion, "INFO_RES") == 0) {
                    printf("\n[Servidor Info]:\n%s\n", msg.cuerpo);
                } else if (strcmp(msg.operacion, "ERROR") == 0) {
                    printf("\n[ERROR]: %s\n", msg.cuerpo);
                } else {
                    printf("\n[%s de %s]: %s\n", msg.operacion, msg.origen, msg.cuerpo);
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