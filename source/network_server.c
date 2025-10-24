#include "network_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>

#include "../include/sdmessage.pb-c.h"
#include "../include/list_skel.h"
#include "../include/message-private.h" /* declara read_all / write_all */

static volatile int server_shutdown_requested = 0;

int network_server_init(short port) {
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0) {
        perror("socket");
        return -1;
    }

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

/* Lê uma MessageT do cliente usando o protocolo: uint16_t (network order) + payload protobuf */
MessageT *network_receive(int client_socket) {
    uint16_t netlen;
    if (read_all(client_socket, &netlen, sizeof(netlen)) != (int)sizeof(netlen)) {
        return NULL; /* erro ou cliente fechou */
    }
    uint16_t len = ntohs(netlen);
    if (len == 0) return NULL;

    uint8_t *buf = malloc(len);
    if (!buf) return NULL;

    if (read_all(client_socket, buf, (int)len) != (int)len) {
        free(buf);
        return NULL;
    }

    MessageT *msg = message_t__unpack(NULL, len, buf);
    free(buf);
    return msg; /* pode ser NULL se unpack falhar */
}

/* Envia uma MessageT para o cliente com o mesmo protocolo (uint16_t + payload) */
int network_send(int client_socket, MessageT *msg) {
    if (!msg) return -1;

    size_t packed = message_t__get_packed_size(msg);
    if (packed > UINT16_MAX) return -1;

    uint8_t *buf = malloc(packed);
    if (!buf) return -1;
    size_t written = message_t__pack(msg, buf);

    uint16_t netlen = htons((uint16_t)written);
    if (write_all(client_socket, &netlen, sizeof(netlen)) != (int)sizeof(netlen)) {
        free(buf);
        return -1;
    }
    if (write_all(client_socket, buf, (int)written) != (int)written) {
        free(buf);
        return -1;
    }

    free(buf);
    return 0;
}

int network_main_loop(int listening_socket, struct list_t *list) {
    if (listening_socket < 0) return -1;

    while (!server_shutdown_requested) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(listening_socket, (struct sockaddr *)&client_addr, &addrlen);
        if (client_sock < 0) {
            if (server_shutdown_requested) break;
            perror("accept");
            continue;
        }

        while (!server_shutdown_requested) {
            MessageT *req = network_receive(client_sock);
            if (!req) break; /* cliente fechou ou erro */

            /* invoke processa a mesma MessageT e preenche o resultado */
            if (invoke(req, list) < 0) {
                req->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
                req->result = -1;
            }

            if (network_send(client_sock, req) != 0) {
                message_t__free_unpacked(req, NULL);
                break;
            }

            message_t__free_unpacked(req, NULL);
        }

        close(client_sock);
    }

    return 0;
}

int network_server_close(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
        return 0;
    }
    return -1;
}

void network_server_request_shutdown(void) {
    server_shutdown_requested = 1;
}