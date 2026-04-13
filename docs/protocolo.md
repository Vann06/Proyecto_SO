# Protocolo de Comunicación

## 1. Formato general

Cada mensaje se envía como:

```text
DESTINATARIO|ORIGEN|OPERACION|LONGITUD|VALIDACION
CUERPO
```

- La primera línea es el encabezado.
- La segunda línea es el cuerpo.
- `LONGITUD` representa los bytes del cuerpo.
- `VALIDACION` es un checksum simple del cuerpo.

## 2. Campos

- `DESTINATARIO`: `SERVER`, `ALL` o username específico.
- `ORIGEN`: username del emisor (o `SERVER` para respuestas del servidor).
- `OPERACION`: tipo de operación.
- `LONGITUD`: tamaño del cuerpo.
- `VALIDACION`: checksum del cuerpo.
- `CUERPO`: payload de la operación.

## 3. Operaciones válidas

- `REGISTER`
- `EXIT`
- `STATUS`
- `LIST_REQ`
- `LIST_RES`
- `BROADCAST`
- `DM`
- `ERROR`
- `INFO_REQ`
- `INFO_RES`

## 4. Estados válidos

- `ACTIVO`
- `OCUPADO`
- `INACTIVO`

## 5. Semántica por operación

### `REGISTER`

Solicitud del cliente para registrarse.

- Destino: `SERVER`
- Cuerpo: vacío
- Respuesta exitosa: `INFO_RES` con `username,ip,ACTIVO`
- Respuesta de error: `ERROR` (`usuario duplicado`, `ip duplicada`, etc.)

### `LIST_REQ` / `LIST_RES`

Consulta de usuarios conectados.

- `LIST_REQ`: destino `SERVER`, cuerpo vacío
- `LIST_RES`: respuesta del servidor con usernames separados por coma
- Ejemplo de cuerpo: `alice,bob,charlie`

### `INFO_REQ` / `INFO_RES`

Consulta de información de un usuario.

- `INFO_REQ`: cuerpo con username objetivo
- `INFO_RES`: `username,ip,status`

### `STATUS`

Cambio de estado del usuario emisor.

- Destino: `SERVER`
- Cuerpo: `ACTIVO`, `OCUPADO` o `INACTIVO`
- Respuesta: `INFO_RES` con el estado aplicado en cuerpo

### `DM`

Mensaje directo entre usuarios.

- Destino: username objetivo
- Cuerpo: mensaje textual
- Si el usuario destino no existe, el emisor recibe `ERROR`

### `BROADCAST`

Mensaje para todos los usuarios conectados, excluyendo al emisor.

- Destino: `ALL`
- Cuerpo: mensaje textual

### `EXIT`

Cierre de sesión.

- Destino: `SERVER`
- Cuerpo: vacío
- El servidor elimina al usuario del registro y cierra su sesión

### `ERROR`

Respuesta del servidor ante solicitudes inválidas o inconsistentes.

## 6. Validaciones del servidor

El servidor valida:

- estructura de encabezado
- operación válida
- consistencia `LONGITUD` vs cuerpo recibido
- checksum (`VALIDACION`)
- coherencia `ORIGEN` con el socket registrado

Si una validación falla, responde `ERROR` y puede descartar el mensaje inválido.

## 7. Flujo de sesión

1. Cliente abre TCP hacia servidor.
2. Cliente envía `REGISTER`.
3. Servidor responde `INFO_RES` o `ERROR`.
4. Cliente opera con `LIST_REQ`, `INFO_REQ`, `STATUS`, `DM`, `BROADCAST`.
5. Cliente envía `EXIT` para terminar sesión.

## 8. Notas de implementación

- El timeout de inactividad se configura con `CHAT_INACTIVITY_TIMEOUT`.
- El modo `CHAT_ENV=testing` permite múltiples clientes desde la misma IP para pruebas locales.
- En modo `production`, por defecto se fuerza unicidad de IP (salvo override explícito).