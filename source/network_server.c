/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */

#include "../include/network_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>

#include "../include/sdmessage.pb-c.h"
#include "../include/list_skel.h"
#include "../include/message-private.h"


static volatile int server_shutdown_requested = 0; // Flag para término do servidor
static int g_listen_fd = -1;   // Socket de listening global
static int g_conn_fd = -1;      // Socket de conexão atual global

int network_server_init(short port) {
    // criar o socket TCP
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0) {
        return -1;
    }

    // utiliza SO_REUSEADDR como pedido
    int opt = 1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error in setsockopt");
        close(listening_socket);
        return -1;
    }

    // endereço do servidor onde a socket irá ouvir
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // ipv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // associar a socket ao porto
    if (bind(listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in bind");
        close(listening_socket);
        return -1;
    }

    // colocar a socket em modo de escuta
    if (listen(listening_socket, 0) < 0) { // TODO backlog = 0 é muito pequeno
        perror("Error in listen");
        close(listening_socket);
        return -1;
    }

    g_listen_fd = listening_socket; 
    printf("Server listening on port %d\n", port);
    return listening_socket;
}

MessageT *network_receive(int client_socket) {
    // ler (2 bytes) qual é o tamanho da mensagem (uint16_t)
    uint16_t netlen;
    if (read_all(client_socket, &netlen, 2) == -1) {
        return NULL; /* erro ou cliente fechou */
    }

    // converter para host order
    uint16_t len = ntohs(netlen);
    
    // agora que ja sabemos o tamanho, alocar o buffer 
    uint8_t *buf = malloc(len);
    if (!buf) return NULL;

    // lê a mensagem
    if (read_all(client_socket, buf, (int)len) == -1) {
        free(buf);
        return NULL;
    }

    // des-serializar a mensagem
    MessageT *msg = message_t__unpack(NULL, len, buf);
    free(buf);
    return msg; 
}

int network_send(int client_socket, MessageT *msg) {
    if (!msg) { 
        return -1;
    }
    
    size_t packed = message_t__get_packed_size(msg); // tamanho da mensagem seriazliada
    if (packed > UINT16_MAX) {
        return -1; // não pode ultrapassar o 65535 bytes de tamanho
    }

    uint8_t *buf = malloc(packed); // aloca memória para a mensagem serializada
    if (!buf) {
        return -1;
    }
    size_t written = message_t__pack(msg, buf); // buffer da mensagem serializada

    uint16_t netlen = htons((uint16_t)written); // converte o tamanho para network order
    
    // envia o tamanho da mensagem
    if (write_all(client_socket, &netlen, sizeof(netlen)) != (int)sizeof(netlen)) {
        free(buf);
        return -1;
    }
    // envia a mensagem
    if (write_all(client_socket, buf, (int)written) != (int)written) {
        free(buf);
        return -1;
    }

    free(buf);
    return 0;
}

int network_main_loop(int listening_socket, struct list_t *list) {
    if (listening_socket < 0) {
        return -1;
    }

    while (!server_shutdown_requested) {
        struct sockaddr_in client_addr; // estrutura onde o endereço do socket vai ser armazenado
        socklen_t addrlen = sizeof(client_addr); // tamanho da estrutura do endereço
        int client_sock = accept(listening_socket, (struct sockaddr *)&client_addr, &addrlen); // endereço do socket do cliente
        // caso haja um erro com a nova ligação ao socket
        if (client_sock < 0) {
            if (server_shutdown_requested) {
                break;
            }
            perror("accept");
            continue;
        }

        printf("Utilizador conectado\n");
        
        g_conn_fd = client_sock; // Guardar socket do cliente globalmente

        // enquanto o server não for desligado
        while (!server_shutdown_requested) {
            MessageT *req = network_receive(client_sock); // receber o pedido do client
            if (!req) {
                break; // client fechou ou erro
            }

            //TODO
            /* invoke processa a mesma MessageT e preenche o resultado */
            if (invoke(req, list) < 0) {
                // req->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
                // req->result = -1; TODO
            }
            

            if (network_send(client_sock, req) != 0) {
                message_t__free_unpacked(req, NULL); 
                break;
            }

            message_t__free_unpacked(req, NULL);
        }

        close(client_sock);
        printf("Utilizador desconectado\n");
        g_conn_fd = -1; // Limpar socket do cliente
    }

    return 0;
}

int network_server_close(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd); // fecha o socket
        return 0;
    }
    return -1;
}

void network_server_request_shutdown(void) {
    // Sinalizar término
    server_shutdown_requested = 1;
    
    // Fechar socket de conexão atual (se existir)
    if (g_conn_fd >= 0) {
        close(g_conn_fd);
        g_conn_fd = -1;
    }
    
    // Fechar socket de listening (desbloqueia accept())
    if (g_listen_fd >= 0) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }
}