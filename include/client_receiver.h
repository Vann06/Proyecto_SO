#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

// Función principal del hilo receptor.
// El argumento debe ser un puntero a ClientContext.
void *receiver_thread_func(void *arg);

#endif // CLIENT_RECEIVER_H
