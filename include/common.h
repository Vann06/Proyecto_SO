#ifndef COMMON_H
#define COMMON_H

// definiciones comunes para cliente y servidor

// puerto por defecto (se puede sobreescribir con CHAT_PORT en .env o argumento CLI)
#define PORT 8080

// limites de campos del mensaje
#define MAX_NAME 32              // usuario, destinatario
#define MAX_OPERATION 20         // tipo de operacion
#define MAX_STATUS 16            // ACTIVO, OCUPADO, INACTIVO
#define MAX_BODY 512             // contenido del mensaje
#define MAX_SERIALIZED 1024      // mensaje serializado completo

#endif