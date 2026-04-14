#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "common.h"
#include "server.h"

static void rtrim_in_place(char *s) {
	// limpia espacios al final para parsear bien el .env
	if (s == NULL) {
		return;
	}

	size_t len = strlen(s);
	while (len > 0 && isspace((unsigned char)s[len - 1])) {
		s[len - 1] = '\0';
		len--;
	}
}

static char *ltrim(char *s) {
	// salta espacios al inicio
	while (s != NULL && *s != '\0' && isspace((unsigned char)*s)) {
		s++;
	}
	return s;
}

static void strip_quotes(char *s) {
	// quita comillas si vienen tipo "valor" o 'valor'
	if (s == NULL) {
		return;
	}

	size_t len = strlen(s);
	if (len >= 2 && ((s[0] == '"' && s[len - 1] == '"') || (s[0] == '\'' && s[len - 1] == '\''))) {
		memmove(s, s + 1, len - 2);
		s[len - 2] = '\0';
	}
}

static void load_dotenv(const char *file_path) {
	// loader simple de .env para no depender de librerias externas
	FILE *fp = fopen(file_path, "r");
	if (fp == NULL) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), fp) != NULL) {
		char *entry = ltrim(line);
		if (entry == NULL || entry[0] == '\0' || entry[0] == '#') {
			continue;
		}

		rtrim_in_place(entry);
		if (entry[0] == '\0') {
			continue;
		}

		char *equal_sign = strchr(entry, '=');
		if (equal_sign == NULL) {
			continue;
		}

		*equal_sign = '\0';
		char *key = ltrim(entry);
		char *value = ltrim(equal_sign + 1);

		rtrim_in_place(key);
		rtrim_in_place(value);
		strip_quotes(value);

		if (key[0] == '\0') {
			continue;
		}

		// sobreescribe variables para que .env mande
		setenv(key, value, 1);
	}

	fclose(fp);
}

static int resolve_timeout(void) {
	// toma timeout del entorno o usa default
	int timeout = DEFAULT_INACTIVITY_TIMEOUT;
	const char *env_timeout = getenv("CHAT_INACTIVITY_TIMEOUT");

	if (env_timeout != NULL) {
		int parsed = atoi(env_timeout);
		if (parsed > 0) {
			timeout = parsed;
		}
	}

	return timeout;
}

static int parse_port(const char *value, int *out_port) {
	// parseo estricto del puerto para evitar basura tipo 8080abc
	char *endptr = NULL;
	errno = 0;
	long parsed = strtol(value, &endptr, 10);

	if (value == endptr || *endptr != '\0' || errno != 0 || parsed <= 0 || parsed > 65535) {
		return -1;
	}

	*out_port = (int)parsed;
	return 0;
}

static int resolve_port(int argc, char const *argv[], int *out_port) {
	// prioridad puerto: argumento -> CHAT_PORT -> default compilado
	const char *source = "valor por defecto";
	const char *value = NULL;

	if (argc == 2) {
		source = "argumento";
		value = argv[1];
	} else {
		value = getenv("CHAT_PORT");
		if (value != NULL && value[0] != '\0') {
			source = "CHAT_PORT";
		} else {
			*out_port = PORT;
			return 0;
		}
	}

	if (parse_port(value, out_port) != 0) {
		fprintf(stderr, "Puerto invalido (%s): %s\n", source, value);
		return -1;
	}

	return 0;
}

int main(int argc, char const *argv[]) {
	load_dotenv(".env");

	if (argc > 2) {
		fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
		fprintf(stderr, "Si no se especifica puerto, se usa CHAT_PORT de .env o PORT por defecto.\n");
		return 1;
	}

	int port = PORT;
	// deja el puerto final listo segun prioridad definida
	if (resolve_port(argc, argv, &port) != 0) {
		return 1;
	}

	int inactivity_timeout_secs = resolve_timeout();
	printf("timeout de inactividad: %d segundos\n", inactivity_timeout_secs);

	if (run_server(port, inactivity_timeout_secs) != 0) {
		return 1;
	}

	return 0;
}