#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stddef.h>
#include <time.h>

#include "protocol.h"
#include "user.h"

// buffer por cliente
#define SERVER_RECV_BUFFER (MAX_SERIALIZED * 2)
// segundos aprox
#define DEFAULT_INACTIVITY_TIMEOUT 180

// registro global de usuarios conectados con sincronizacion
typedef struct {
	UserNode *head;                   // cabeza de lista enlazada
	pthread_mutex_t mutex;            // protege acceso concurrente
	int inactivity_timeout_secs;      // timeout global para marcar inactivos
	int enforce_unique_ip;            // 1=una ip por usuario, 0=permite duplicados
} UserRegistry;

// argumentos que se pasan al hilo de cada cliente
typedef struct {
	int client_socket;                // socket de comunicacion
	char client_ip[INET_ADDRSTRLEN];  // ip del cliente conectado
	UserRegistry *registry;           // dato compartido entre todos los hilos
} ClientThreadArgs;

// inicia servidor en puerto y timeout especificados
int run_server(int port, int inactivity_timeout_secs);
// hilo que atiende a un cliente, procesa sus mensajes hasta desconexion
void *client_thread_func(void *arg);

// funciones del registro de usuarios del servidor
int registry_init(UserRegistry *registry, int inactivity_timeout_secs);
void registry_destroy(UserRegistry *registry);
int registry_add_user(UserRegistry *registry, const char *username, const char *ip, int socket_fd);
int registry_remove_user(UserRegistry *registry, const char *username);
int registry_remove_user_by_socket(UserRegistry *registry,
								   int socket_fd,
								   char *username_out,
								   size_t username_out_size);
int registry_get_user(UserRegistry *registry, const char *username, User *out_user);
int registry_update_status(UserRegistry *registry, const char *username, const char *status);
int registry_touch_activity(UserRegistry *registry, const char *username, time_t now_ts);
int registry_mark_inactive_if_timeout(UserRegistry *registry, time_t now_ts);
int registry_build_user_list(UserRegistry *registry, char *out_buffer, size_t out_size);
int registry_send_to_user(UserRegistry *registry, const char *username, const Message *msg);
int registry_broadcast(UserRegistry *registry, const Message *msg, const char *exclude_username);
int registry_username_from_socket(UserRegistry *registry,
								  int socket_fd,
								  char *username_out,
								  size_t username_out_size);

// despacha operaciones de cliente segun tipo de mensaje
int handle_client_message(UserRegistry *registry,
						  int client_socket,
						  const char *client_ip,
						  Message *msg,
						  int *should_close);

// envia respuesta de error al cliente
int send_error_response(UserRegistry *registry, int client_socket, const char *error_text);

#endif