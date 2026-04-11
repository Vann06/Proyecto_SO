#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void trim_newline(char *s);
int safe_str_copy(char *dest, size_t dest_size, const char *src);
void limpiar_buffer(char *buffer, size_t size);
int es_cadena_vacia(const char *s);

#endif