#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static size_t compute_message_size(const char *buffer, size_t bytes_in_buffer, const Message *msg) {
	const void *line_break = memchr(buffer, '\n', bytes_in_buffer);
	if (line_break == NULL) {
		return 0;
	}

	size_t header_size = (size_t)((const char *)line_break - buffer) + 1;
	return header_size + (size_t)msg->longitud;
}

void *client_thread_func(void *arg) {
	ClientThreadArgs *thread_args = (ClientThreadArgs *)arg;
	if (thread_args == NULL) {
		return NULL;
	}

	int client_socket = thread_args->client_socket;
	char client_ip[INET_ADDRSTRLEN];
	UserRegistry *registry = thread_args->registry;

	strncpy(client_ip, thread_args->client_ip, sizeof(client_ip) - 1);
	client_ip[sizeof(client_ip) - 1] = '\0';

	free(thread_args);

	char recv_buffer[SERVER_RECV_BUFFER];
	int bytes_in_buffer = 0;
	int keep_running = 1;

	while (keep_running) {
		int free_space = SERVER_RECV_BUFFER - bytes_in_buffer - 1;
		if (free_space <= 0) {
			send_error_response(registry, client_socket, "buffer de recepcion saturado");
			bytes_in_buffer = 0;
			free_space = SERVER_RECV_BUFFER - 1;
		}

		int bytes_read = recv(client_socket, recv_buffer + bytes_in_buffer, free_space, 0);
		if (bytes_read <= 0) {
			break;
		}

		bytes_in_buffer += bytes_read;
		recv_buffer[bytes_in_buffer] = '\0';

		while (bytes_in_buffer > 0) {
			Message msg;
			int parse_res = deserializar_mensaje(recv_buffer, (size_t)bytes_in_buffer, &msg);

			if (parse_res == 0) {
				int should_close = 0;
				int handle_res = handle_client_message(registry,
													   client_socket,
													   client_ip,
													   &msg,
													   &should_close);
				if (handle_res < 0) {
					send_error_response(registry, client_socket, "error interno en procesamiento");
				}

				size_t msg_size = compute_message_size(recv_buffer,
													   (size_t)bytes_in_buffer,
													   &msg);
				if (msg_size == 0 || msg_size > (size_t)bytes_in_buffer) {
					bytes_in_buffer = 0;
				} else {
					memmove(recv_buffer,
							recv_buffer + msg_size,
							(size_t)bytes_in_buffer - msg_size);
					bytes_in_buffer -= (int)msg_size;
				}

				if (should_close) {
					keep_running = 0;
					break;
				}

				continue;
			}

			if (parse_res == -2 || parse_res == -14) {
				break;
			}

			send_error_response(registry, client_socket, "mensaje invalido");
			bytes_in_buffer = 0;
			break;
		}
	}

	char removed_username[MAX_NAME] = {0};
	registry_remove_user_by_socket(registry,
								   client_socket,
								   removed_username,
								   sizeof(removed_username));

	close(client_socket);
	return NULL;
}

int run_server(int port, int inactivity_timeout_secs) {
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		perror("error al crear socket del servidor");
		return -1;
	}

	int reuse_addr = 1;
	if (setsockopt(server_socket,
				   SOL_SOCKET,
				   SO_REUSEADDR,
				   &reuse_addr,
				   sizeof(reuse_addr)) < 0) {
		perror("error al configurar so_reuseaddr");
		close(server_socket);
		return -2;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons((uint16_t)port);

	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("error en bind del servidor");
		close(server_socket);
		return -3;
	}

	if (listen(server_socket, 16) < 0) {
		perror("error en listen del servidor");
		close(server_socket);
		return -4;
	}

	UserRegistry registry;
	if (registry_init(&registry, inactivity_timeout_secs) != 0) {
		fprintf(stderr, "no se pudo inicializar el registro de usuarios\n");
		close(server_socket);
		return -5;
	}

	printf("Servidor escuchando en puerto %d...\n", port);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_socket = accept(server_socket,
								   (struct sockaddr *)&client_addr,
								   &client_addr_len);

		if (client_socket < 0) {
			if (errno == EINTR) {
				continue;
			}
			perror("error al aceptar cliente");
			continue;
		}

		char client_ip[INET_ADDRSTRLEN] = {0};
		if (inet_ntop(AF_INET,
					  &client_addr.sin_addr,
					  client_ip,
					  sizeof(client_ip)) == NULL) {
			strncpy(client_ip, "0.0.0.0", sizeof(client_ip) - 1);
		}

		ClientThreadArgs *thread_args = (ClientThreadArgs *)calloc(1, sizeof(ClientThreadArgs));
		if (thread_args == NULL) {
			close(client_socket);
			continue;
		}

		thread_args->client_socket = client_socket;
		thread_args->registry = &registry;
		strncpy(thread_args->client_ip, client_ip, sizeof(thread_args->client_ip) - 1);

		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, client_thread_func, thread_args) != 0) {
			close(client_socket);
			free(thread_args);
			continue;
		}

		pthread_detach(thread_id);
	}

	registry_destroy(&registry);
	close(server_socket);
	return 0;
}