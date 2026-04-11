#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include "common.h"

typedef struct {
    char destinatario[MAX_NAME];
    char origen[MAX_NAME];
    char operacion[MAX_OPERATION];
    int longitud;
    unsigned int validacion;
    char cuerpo[MAX_BODY + 1];
} Message;

/* Construcción y validación */
int construir_mensaje(Message *msg,
                      const char *destinatario,
                      const char *origen,
                      const char *operacion,
                      const char *cuerpo);

unsigned int calcular_checksum(const Message *msg);
int validar_checksum(const Message *msg);
int es_operacion_valida(const char *operacion);
int es_estado_valido(const char *estado);

/* Serialización:
   Formato:
   DESTINATARIO|ORIGEN|OPERACION|LONGITUD|VALIDACION\n
   CUERPO
*/
int serializar_mensaje(const Message *msg,
                       char *buffer,
                       size_t buffer_size,
                       size_t *bytes_escritos);

int deserializar_mensaje(const char *buffer,
                         size_t bytes_recibidos,
                         Message *msg);

#endif