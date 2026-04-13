#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include "protocol.h"

// Estructura para compartir información de estado del cliente 
// entre los distintos hilos (UI y Receiver).
typedef struct {
    int socket_fd;
    char username[MAX_NAME];
    pthread_mutex_t console_mutex; // Para evitar que UI y Receiver impriman al mismo tiempo y se rompa la consola
    int running; // Flag para cerrar limpiamente
} ClientContext;

#endif // CLIENT_H