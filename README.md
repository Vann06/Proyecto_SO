# Chat Multicliente en C con Sockets TCP

Proyecto de chat cliente-servidor implementado en **C**, utilizando **sockets TCP** y concurrencia para permitir la comunicación entre múltiples clientes a través de un servidor central.

## Descripción general

Este proyecto implementa un sistema de chat donde:

- un **servidor** acepta múltiples conexiones de clientes
- cada **cliente** puede registrarse con un nombre único
- los usuarios pueden consultar quiénes están conectados
- los usuarios pueden cambiar su estado
- los usuarios pueden enviar mensajes directos
- los usuarios pueden enviar mensajes al chat general
- toda la comunicación se realiza mediante un **protocolo propio**

El servidor es el punto central de comunicación. Los clientes no se comunican directamente entre sí; todos los mensajes pasan a través del servidor.

---

## Objetivo del proyecto

Desarrollar un sistema de chat concurrente en C, con una arquitectura clara y modular, capaz de manejar múltiples clientes conectados al mismo tiempo, utilizando un protocolo de comunicación simple, consistente y validable.

---

## Estado actual

Actualmente el proyecto cuenta con la **base del protocolo** y de los módulos comunes.

### Ya definido
- estructura del proyecto
- formato del protocolo
- operaciones válidas
- validación de estados
- serialización y deserialización de mensajes
- checksum básico
- documentación inicial del protocolo
- prueba local del módulo de protocolo

### Pendiente de implementar o integrar
- servidor multicliente completo
- registro real de usuarios conectados
- handlers de operaciones del servidor
- cliente interactivo
- recepción concurrente de mensajes
- integración completa entre módulos

---

## Arquitectura del proyecto

```text
chat/
├── Makefile
├── README.md
├── include/
│   ├── common.h
│   ├── protocol.h
│   ├── utils.h
│   ├── server.h
│   ├── client.h
│   └── user.h
├── src/
│   ├── protocol.c
│   ├── utils.c
│   ├── main_server.c
│   ├── main_client.c
│   ├── server.c
│   ├── server_handlers.c
│   ├── server_registry.c
│   ├── client.c
│   ├── client_receiver.c
│   └── client_ui.c
├── tests/
│   └── test_protocol.c
└── docs/
    ├── protocolo.md
    └── analisis_concurrencia.md
```
