#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "protocol.h"

// estructura compartida entre hilos del cliente
// ui = hilo principal, receiver = hilo que captura mensajes
typedef struct {
    int socket_fd;                     // conexion al servidor
    char username[MAX_NAME];           // nombre de usuario local
    pthread_mutex_t console_mutex;     // evita que ui y receiver impriman simultaneamente
    int running;                       // bandera de cierre limpio (0 = terminar)
} ClientContext;

#endif // CLIENT_H