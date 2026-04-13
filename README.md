# Chat Multicliente en C con Sockets TCP

Proyecto cliente-servidor en C11 para chat concurrente sobre TCP, con protocolo propio, manejo de estados por usuario y validación de mensajes.

## Estado del proyecto

Implementado:
- servidor multicliente con hilos (pthread), un hilo por cliente
- registro y administración thread-safe de usuarios conectados
- operaciones: REGISTER, LIST_REQ, INFO_REQ, STATUS, DM, BROADCAST, EXIT
- recepción asíncrona en cliente (hilo receptor + UI interactiva)
- validación de protocolo (longitud, checksum, formato y operación)
- timeout de inactividad configurable por entorno
- automatización E2E en WSL: [scripts/e2e_wsl.sh](scripts/e2e_wsl.sh)

## Estructura principal

```text
.
├── Makefile
├── README.md
├── include/
│   ├── client.h
│   ├── client_receiver.h
│   ├── client_ui.h
│   ├── common.h
│   ├── protocol.h
│   ├── server.h
│   ├── user.h
│   └── utils.h
├── src/
│   ├── client.c
│   ├── client_receiver.c
│   ├── client_ui.c
│   ├── main_client.c
│   ├── main_server.c
│   ├── protocol.c
│   ├── server.c
│   ├── server_handlers.c
│   ├── server_registry.c
│   └── utils.c
├── docs/
│   ├── analisis_concurrencia.md
│   └── protocolo.md
└── scripts/
    └── e2e_wsl.sh
```

## Build y ejecución

El flujo recomendado es en WSL (Ubuntu).

1. Compilar:

```bash
make clean
make all
```

2. Ejecutar servidor:

```bash
./server <puerto>
```

3. Ejecutar clientes:

```bash
./client <usuario> <ip_servidor> <puerto_servidor>
```

Ejemplo:

```bash
./server 8080
./client alice 127.0.0.1 8080
./client bob 127.0.0.1 8080
```

## Variables de entorno

El servidor lee `.env` al iniciar:

- `CHAT_ENV`: `production` o `testing`
- `CHAT_INACTIVITY_TIMEOUT`: timeout en segundos (default 180)
- `CHAT_ENFORCE_UNIQUE_IP`: `1` o `0` (override explícito)

Referencia: [.env.example](.env.example)

Comportamiento esperado:
- en `production`, se fuerza unicidad de IP por defecto
- en `testing`, se permite misma IP para facilitar pruebas locales

## Validación E2E

Ejecución:

```bash
./scripts/e2e_wsl.sh
```

Salida esperada (resumen):

```text
=== summary ===
pass: 5
fail: 0
```

El reporte completo queda en [e2e_report.txt](e2e_report.txt).

## Matriz de evidencia (rúbrica servidor)

| Criterio | Evidencia de implementación | Evidencia de validación |
|---|---|---|
| Servidor concurrente multicliente | `src/server.c` (accept loop + `pthread_create` + `pthread_detach`) | caso `estres basico concurrente` |
| Registro y unicidad de usuarios | `src/server_handlers.c` (`REGISTER`), `src/server_registry.c` (`registry_add_user`) | casos `production bloquea ip duplicada`, `testing permite misma ip` |
| Consulta de usuarios conectados | `src/server_handlers.c` (`LIST_REQ`), `src/server_registry.c` (`registry_build_user_list`) | casos `flujo core con timeout inactivo`, `exit limpia sesion` |
| Consulta de información de usuario | `src/server_handlers.c` (`INFO_REQ`) | caso `flujo core con timeout inactivo` |
| Cambio de estado y timeout | `src/server_handlers.c` (`STATUS`), `src/server_registry.c` (`registry_mark_inactive_if_timeout`) | caso `flujo core con timeout inactivo` |
| Mensajería directa y broadcast | `src/server_handlers.c` (`DM`, `BROADCAST`), `src/server_registry.c` (`registry_send_to_user`, `registry_broadcast`) | casos `flujo core con timeout inactivo`, `estres basico concurrente` |
| Cierre de sesión y limpieza | `src/server_handlers.c` (`EXIT`), `src/server_registry.c` (`registry_remove_user*`) | caso `exit limpia sesion` |
| Manejo de errores de protocolo/operación | `src/server.c` (parse y descarte), `src/server_handlers.c` (`send_error_response`) | reporte E2E + pruebas manuales de comandos inválidos |

## Documentación técnica

- protocolo de mensajes: [docs/protocolo.md](docs/protocolo.md)
- análisis de concurrencia y sincronización: [docs/analisis_concurrencia.md](docs/analisis_concurrencia.md)
