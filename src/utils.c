#include "utils.h"

#include <string.h>
#include <ctype.h>

// quita saltos de linea que quedan de fgets()
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

// copia segura de string con bounds checking
int safe_str_copy(char *dest, size_t dest_size, const char *src) {
    if (dest == NULL || src == NULL || dest_size == 0) {
        return -1;
    }

    size_t len = strlen(src);
    // rechaza si fuente no cabe en destino
    if (len >= dest_size) {
        return -2;
    }

    memcpy(dest, src, len + 1);
    return 0;
}

// limpia un buffer comun
void limpiar_buffer(char *buffer, size_t size) {
    if (buffer == NULL) {
        return;
    }

    memset(buffer, 0, size);
}

// valida si cadena es vacia o solo espacios
int es_cadena_vacia(const char *s) {
    if (s == NULL) {
        return 1;
    }

    while (*s) {
        if (!isspace((unsigned char)*s)) {
            return 0;  // hay contenido no-blanco
        }
        s++;
    }

    return 1;  // solo espacios o vacio
}