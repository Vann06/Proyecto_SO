#include "server.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int send_all_bytes(int socket_fd, const char *data, size_t size) {
	// asegura envio completo aunque send mande parcial
	size_t sent = 0;

	while (sent < size) {
		ssize_t bytes = send(socket_fd, data + sent, size - sent, 0);
		if (bytes <= 0) {
			return -1;
		}
		sent += (size_t)bytes;
	}

	return 0;
}

static int send_message_to_socket(int socket_fd, const Message *msg) {
	char serialized[MAX_SERIALIZED];
	size_t written = 0;

	if (serializar_mensaje(msg, serialized, sizeof(serialized), &written) < 0) {
		return -1;
	}

	return send_all_bytes(socket_fd, serialized, written);
}

int registry_init(UserRegistry *registry, int inactivity_timeout_secs) {
	// inicializa estado global del registro compartido
	if (registry == NULL) {
		return -1;
	}

	registry->head = NULL;
	registry->inactivity_timeout_secs = inactivity_timeout_secs;
	registry->enforce_unique_ip = 1;

	const char *chat_env = getenv("CHAT_ENV");
	// en testing se permite misma ip para pruebas locales
	if (chat_env != NULL && strcmp(chat_env, "testing") == 0) {
		registry->enforce_unique_ip = 0;
	}

	const char *enforce_unique_ip = getenv("CHAT_ENFORCE_UNIQUE_IP");
	if (enforce_unique_ip != NULL) {
		if (strcmp(enforce_unique_ip, "0") == 0 ||
			strcmp(enforce_unique_ip, "false") == 0 ||
			strcmp(enforce_unique_ip, "FALSE") == 0) {
			registry->enforce_unique_ip = 0;
		} else {
			registry->enforce_unique_ip = 1;
		}
	}

	if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
		return -2;
	}

	return 0;
}

void registry_destroy(UserRegistry *registry) {
	// libera lista enlazada bajo lock y destruye mutex
	if (registry == NULL) {
		return;
	}

	pthread_mutex_lock(&registry->mutex);

	UserNode *current = registry->head;
	while (current != NULL) {
		UserNode *next = current->next;
		free(current);
		current = next;
	}

	registry->head = NULL;

	pthread_mutex_unlock(&registry->mutex);
	pthread_mutex_destroy(&registry->mutex);
}

int registry_add_user(UserRegistry *registry, const char *username, const char *ip, int socket_fd) {
	// alta atomica de usuario con validaciones de unicidad
	if (registry == NULL || username == NULL || ip == NULL) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (strcmp(current->user.username, username) == 0) {
			pthread_mutex_unlock(&registry->mutex);
			return -2;
		}

		if (registry->enforce_unique_ip && strcmp(current->user.ip, ip) == 0) {
			pthread_mutex_unlock(&registry->mutex);
			return -3;
		}
	}

	UserNode *node = (UserNode *)calloc(1, sizeof(UserNode));
	if (node == NULL) {
		pthread_mutex_unlock(&registry->mutex);
		return -4;
	}

	strncpy(node->user.username, username, sizeof(node->user.username) - 1);
	strncpy(node->user.ip, ip, sizeof(node->user.ip) - 1);
	strncpy(node->user.status, "ACTIVO", sizeof(node->user.status) - 1);
	node->user.socket_fd = socket_fd;
	node->user.last_activity = time(NULL);

	node->next = registry->head;
	// inserta al inicio para costo o(1)
	registry->head = node;

	pthread_mutex_unlock(&registry->mutex);
	return 0;
}

int registry_remove_user(UserRegistry *registry, const char *username) {
	// elimina por username recorriendo lista enlazada
	if (registry == NULL || username == NULL) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	UserNode *prev = NULL;
	UserNode *current = registry->head;

	while (current != NULL) {
		if (strcmp(current->user.username, username) == 0) {
			if (prev == NULL) {
				registry->head = current->next;
			} else {
				prev->next = current->next;
			}

			free(current);
			pthread_mutex_unlock(&registry->mutex);
			return 0;
		}

		prev = current;
		current = current->next;
	}

	pthread_mutex_unlock(&registry->mutex);
	return -2;
}

int registry_remove_user_by_socket(UserRegistry *registry,
								   int socket_fd,
								   char *username_out,
								   size_t username_out_size) {
	// variante de borrado para cierre de conexion por socket
	if (registry == NULL) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	UserNode *prev = NULL;
	UserNode *current = registry->head;

	while (current != NULL) {
		if (current->user.socket_fd == socket_fd) {
			if (username_out != NULL && username_out_size > 0) {
				strncpy(username_out, current->user.username, username_out_size - 1);
				username_out[username_out_size - 1] = '\0';
			}

			if (prev == NULL) {
				registry->head = current->next;
			} else {
				prev->next = current->next;
			}

			free(current);
			pthread_mutex_unlock(&registry->mutex);
			return 0;
		}

		prev = current;
		current = current->next;
	}

	pthread_mutex_unlock(&registry->mutex);
	return -2;
}

