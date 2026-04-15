#ifndef USER_H
#define USER_H

#include <arpa/inet.h>
#include <time.h>

#include "common.h"

// datos de un usuario conectado en el servidor
typedef struct {
    char username[MAX_NAME];           // nombre unico
    char ip[INET_ADDRSTRLEN];          // ip de origen
    char status[MAX_STATUS];           // ACTIVO, OCUPADO, INACTIVO
    int socket_fd;                     // descriptor para enviar mensajes
    time_t last_activity;              // timestamp del ultimo evento
} User;

// nodo de la lista enlazada que guarda usuarios en el servidor
typedef struct UserNode {
    User user;
    struct UserNode *next;             // enlace al siguiente
} UserNode;

#endif
