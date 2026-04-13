# Análisis de Concurrencia

## 1. Modelo concurrente implementado

El servidor usa un modelo thread-per-client:

- un hilo principal acepta conexiones TCP (`accept`)
- por cada cliente se crea un hilo dedicado (`pthread_create`)
- cada hilo de cliente procesa solicitudes en bucle hasta `EXIT` o desconexión

Este modelo permite paralelismo real entre múltiples clientes sobre una estructura de datos compartida (registro de usuarios).

## 2. Recurso compartido crítico

El recurso compartido principal es el `UserRegistry`:

- lista enlazada de usuarios conectados
- estado, socket, ip y marca de última actividad

Todas las operaciones sobre esta estructura están protegidas por `pthread_mutex_t` (`registry->mutex`).

## 3. Estrategia de sincronización

Se usa exclusión mutua con un único mutex para serializar cambios y lecturas sensibles:

- altas/bajas de usuarios
- búsquedas por username/socket
- actualización de estado y actividad
- marcaje por timeout a `INACTIVO`
- construcción de listado de usuarios

Puntos clave:

- se evita retener el mutex durante envíos de red largos
- en broadcast se copia primero la lista de sockets y luego se libera el lock
- no se usan locks anidados, reduciendo riesgo de deadlock

## 4. Condiciones de carrera consideradas

### Caso A: alta duplicada simultánea

Riesgo:
- dos clientes intentan registrar mismo username (o IP en modo estricto) casi al mismo tiempo.

Mitigación:
- `registry_add_user` ejecuta validación e inserción bajo el mismo mutex.
- esto vuelve atómica la decisión de unicidad + inserción.

### Caso B: envío a usuario mientras otro hilo lo elimina

Riesgo:
- hilo 1 quiere enviar DM a `bob` mientras hilo 2 procesa `EXIT` de `bob`.

Mitigación:
- `registry_send_to_user` obtiene el socket objetivo bajo lock.
- si el usuario no existe en el momento de lookup retorna error controlado.
- si desaparece inmediatamente después, el `send` falla y se reporta error al emisor.

### Caso C: timeout concurrente con actualización de estado

Riesgo:
- un hilo marca `INACTIVO` por timeout mientras otro hilo procesa `STATUS`/actividad del mismo usuario.

Mitigación:
- ambas rutas usan el mismo mutex (`registry_mark_inactive_if_timeout`, `registry_update_status`, `registry_touch_activity`).
- se garantiza orden total entre transiciones de estado.

## 5. Riesgos residuales y trade-offs

- un único mutex simplifica consistencia pero reduce escalabilidad bajo alta carga.
- no hay cola de eventos ni pool de hilos; para cargas muy altas se podría migrar a un modelo con worker pool o IO multiplexada.
- el lock global puede aumentar latencia en picos de broadcast con muchos clientes.

## 6. Validación práctica realizada

Se validó comportamiento concurrente con pruebas E2E automatizadas:

- bloqueo de IP duplicada en `production`
- permiso de misma IP en `testing`
- timeout a `INACTIVO`
- limpieza de sesión en `EXIT`
- estrés básico con múltiples emisores concurrentes

Referencia de ejecución: [scripts/e2e_wsl.sh](../scripts/e2e_wsl.sh) y [e2e_report.txt](../e2e_report.txt).

## 7. Conclusión

La implementación actual mantiene consistencia funcional para el alcance del proyecto, controla condiciones de carrera principales y ofrece comportamiento determinista en pruebas concurrentes de integración.