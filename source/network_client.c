/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */

#include "../include/client_stub-private.h"
#include "../include/network_client.h"
#include "../include/message-private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int network_connect(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist invalida
    }

    // cria socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1; // erro a criar socket
    }

    // configura endereco server
    struct sockaddr_in serv_addr; // estrutura endereco server
    memset(&serv_addr, 0, sizeof(serv_addr)); // limpa struct
    serv_addr.sin_family = AF_INET; // define ipv4
    serv_addr.sin_port = htons(rlist->server_port); // converte porta network order
    if (inet_pton(AF_INET, rlist->server_address, &serv_addr.sin_addr) <= 0) { // converte ip
        close(sockfd);
        return -1; // erro a converter ip
    }

    // liga ao server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1; // erro a conectar
    }

    rlist->sockfd = sockfd; // guarda socket rlist

    return 0; 
}


MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg) {
    if (!rlist || !msg) {
        return NULL; // rlist ou msg invalidos
    }

    // obtem socket rlist
    int socket = rlist->sockfd;

    size_t msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size); // aloca buffer

    if (!buffer) {
        return NULL; // erro a alocar memoria
    }
    // serializa msg
    message_t__pack(msg, buffer);

    // envia tamanho msg serializada
    uint16_t network_msg_size = htons((uint16_t)msg_size);
    if (write_all(socket, &network_msg_size, sizeof(network_msg_size)) == -1) {
        free(buffer);
        return NULL;
    }


    // envia msg serializada
    if (write_all(socket, buffer, msg_size) == -1) {
        free(buffer);
        return NULL;
    }
    free(buffer); // liberta buffer

    // recebe tamanho resposta
    uint16_t response_size_network;
    if (read_all(socket, &response_size_network, sizeof(response_size_network)) == -1) {
        return NULL;
    }
    uint16_t response_size = ntohs(response_size_network);

    // aloca memoria resposta
    uint8_t *response_buffer = malloc(response_size);
    if (!response_buffer) {
        return NULL;
    }
    // recebe resposta
    if (read_all(socket, response_buffer, response_size) == -1) {
        free(response_buffer); // erro a ler liberta buffer
        return NULL;
    }

    MessageT *response_msg = message_t__unpack(NULL, response_size, response_buffer);
    free(response_buffer); // liberta buffer resposta

    return response_msg; // devolve msg deserializada

}

int network_close(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist invalida
    }

    // fecha socket
    if (rlist->sockfd != -1) {
        close(rlist->sockfd);
        rlist->sockfd = -1; // marca fechada
    }

    return 0;
}