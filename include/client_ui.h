#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include "client.h"

// inicia la consola interactiva para que el usuario escriba comandos
// soporta LIST, INFO, STATUS, MSG, BROADCAST, EXIT
void start_ui(ClientContext *ctx);

#endif // CLIENT_UI_H
