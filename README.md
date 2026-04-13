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

## Guía de uso paso a paso

### 1. Requisitos previos

- WSL con Ubuntu (recomendado para compilar y correr)
- `gcc`, `make` y `pthread` (vienen con `build-essential`)

Si faltan herramientas en Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential
```

### 2. Entrar al proyecto

Desde Ubuntu WSL, muévete a la carpeta del proyecto:

```bash
cd Proyecto_SO
```

Si ya estás dentro del repositorio, no necesitas hacer este paso.

### 3. Configurar entorno (opcional, pero recomendado)

El servidor lee un archivo `.env` al arrancar.

```bash
cp .env.example .env
```

Valores recomendados para pruebas locales:

```env
CHAT_ENV=testing
CHAT_INACTIVITY_TIMEOUT=180
CHAT_ENFORCE_UNIQUE_IP=0
```

### 4. Compilar

```bash
make clean
make all
```

Al terminar, tendrás dos ejecutables:

- `server`
- `client`

### 5. Iniciar servidor (Terminal 1)

```bash
./server 8080
```

Si todo va bien, verás un mensaje parecido a:

```text
Servidor escuchando en puerto 8080...
```

### 6. Conectar clientes (Terminal 2 y 3)

En una segunda terminal:

```bash
./client alice 127.0.0.1 8080
```

En una tercera terminal:

```bash
./client bob 127.0.0.1 8080
```

Cuando el registro funciona, cada cliente muestra:

```text
Registro exitoso: <usuario>,<ip>,ACTIVO
```

### 7. Comandos del chat y ejemplos reales

Todos estos comandos se escriben dentro del cliente (no en el servidor).

1. Ver usuarios conectados:

```text
LIST
```

Respuesta típica:

```text
[Servidor Info]:
alice,bob
```

2. Ver información de un usuario:

```text
INFO bob
```

Respuesta típica:

```text
[Servidor Info]:
bob,127.0.0.1,ACTIVO
```

3. Cambiar tu estado:

```text
STATUS OCUPADO
```

Estados válidos: `ACTIVO`, `OCUPADO`, `INACTIVO`.

4. Mensaje directo a un usuario:

```text
MSG bob hola bob, estas ahi?
```

En el cliente de bob aparece algo como:

```text
[DM de alice]: hola bob, estas ahi?
```

5. Mensaje para todos (broadcast):

```text
BROADCAST hola a todos
```

En los demás clientes aparece:

```text
[General - alice]: hola a todos
```

6. Salir del chat:

```text
EXIT
```

### 8. Flujo recomendado para una demo rápida

1. Arranca servidor en puerto `8080`.
2. Conecta `alice` y `bob`.
3. Desde `alice`, ejecuta `LIST`.
4. Desde `alice`, ejecuta `MSG bob hola bob`.
5. Desde `bob`, ejecuta `BROADCAST hola alice`.
6. Desde `alice`, ejecuta `INFO bob`.
7. Ambos ejecutan `EXIT`.

### 9. Cerrar todo correctamente

- Clientes: escribir `EXIT`.
- Servidor: `Ctrl + C` en su terminal.

### 10. Problemas comunes

1. `Address already in use` al iniciar servidor.

Solución: ya hay un proceso usando ese puerto. Cierra el proceso o usa otro puerto (por ejemplo `8081`).

2. `Error en registro: ip duplicada`.

Solución: ocurre en modo `production` cuando dos clientes vienen de la misma IP. Para pruebas locales usa `CHAT_ENV=testing`.

3. Cliente no conecta al servidor.

Solución: verifica que IP, puerto y servidor en ejecución coincidan.

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

| Criterio                                  | Evidencia de implementación                                                                                                     | Evidencia de validación                                                 |
| ----------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------ |
| Servidor concurrente multicliente         | `src/server.c` (accept loop + `pthread_create` + `pthread_detach`)                                                         | caso `estres basico concurrente`                                       |
| Registro y unicidad de usuarios           | `src/server_handlers.c` (`REGISTER`), `src/server_registry.c` (`registry_add_user`)                                      | casos `production bloquea ip duplicada`, `testing permite misma ip`  |
| Consulta de usuarios conectados           | `src/server_handlers.c` (`LIST_REQ`), `src/server_registry.c` (`registry_build_user_list`)                               | casos `flujo core con timeout inactivo`, `exit limpia sesion`        |
| Consulta de información de usuario       | `src/server_handlers.c` (`INFO_REQ`)                                                                                         | caso `flujo core con timeout inactivo`                                 |
| Cambio de estado y timeout                | `src/server_handlers.c` (`STATUS`), `src/server_registry.c` (`registry_mark_inactive_if_timeout`)                        | caso `flujo core con timeout inactivo`                                 |
| Mensajería directa y broadcast           | `src/server_handlers.c` (`DM`, `BROADCAST`), `src/server_registry.c` (`registry_send_to_user`, `registry_broadcast`) | casos `flujo core con timeout inactivo`, `estres basico concurrente` |
| Cierre de sesión y limpieza              | `src/server_handlers.c` (`EXIT`), `src/server_registry.c` (`registry_remove_user*`)                                      | caso `exit limpia sesion`                                              |
| Manejo de errores de protocolo/operación | `src/server.c` (parse y descarte), `src/server_handlers.c` (`send_error_response`)                                         | reporte E2E + pruebas manuales de comandos inválidos                    |

## Documentación técnica

- protocolo de mensajes: [docs/protocolo.md](docs/protocolo.md)
- análisis de concurrencia y sincronización: [docs/analisis_concurrencia.md](docs/analisis_concurrencia.md)
