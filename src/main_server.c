#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "server.h"

static void rtrim_in_place(char *s) {
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
	while (s != NULL && *s != '\0' && isspace((unsigned char)*s)) {
		s++;
	}
	return s;
}

static void strip_quotes(char *s) {
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

		setenv(key, value, 1);
	}

	fclose(fp);
}

static int resolve_timeout(void) {
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

int main(int argc, char const *argv[]) {
	load_dotenv(".env");

	if (argc != 2) {
		fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
		return 1;
	}

	int port = atoi(argv[1]);
	if (port <= 0 || port > 65535) {
		fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
		return 1;
	}

	int inactivity_timeout_secs = resolve_timeout();
	printf("timeout de inactividad: %d segundos\n", inactivity_timeout_secs);

	if (run_server(port, inactivity_timeout_secs) != 0) {
		return 1;
	}

	return 0;
}