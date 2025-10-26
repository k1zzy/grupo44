
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
        return -1; // rlist inválida
    }

    // criar o socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1; // erro ao criar o socket
    }

    // configurar o endereço do servidor
    struct sockaddr_in serv_addr; // estrutura para o endereço do servidor
    memset(&serv_addr, 0, sizeof(serv_addr)); // limpar a estrutura por precaução
    serv_addr.sin_family = AF_INET; // definir como ipv4
    serv_addr.sin_port = htons(rlist->server_port); // converter a porta para network byte order
    if (inet_pton(AF_INET, rlist->server_address, &serv_addr.sin_addr) <= 0) { // converter o IP
        close(sockfd);
        return -1; // erro ao converter o IP
    }

    // estabelecer a ligação com o servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1; // erro ao conectar ao servidor
    }

    // guardar a socket na estrutura rlist
    rlist->sockfd = sockfd;

    return 0; 
}


MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg) {
    if (!rlist || !msg) {
        return NULL; // rlist ou msg inválidos
    }

    // Obter o socket da rlist
    int socket = rlist->sockfd;

    size_t msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size); // 16 para o tamanho do short

    if (!buffer) {
        return NULL; // erro ao alocar memória
    }
    // serializar a mensagem
    message_t__pack(msg, buffer);

    // enviar o tamanho da mensagem serializada ao server
    uint16_t network_msg_size = htons((uint16_t)msg_size);
    if (write_all(socket, &network_msg_size, sizeof(network_msg_size)) == -1) {
        free(buffer);
        return NULL;
    }

    // enviar a mensagem serializada ao servidor
    if( write_all(socket, buffer, msg_size) == -1){
        free(buffer);
        return NULL;
    }
    free(buffer); // libertar a memoria antes de enviar

    printf("Server waiting for the response\n");

    // receber o tamamho da resposta do servidor
    uint16_t response_size_network;
    if (read_all(socket, &response_size_network, sizeof(response_size_network)) == -1) {
        return NULL;
    }
    uint16_t response_size = ntohs(response_size_network);

    // alocar memória para o tamamnho da resposta
    uint8_t *response_buffer = malloc(response_size);
    if (!response_buffer) {
        return NULL;
    }
    // receber a resposta do servidor
    if( read_all(socket, response_buffer, response_size) == -1){
        free(response_buffer); // se houver erro a ler a mensagem, liberta o buffer
        return NULL; 
    }

    MessageT *response_msg = message_t__unpack(NULL, response_size, response_buffer); 
    free(response_buffer); // liberta o buffer da respostas após a de-serialização

    return response_msg; // mensagem de-serializada

}

int network_close(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist inválida
    }

    // fechar a socket
    if (rlist->sockfd != -1) {
        close(rlist->sockfd);
        rlist->sockfd = -1; // marcar como fechada
    }

    return 0;
}