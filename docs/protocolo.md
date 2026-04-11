# Protocolo de Comunicación

## 1. Introducción

El sistema de chat utiliza un protocolo de comunicación propio entre cliente y servidor. Su objetivo es definir una estructura uniforme para enviar y recibir mensajes, de manera que ambos extremos interpreten correctamente cada operación.

El protocolo fue diseñado para ser simple, consistente y fácil de implementar en C sobre sockets TCP.

## 2. Objetivos específicos

- Definir un formato estándar para todos los mensajes.
- Permitir registro, consulta, mensajería y cierre de sesión.
- Validar la integridad de cada mensaje.
- Separar claramente encabezado y cuerpo del mensaje.

## 3. Supuestos

- La comunicación será por TCP.
- Los mensajes serán de texto.
- Los nombres de usuario serán únicos.
- El cuerpo del mensaje tendrá un tamaño máximo de 512 caracteres.
- Los estados válidos serán ACTIVO, OCUPADO e INACTIVO.

## 4. Alcance

El protocolo cubre:

- Registro de usuario
- Consulta de usuarios conectados
- Consulta de información de usuario
- Cambio de estado
- Mensaje directo
- Broadcast
- Cierre de sesión
- Manejo de errores

No cubre:

- Transferencia de archivos
- Cifrado
- Historial persistente
- Autenticación con contraseña

## 5. Estructura del mensaje

Formato general:

DESTINATARIO|ORIGEN|OPERACION|LONGITUD|VALIDACION
CUERPO

## 6. Significado de los campos

### DESTINATARIO
Indica a quién va dirigido el mensaje. Puede ser SERVER, ALL o el nombre de un usuario.

### ORIGEN
Indica quién envía el mensaje.

### OPERACION
Acción que se desea ejecutar.

### LONGITUD
Número de bytes del cuerpo.

### VALIDACION
Checksum simple para verificar integridad.

### CUERPO
Contenido de la operación.

## 7. Operaciones válidas

- REGISTER
- EXIT
- STATUS
- LIST_REQ
- LIST_RES
- BROADCAST
- DM
- ERROR
- INFO_REQ
- INFO_RES

## 8. Estados válidos

- ACTIVO
- OCUPADO
- INACTIVO

## 9. Ejemplos

### Registro
SERVER|Ana|REGISTER|3|0
Ana

### Lista de usuarios
SERVER|Ana|LIST_REQ|0|0

### Respuesta de lista
Ana|SERVER|LIST_RES|14|0
Ana,Luis,Pedro

### Información de usuario
SERVER|Ana|INFO_REQ|4|0
Luis

### Mensaje directo
Luis|Ana|DM|10|0
hola luis

### Broadcast
ALL|Ana|BROADCAST|12|0
hola a todos

### Cambio de estado
SERVER|Ana|STATUS|7|0
OCUPADO

### Error
Ana|SERVER|ERROR|19|0
usuario no existe

## 10. Flujo de conexión

1. El cliente establece conexión TCP con el servidor.
2. El cliente envía REGISTER.
3. El servidor valida y responde.
4. Durante la sesión el cliente puede enviar LIST_REQ, INFO_REQ, STATUS, DM, BROADCAST y EXIT.
5. El servidor procesa y responde según la operación.
6. El cliente envía EXIT para cerrar la sesión.

## 11. Manejo de errores

Se responderá con ERROR cuando:

- la operación no exista
- el usuario destino no exista
- el formato del mensaje sea inválido
- el checksum no coincida
- la longitud sea inconsistente