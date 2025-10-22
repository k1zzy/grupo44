#include "network_client.h"
#include "client_stub-private.h"
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
    memset(&serv_addr, 0, sizeof(serv_addr)); // limpar a estrutura, so por precaução
    serv_addr.sin_family = AF_INET; // dizer que é IPv4
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

    return 0; // sucesso
}
// TODO placeholder do copilot, nao sei sequer se está perto de estar bem
MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg) {
    if (!rlist || !msg) {
        return NULL; // rlist ou msg inválidos
    }

    // Serializar a mensagem
    size_t msg_size = message_t__get_packed_size(msg);
    uint8_t *buffer = malloc(msg_size);
    if (!buffer) {
        return NULL; // erro ao alocar memória
    }
    message_t__pack(msg, buffer);

    // Enviar a mensagem serializada para o servidor
    ssize_t bytes_sent = send(rlist->sockfd, buffer, msg_size, 0);
    free(buffer); // libertar o buffer após o envio
    if (bytes_sent != msg_size) {
        return NULL; // erro ao enviar a mensagem
    }

    // Esperar a resposta do servidor
    uint8_t response_buffer[4096]; // buffer para a resposta
    ssize_t bytes_received = recv(rlist->sockfd, response_buffer, sizeof(response_buffer), 0);
    if (bytes_received <= 0) {
        return NULL; // erro ao receber a resposta
    }

    // De-serializar a mensagem de resposta
    MessageT *response_msg = message_t__unpack(NULL, bytes_received, response_buffer);
    if (!response_msg) {
        return NULL; // erro ao de-serializar a mensagem
    }

    return response_msg; // retornar a mensagem de-serializada
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