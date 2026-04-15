#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// quita saltos de linea del final (usada despues de fgets)
void trim_newline(char *s);

// copia string segura con validacion de tamaño
int safe_str_copy(char *dest, size_t dest_size, const char *src);

// rellena buffer con ceros
void limpiar_buffer(char *buffer, size_t size);

// valida si cadena es nula, vacia o solo espacios
int es_cadena_vacia(const char *s);

#endif