#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *OPERACIONES_VALIDAS[] = {
    "REGISTER",
    "EXIT",
    "STATUS",
    "LIST_REQ",
    "LIST_RES",
    "BROADCAST",
    "DM",
    "ERROR",
    "INFO_REQ",
    "INFO_RES"
};

static const size_t NUM_OPERACIONES =
    sizeof(OPERACIONES_VALIDAS) / sizeof(OPERACIONES_VALIDAS[0]);

static unsigned int sumar_bytes_cadena(const char *s) {
    unsigned int suma = 0;

    if (s == NULL) {
        return 0;
    }

    while (*s) {
        suma += (unsigned char)(*s);
        s++;
    }

    return suma;
}

int es_operacion_valida(const char *operacion) {
    if (operacion == NULL || operacion[0] == '\0') {
        return 0;
    }

    for (size_t i = 0; i < NUM_OPERACIONES; i++) {
        if (strcmp(operacion, OPERACIONES_VALIDAS[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

int es_estado_valido(const char *estado) {
    if (estado == NULL) {
        return 0;
    }

    return strcmp(estado, "ACTIVO") == 0 ||
           strcmp(estado, "OCUPADO") == 0 ||
           strcmp(estado, "INACTIVO") == 0;
}

unsigned int calcular_checksum(const Message *msg) {
    if (msg == NULL) {
        return 0;
    }

    unsigned int suma = 0;

    suma += sumar_bytes_cadena(msg->destinatario);
    suma += sumar_bytes_cadena(msg->origen);
    suma += sumar_bytes_cadena(msg->operacion);
    suma += (unsigned int)msg->longitud;

    for (int i = 0; i < msg->longitud; i++) {
        suma += (unsigned char)msg->cuerpo[i];
    }

    return suma % 65535;
}

int validar_checksum(const Message *msg) {
    if (msg == NULL) {
        return 0;
    }

    return calcular_checksum(msg) == msg->validacion;
}

int construir_mensaje(Message *msg,
                      const char *destinatario,
                      const char *origen,
                      const char *operacion,
                      const char *cuerpo) {
    if (msg == NULL || destinatario == NULL || origen == NULL || operacion == NULL) {
        return -1;
    }

    if (!es_operacion_valida(operacion)) {
        return -2;
    }

    size_t len_dest = strlen(destinatario);
    size_t len_orig = strlen(origen);
    size_t len_op = strlen(operacion);
    size_t len_body = (cuerpo != NULL) ? strlen(cuerpo) : 0;

    if (len_dest >= MAX_NAME || len_orig >= MAX_NAME ||
        len_op >= MAX_OPERATION || len_body > MAX_BODY) {
        return -3;
    }

    memset(msg, 0, sizeof(Message));

    strcpy(msg->destinatario, destinatario);
    strcpy(msg->origen, origen);
    strcpy(msg->operacion, operacion);

    if (cuerpo != NULL) {
        strcpy(msg->cuerpo, cuerpo);
    }

    msg->longitud = (int)len_body;
    msg->validacion = calcular_checksum(msg);

    return 0;
}

int serializar_mensaje(const Message *msg,
                       char *buffer,
                       size_t buffer_size,
                       size_t *bytes_escritos) {
    if (msg == NULL || buffer == NULL || bytes_escritos == NULL) {
        return -1;
    }

    if (!es_operacion_valida(msg->operacion)) {
        return -2;
    }

    if (msg->longitud < 0 || msg->longitud > MAX_BODY) {
        return -3;
    }

    unsigned int checksum = calcular_checksum(msg);

    int header_len = snprintf(buffer,
                              buffer_size,
                              "%s|%s|%s|%d|%u\n",
                              msg->destinatario,
                              msg->origen,
                              msg->operacion,
                              msg->longitud,
                              checksum);

    if (header_len < 0) {
        return -4;
    }

    if ((size_t)header_len >= buffer_size) {
        return -5;
    }

    if ((size_t)header_len + (size_t)msg->longitud >= buffer_size) {
        return -6;
    }

    if (msg->longitud > 0) {
        memcpy(buffer + header_len, msg->cuerpo, (size_t)msg->longitud);
    }

    buffer[header_len + msg->longitud] = '\0';
    *bytes_escritos = (size_t)header_len + (size_t)msg->longitud;

    return 0;
}

int deserializar_mensaje(const char *buffer,
                         size_t bytes_recibidos,
                         Message *msg) {
    if (buffer == NULL || msg == NULL || bytes_recibidos == 0) {
        return -1;
    }

    memset(msg, 0, sizeof(Message));

    const char *salto = NULL;
    for (size_t i = 0; i < bytes_recibidos; i++) {
        if (buffer[i] == '\n') {
            salto = &buffer[i];
            break;
        }
    }

    if (salto == NULL) {
        return -2;
    }

    size_t header_size = (size_t)(salto - buffer);

    char header[MAX_SERIALIZED];
    if (header_size >= sizeof(header)) {
        return -3;
    }

    memcpy(header, buffer, header_size);
    header[header_size] = '\0';

    char *token = strtok(header, "|");
    if (token == NULL) return -4;
    if (strlen(token) >= MAX_NAME) return -5;
    strcpy(msg->destinatario, token);

    token = strtok(NULL, "|");
    if (token == NULL) return -6;
    if (strlen(token) >= MAX_NAME) return -7;
    strcpy(msg->origen, token);

    token = strtok(NULL, "|");
    if (token == NULL) return -8;
    if (strlen(token) >= MAX_OPERATION) return -9;
    if (!es_operacion_valida(token)) return -10;
    strcpy(msg->operacion, token);

    token = strtok(NULL, "|");
    if (token == NULL) return -11;
    msg->longitud = atoi(token);
    if (msg->longitud < 0 || msg->longitud > MAX_BODY) return -12;

    token = strtok(NULL, "|");
    if (token == NULL) return -13;
    msg->validacion = (unsigned int)strtoul(token, NULL, 10);

    size_t body_offset = header_size + 1;
    size_t body_disponible = (bytes_recibidos > body_offset) ? (bytes_recibidos - body_offset) : 0;

    if ((size_t)msg->longitud > body_disponible) {
        return -14;
    }

    if (msg->longitud > 0) {
        memcpy(msg->cuerpo, buffer + body_offset, (size_t)msg->longitud);
    }
    msg->cuerpo[msg->longitud] = '\0';

    if (!validar_checksum(msg)) {
        return -15;
    }

    return 0;
}