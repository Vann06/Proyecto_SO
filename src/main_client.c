#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "protocol.h"
#include "client.h"
#include "client_receiver.h"
#include "client_ui.h"

int connect_to_server(const char *ip, int port) {
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error de creacion de socket");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Direccion invalida / no soportada");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Fallo la conexion con el servidor");
        close(sock);
        return -1;
    }

    return sock;
}

int register_user(int socket_fd, const char *username) {
    Message msg;
    char buffer[MAX_SERIALIZED];
    size_t bytes_escritos;

    // Enviar solicitud REGISTER
    if (construir_mensaje(&msg, "SERVER", username, "REGISTER", "") < 0) {
        printf("Error al construir mensaje de registro.\n");
        return -1;
    }

    if (serializar_mensaje(&msg, buffer, sizeof(buffer), &bytes_escritos) < 0) {
        printf("Error al serializar mensaje de registro.\n");
        return -1;
    }

    if (send(socket_fd, buffer, bytes_escritos, 0) < 0) {
        perror("Error al enviar solicitud de registro");
        return -1;
    }

    // Esperar respuesta (INFO_RES = Éxito, ERROR = Fallo)
    int valread = recv(socket_fd, buffer, MAX_SERIALIZED - 1, 0);
    if (valread <= 0) {
        printf("El servidor cerró la conexión durante el registro.\n");
        return -1;
    }
    
    buffer[valread] = '\0';

    Message response;
    if (deserializar_mensaje(buffer, valread, &response) < 0) {
        printf("Error al parsear la respuesta de registro del servidor.\n");
        return -1;
    }

    if (strcmp(response.operacion, "ERROR") == 0) {
        printf("Error en registro: %s\n", response.cuerpo);
        return -1;
    }

    if (strcmp(response.operacion, "INFO_RES") != 0) {
        printf("Respuesta inesperada del servidor durante registro.\n");
        return -1;
    }

    printf("Registro exitoso: %s\n", response.cuerpo);
    return 0;
}

int main(int argc, char const *argv[]) {
    printf("=== CHAT MULTICLIENTE ===\n");

    char ip[16];
    int port = PORT;
    char username[MAX_NAME];

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <usuario> <ip_servidor> <puerto_servidor>\n", argv[0]);
        return 1;
    }

    strncpy(username, argv[1], sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';

    strncpy(ip, argv[2], sizeof(ip) - 1);
    ip[sizeof(ip) - 1] = '\0';

    port = atoi(argv[3]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Puerto invalido: %s\n", argv[3]);
        return 1;
    }

    if (strlen(username) == 0) {
        fprintf(stderr, "Nombre de usuario invalido.\n");
        return 1;
    }

    int socket_fd = connect_to_server(ip, port);
    if (socket_fd < 0) {
        return 1;
    }

    if (register_user(socket_fd, username) < 0) {
        close(socket_fd);
        return 1;
    }

    // Preparar contexto del cliente
    ClientContext ctx;
    ctx.socket_fd = socket_fd;
    strncpy(ctx.username, username, MAX_NAME);
    ctx.username[MAX_NAME - 1] = '\0';
    pthread_mutex_init(&ctx.console_mutex, NULL);
    ctx.running = 1;

    // Iniciar hilo receptor
    pthread_t receiver_thread;
    if (pthread_create(&receiver_thread, NULL, receiver_thread_func, &ctx) != 0) {
        perror("No se pudo crear el hilo receptor");
        close(socket_fd);
        return 1;
    }

    // Iniciar el UI principal en este hilo
    start_ui(&ctx);

    // Cuando start_ui termina (ej. el usuario eligió EXIT)
    ctx.running = 0;
    
    // Cerramos el socket, lo cual destrabará recv() en el hilo receptor 
    // o podemos esperarlo.
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    pthread_join(receiver_thread, NULL);
    
    pthread_mutex_destroy(&ctx.console_mutex);

    printf("Conexión cerrada. Hasta luego!\n");
    return 0;
}