int registry_get_user(UserRegistry *registry, const char *username, User *out_user) {
	if (registry == NULL || username == NULL || out_user == NULL) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (strcmp(current->user.username, username) == 0) {
			*out_user = current->user;
			pthread_mutex_unlock(&registry->mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&registry->mutex);
	return -2;
}

int registry_update_status(UserRegistry *registry, const char *username, const char *status) {
	if (registry == NULL || username == NULL || status == NULL) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (strcmp(current->user.username, username) == 0) {
			strncpy(current->user.status, status, sizeof(current->user.status) - 1);
			current->user.status[sizeof(current->user.status) - 1] = '\0';
			pthread_mutex_unlock(&registry->mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&registry->mutex);
	return -2;
}

int registry_touch_activity(UserRegistry *registry, const char *username, time_t now_ts) {
	if (registry == NULL || username == NULL) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (strcmp(current->user.username, username) == 0) {
			current->user.last_activity = now_ts;
			pthread_mutex_unlock(&registry->mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&registry->mutex);
	return -2;
}

int registry_mark_inactive_if_timeout(UserRegistry *registry, time_t now_ts) {
	// cambia estado a inactivo si pasa el timeout global
	if (registry == NULL) {
		return -1;
	}

	if (registry->inactivity_timeout_secs <= 0) {
		return 0;
	}

	int updated = 0;

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		double inactive_for = difftime(now_ts, current->user.last_activity);
		if (inactive_for >= registry->inactivity_timeout_secs &&
			strcmp(current->user.status, "INACTIVO") != 0) {
			strncpy(current->user.status, "INACTIVO", sizeof(current->user.status) - 1);
			current->user.status[sizeof(current->user.status) - 1] = '\0';
			updated++;
		}
	}

	pthread_mutex_unlock(&registry->mutex);
	return updated;
}

int registry_build_user_list(UserRegistry *registry, char *out_buffer, size_t out_size) {
	// arma csv de usernames en un solo buffer de salida
	if (registry == NULL || out_buffer == NULL || out_size == 0) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	size_t required = 1;
	size_t count = 0;
	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		required += strlen(current->user.username);
		if (count > 0) {
			required += 1;
		}
		count++;
	}

	if (required > out_size) {
		pthread_mutex_unlock(&registry->mutex);
		return -2;
	}

	out_buffer[0] = '\0';
	count = 0;

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (count > 0) {
			strcat(out_buffer, ",");
		}
		strcat(out_buffer, current->user.username);
		count++;
	}

	pthread_mutex_unlock(&registry->mutex);
	return 0;
}

int registry_username_from_socket(UserRegistry *registry,
								  int socket_fd,
								  char *username_out,
								  size_t username_out_size) {
	if (registry == NULL || username_out == NULL || username_out_size == 0) {
		return -1;
	}

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (current->user.socket_fd == socket_fd) {
			strncpy(username_out, current->user.username, username_out_size - 1);
			username_out[username_out_size - 1] = '\0';
			pthread_mutex_unlock(&registry->mutex);
			return 0;
		}
	}

	pthread_mutex_unlock(&registry->mutex);
	return -2;
}

int registry_send_to_user(UserRegistry *registry, const char *username, const Message *msg) {
	// busca socket destino bajo lock y envia fuera del lock
	if (registry == NULL || username == NULL || msg == NULL) {
		return -1;
	}

	int socket_fd = -1;

	pthread_mutex_lock(&registry->mutex);
	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (strcmp(current->user.username, username) == 0) {
			socket_fd = current->user.socket_fd;
			break;
		}
	}
	pthread_mutex_unlock(&registry->mutex);

	if (socket_fd < 0) {
		return -2;
	}

	if (send_message_to_socket(socket_fd, msg) < 0) {
		return -3;
	}

	return 0;
}

int registry_broadcast(UserRegistry *registry, const Message *msg, const char *exclude_username) {
	// snapshot de sockets para no bloquear mutex durante sends
	if (registry == NULL || msg == NULL) {
		return -1;
	}

	int *sockets = NULL;
	size_t count = 0;

	pthread_mutex_lock(&registry->mutex);

	for (UserNode *current = registry->head; current != NULL; current = current->next) {
		if (exclude_username != NULL && strcmp(current->user.username, exclude_username) == 0) {
			continue;
		}
		count++;
	}

	if (count > 0) {
		sockets = (int *)malloc(sizeof(int) * count);
		if (sockets == NULL) {
			pthread_mutex_unlock(&registry->mutex);
			return -2;
		}

		size_t idx = 0;
		for (UserNode *current = registry->head; current != NULL; current = current->next) {
			if (exclude_username != NULL && strcmp(current->user.username, exclude_username) == 0) {
				continue;
			}
			sockets[idx++] = current->user.socket_fd;
		}
	}

	pthread_mutex_unlock(&registry->mutex);

	int failed = 0;
	for (size_t i = 0; i < count; i++) {
		if (send_message_to_socket(sockets[i], msg) < 0) {
			failed++;
		}
	}

	free(sockets);

	return (failed == 0) ? 0 : -3;
}