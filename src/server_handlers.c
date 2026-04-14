#include "server.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "utils.h"

static int send_all_bytes(int socket_fd, const char *data, size_t size) {
	// envia todo el payload aunque el kernel lo corte en partes
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

static int send_message_to_socket(int socket_fd,
								  const char *destinatario,
								  const char *origen,
								  const char *operacion,
								  const char *cuerpo) {
	// helper comun para armar+serializar+enviar respuestas
	Message out_msg;
	char buffer[MAX_SERIALIZED];
	size_t written = 0;

	if (construir_mensaje(&out_msg, destinatario, origen, operacion, cuerpo) < 0) {
		return -1;
	}

	if (serializar_mensaje(&out_msg, buffer, sizeof(buffer), &written) < 0) {
		return -2;
	}

	return send_all_bytes(socket_fd, buffer, written);
}

int send_error_response(UserRegistry *registry, int client_socket, const char *error_text) {
	// intenta responder al username real si ya esta registrado
	char username[MAX_NAME] = {0};
	const char *dest = "CLIENT";

	if (registry != NULL &&
		registry_username_from_socket(registry, client_socket, username, sizeof(username)) == 0) {
		dest = username;
	}

	if (send_message_to_socket(client_socket, dest, "SERVER", "ERROR", error_text) < 0) {
		return -1;
	}

	return 0;
}

static int handle_register(UserRegistry *registry,
						   int client_socket,
						   const char *client_ip,
						   const Message *msg) {
	// registra usuario nuevo y devuelve info base de sesion
	if (es_cadena_vacia(msg->origen)) {
		send_error_response(registry, client_socket, "nombre de usuario invalido");
		return 0;
	}

	int add_res = registry_add_user(registry, msg->origen, client_ip, client_socket);
	if (add_res == -2) {
		send_error_response(registry, client_socket, "usuario duplicado");
		return 0;
	}

	if (add_res == -3) {
		send_error_response(registry, client_socket, "ip duplicada");
		return 0;
	}

	if (add_res != 0) {
		send_error_response(registry, client_socket, "error interno de registro");
		return 0;
	}

	char info_body[MAX_BODY + 1];
	snprintf(info_body, sizeof(info_body), "%s,%s,%s", msg->origen, client_ip, "ACTIVO");

	if (send_message_to_socket(client_socket, msg->origen, "SERVER", "INFO_RES", info_body) < 0) {
		return -1;
	}

	return 0;
}

static int validate_origin(UserRegistry *registry,
						   int client_socket,
						   const Message *msg,
						   char *username_from_socket,
						   size_t username_size) {
	// evita suplantacion de origen comparando socket vs campo origen
	if (registry_username_from_socket(registry,
									  client_socket,
									  username_from_socket,
									  username_size) != 0) {
		send_error_response(registry, client_socket, "usuario no registrado");
		return -1;
	}

	if (strcmp(username_from_socket, msg->origen) != 0) {
		send_error_response(registry, client_socket, "origen invalido");
		return -1;
	}

	return 0;
}

static int handle_list_request(UserRegistry *registry, int client_socket, const char *requester) {
	char list_body[MAX_BODY + 1];
	if (registry_build_user_list(registry, list_body, sizeof(list_body)) < 0) {
		send_error_response(registry, client_socket, "no se pudo construir listado");
		return 0;
	}

	if (send_message_to_socket(client_socket, requester, "SERVER", "LIST_RES", list_body) < 0) {
		return -1;
	}

	return 0;
}

static int handle_info_request(UserRegistry *registry, int client_socket, const Message *msg) {
	if (msg->longitud <= 0 || es_cadena_vacia(msg->cuerpo)) {
		send_error_response(registry, client_socket, "usuario destino invalido");
		return 0;
	}

	User user;
	if (registry_get_user(registry, msg->cuerpo, &user) != 0) {
		send_error_response(registry, client_socket, "usuario no existe");
		return 0;
	}

	char info_body[MAX_BODY + 1];
	snprintf(info_body,
			 sizeof(info_body),
			 "%s,%s,%s",
			 user.username,
			 user.ip,
			 user.status);

	if (send_message_to_socket(client_socket, msg->origen, "SERVER", "INFO_RES", info_body) < 0) {
		return -1;
	}

	return 0;
}

static int handle_status_change(UserRegistry *registry,
								int client_socket,
								const char *requester,
								const Message *msg) {
	if (!es_estado_valido(msg->cuerpo)) {
		send_error_response(registry, client_socket, "estado invalido");
		return 0;
	}

	if (registry_update_status(registry, requester, msg->cuerpo) != 0) {
		send_error_response(registry, client_socket, "usuario no registrado");
		return 0;
	}

	registry_touch_activity(registry, requester, time(NULL));

	char body[MAX_BODY + 1];
	snprintf(body, sizeof(body), "%s", msg->cuerpo);

	if (send_message_to_socket(client_socket, requester, "SERVER", "INFO_RES", body) < 0) {
		return -1;
	}

	return 0;
}

static int handle_direct_message(UserRegistry *registry,
								 int client_socket,
								 const char *sender,
								 const Message *msg) {
	// dm solo acepta destinatario usuario valido, no server/all
	if (es_cadena_vacia(msg->destinatario) || strcmp(msg->destinatario, "SERVER") == 0 ||
		strcmp(msg->destinatario, "ALL") == 0) {
		send_error_response(registry, client_socket, "destinatario invalido");
		return 0;
	}

	Message dm;
	if (construir_mensaje(&dm, msg->destinatario, sender, "DM", msg->cuerpo) < 0) {
		send_error_response(registry, client_socket, "mensaje directo invalido");
		return 0;
	}

	int send_res = registry_send_to_user(registry, msg->destinatario, &dm);
	if (send_res != 0) {
		send_error_response(registry, client_socket, "usuario destino no existe");
		return 0;
	}

	return 0;
}

static int handle_broadcast(UserRegistry *registry,
							int client_socket,
							const char *sender,
							const Message *msg) {
	Message out;
	if (construir_mensaje(&out, "ALL", sender, "BROADCAST", msg->cuerpo) < 0) {
		send_error_response(registry, client_socket, "mensaje broadcast invalido");
		return 0;
	}

	if (registry_broadcast(registry, &out, sender) != 0) {
		send_error_response(registry, client_socket, "fallo de broadcasting");
		return 0;
	}

	return 0;
}

int handle_client_message(UserRegistry *registry,
						  int client_socket,
						  const char *client_ip,
						  Message *msg,
						  int *should_close) {
	// router principal de operaciones del protocolo
	if (registry == NULL || client_ip == NULL || msg == NULL || should_close == NULL) {
		return -1;
	}

	*should_close = 0;

	registry_mark_inactive_if_timeout(registry, time(NULL));

	if (strcmp(msg->operacion, "REGISTER") == 0) {
		return handle_register(registry, client_socket, client_ip, msg);
	}

	char requester[MAX_NAME];
	if (validate_origin(registry, client_socket, msg, requester, sizeof(requester)) != 0) {
		return 0;
	}

	registry_touch_activity(registry, requester, time(NULL));
	// cualquier accion normal reactiva al usuario
	if (strcmp(msg->operacion, "STATUS") != 0 && strcmp(msg->operacion, "EXIT") != 0) {
		registry_update_status(registry, requester, "ACTIVO");
	}

	if (strcmp(msg->operacion, "LIST_REQ") == 0) {
		return handle_list_request(registry, client_socket, requester);
	}

	if (strcmp(msg->operacion, "INFO_REQ") == 0) {
		return handle_info_request(registry, client_socket, msg);
	}

	if (strcmp(msg->operacion, "STATUS") == 0) {
		return handle_status_change(registry, client_socket, requester, msg);
	}

	if (strcmp(msg->operacion, "DM") == 0) {
		return handle_direct_message(registry, client_socket, requester, msg);
	}

	if (strcmp(msg->operacion, "BROADCAST") == 0) {
		return handle_broadcast(registry, client_socket, requester, msg);
	}

	if (strcmp(msg->operacion, "EXIT") == 0) {
		registry_remove_user(registry, requester);
		*should_close = 1;
		return 0;
	}

	send_error_response(registry, client_socket, "operacion no soportada por servidor");
	return 0;
}