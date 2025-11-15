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
#include <pthread.h>
#include <sys/time.h>

#include "../include/server_log.h"
#include "../include/sdmessage.pb-c.h"
#include "../include/list_skel.h"
#include "../include/message-private.h"

#define MAX_CONNECTIONS 5

static volatile int server_shutdown_requested = 0; // Flag para término do servidor
static int g_listen_fd = -1;   // Socket de listening global
static int g_conn_fd = -1;      // Socket de conexão atual global
static int active_connections = 0; // Contador de conexões ativas
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para sincronização da lista
static struct thread_node *active_threads = NULL; // Cabeça da lista ligada de threads ativas
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger acessos à lista compartilhada

// argumentos passados a cada thread cliente
struct client_handler_args {
    int client_socket;
    struct list_t *list;
    char *client_addr_port;  // client addr:port
};

static void *handle_client(void *arg);

// estrutura para lista de threads
struct thread_node {
    pthread_t thread;
    struct thread_node *next;
};

// adicionar thread à lista
void add_active_thread(pthread_t t) {
    struct thread_node *new_node = malloc(sizeof(struct thread_node));
    if (!new_node) {
        perror("malloc");
        return;
    }
    new_node->thread = t;
    new_node->next = active_threads;
    active_threads = new_node;
}

// remover thread da lista
void remove_active_thread(pthread_t t) {
    struct thread_node *current = active_threads;
    struct thread_node *prev = NULL;
    while (current) {
        if (pthread_equal(current->thread, t)) {
            if (prev) {
                prev->next = current->next;
            } else {
                active_threads = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

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
    if (listen(listening_socket, MAX_CONNECTIONS) < 0) {
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

char *get_client_addr_port(struct sockaddr_in *client_addr) {
    static char addr_port[INET_ADDRSTRLEN + 6]; // espaço para IP + ':' + porto + '\0'
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, ip_str, sizeof(ip_str));
    uint16_t port = ntohs(client_addr->sin_port);
    snprintf(addr_port, sizeof(addr_port), "%s:%d", ip_str, port);
    return addr_port;
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
        
        pthread_mutex_lock(&thread_mutex);
        if (active_connections >= MAX_CONNECTIONS) {
            // send OP_BUSY
            MessageT *busy_msg = calloc(1, sizeof(MessageT));
            message_t__init(busy_msg);
            busy_msg->opcode = MESSAGE_T__OPCODE__OP_BUSY;
            busy_msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            network_send(client_sock, busy_msg);
            message_t__free_unpacked(busy_msg, NULL);
            close(client_sock);
            pthread_mutex_unlock(&thread_mutex);
            continue;
        }

        // send OP_READY
        MessageT *ready_msg = calloc(1, sizeof(MessageT));
        message_t__init(ready_msg);
        ready_msg->opcode = MESSAGE_T__OPCODE__OP_READY;
        ready_msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        network_send(client_sock, ready_msg);
        message_t__free_unpacked(ready_msg, NULL);

        struct client_handler_args *handler_args = malloc(sizeof(*handler_args));
        if (!handler_args) {
            perror("malloc");
            close(client_sock);
            pthread_mutex_unlock(&thread_mutex);
            continue;
        }
        handler_args->client_socket = client_sock;
        handler_args->list = list;
        // get client address and port as string
        char *client_addr_port = get_client_addr_port(&client_addr);
        handler_args->client_addr_port = strdup(client_addr_port);

        pthread_t thread;
        int seconds = get_seconds();
        if (pthread_create(&thread, NULL, handle_client, handler_args) != 0) {
            perror("pthread_create");
            free(handler_args);
            close(client_sock);
            pthread_mutex_unlock(&thread_mutex);
            continue;
        }
        add_active_thread(thread);
        active_connections++;
        
        // log de conexão
        write_log(seconds, client_addr_port, "CONNECT", (MessageT__Opcode)0, (MessageT__CType)0, NULL);

        printf("Utilizador conectado, conexões ativas: %d\n", active_connections);
        
        pthread_mutex_unlock(&thread_mutex);
        
        g_conn_fd = client_sock; // Guardar socket do cliente globalmente
    }

    return 0;
}



void *handle_client(void *arg) {
    struct client_handler_args *handler_args = arg;
    int client_socket = handler_args->client_socket;
    struct list_t *list = handler_args->list;
    char *client_addr_port = handler_args->client_addr_port;
    
    free(handler_args);
    
    MessageT *req = NULL;
    // enquanto o server não for desligado
    while (!server_shutdown_requested && (req = network_receive(client_socket))!= NULL) {
        
        // proteger acessos à lista com mutex
        pthread_mutex_lock(&list_mutex);
        if (invoke(req, list) < 0 || network_send(client_socket, req) < 0) {
            req->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            req->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            message_t__free_unpacked(req, NULL);
            pthread_mutex_unlock(&list_mutex);
            continue;
        }
        pthread_mutex_unlock(&list_mutex);

        // log da operação
        write_log(get_seconds(), client_addr_port, "REQUEST", req->opcode, req->c_type, req->data);

        message_t__free_unpacked(req, NULL);
    }
    int seconds = get_seconds();

    // log de desconexão
    write_log(seconds, client_addr_port, "CLOSE", (MessageT__Opcode)0, (MessageT__CType)0, NULL);
    free(client_addr_port);

    g_conn_fd = -1; // limpar socket do cliente
        
    close(client_socket);
    
    // Proteger o decremento com mutex
    pthread_mutex_lock(&thread_mutex);
    remove_active_thread(pthread_self());  // Remover da lista
    active_connections--;  // Decrementar contador
    printf("Utilizador desconectado, conexões ativas: %d\n", active_connections);
    pthread_mutex_unlock(&thread_mutex);

    pthread_exit(NULL);
    return NULL;
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