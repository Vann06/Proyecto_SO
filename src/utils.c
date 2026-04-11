#include "utils.h"

#include <string.h>
#include <ctype.h>

void trim_newline(char *s) {
    if (s == NULL) {
        return;
    }

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

int safe_str_copy(char *dest, size_t dest_size, const char *src) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return -1;
    }

    size_t len = strlen(src);
    if (len >= dest_size) {
        return -2;
    }

    memcpy(dest, src, len + 1);
    return 0;
}

void limpiar_buffer(char *buffer, size_t size) {
    if (buffer == NULL) {
        return;
    }

    memset(buffer, 0, size);
}

int es_cadena_vacia(const char *s) {
    if (s == NULL) {
        return 1;
    }

    while (*s) {
        if (!isspace((unsigned char)*s)) {
            return 0;
        }
        s++;
    }

    return 1;
}