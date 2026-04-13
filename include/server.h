#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stddef.h>
#include <time.h>

#include "protocol.h"
#include "user.h"

#define SERVER_RECV_BUFFER (MAX_SERIALIZED * 2)
#define DEFAULT_INACTIVITY_TIMEOUT 180

typedef struct {
	UserNode *head;
	pthread_mutex_t mutex;
	int inactivity_timeout_secs;
	int enforce_unique_ip;
} UserRegistry;

typedef struct {
	int client_socket;
	char client_ip[INET_ADDRSTRLEN];
	UserRegistry *registry;
} ClientThreadArgs;

int run_server(int port, int inactivity_timeout_secs);
void *client_thread_func(void *arg);

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

int handle_client_message(UserRegistry *registry,
						  int client_socket,
						  const char *client_ip,
						  Message *msg,
						  int *should_close);

int send_error_response(UserRegistry *registry, int client_socket, const char *error_text);

#endif