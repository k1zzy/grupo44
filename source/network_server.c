#include "network_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "network_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// TODO prob mal
int network_server_init(short port) {
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0) {
        perror("socket");
        return -1;
    }
    
    // Permitir reutilização do endereço/porta
    int opt = 1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listening_socket);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listening_socket);
        return -1;
    }
    
    if (listen(listening_socket, 5) < 0) {
        perror("listen");
        close(listening_socket);
        return -1;
    }

    return listening_socket;
}

int network_main_loop(int listening_socket, struct list_t *list) {
    // Implementação da função network_main_loop
    return -1; // Placeholder
}

MessageT *network_receive(int client_socket) {
    // Implementação da função network_receive
    return NULL; // Placeholder
}

int network_send(int client_socket, MessageT *msg) {
    // Implementação da função network_send
    return -1; // Placeholder
}

int network_server_close(int socket) {
    // Implementação da função network_server_close
    return -1; // Placeholder
}

void network_server_request_shutdown(void) {
    // Implementação da função network_server_request_shutdown
}