#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H

// hilo que recibe mensajes del servidor en forma asincrona
// mientras el hilo principal espera input de consola
// el argumento debe ser un puntero a ClientContext
void *receiver_thread_func(void *arg);

#endif // CLIENT_RECEIVER_H
