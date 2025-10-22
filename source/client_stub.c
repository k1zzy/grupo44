#include "client_stub.h"
#include "client_stub-private.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// TODO nao sei se a socket deve ser criada aqui ou noutro sitio
struct rlist_t *rlist_connect(char *address_port) {
    struct rlist_t *rlist = NULL;
    rlist = malloc(sizeof(struct rlist_t));
    if (!rlist) {
        return NULL; // se falhou a alocação de memória
    }
    char *pontos_ptr = strchr(address_port, ':'); // procura o ':'
    if (!pontos_ptr) {
        free(rlist);
        return NULL; // se nao encontrou o ':', formato inválido
    }
    size_t ip = pontos_ptr - address_port; // um ptr menos o outro dá o tamanho do server address
    // +1 para o '\0', temos de fazer assim porque nao sabemos o tamanho do server address
    rlist->server_address = malloc(ip + 1);
    if (!rlist->server_address) {
        free(rlist); // se falhou a alocação de memória
        return NULL;
    }
    strncpy(rlist->server_address, address_port, ip); // por fim, copia o server address
    rlist->server_address[ip] = '\0'; // adiciona o terminador de string na ultima posição
    // podemos utilizar atoi porque este para quando encontrar um caracter que nao seja número
    // neste caso, o terminator de string '\0'
    rlist->server_port = atoi(pontos_ptr + 1); // converte a parte do port para int
    rlist->sockfd = -1; // placeholder, socket ainda nao criada
    return rlist;
}

// TODO nao sei se esta bem, depende de onde e como a socket será criada
int rlist_disconnect(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist inválida
    }
    // fechar a socket se estiver aberta
    if (rlist->sockfd != -1) {
        // fechar a socket
        close(rlist->sockfd);
    }
    free(rlist->server_address); // libertar o server address
    free(rlist); // libertar a estrutura rlist
    return 0;
}

int rlist_add(struct rlist_t *rlist, struct data_t *car) {
    if (!rlist || !car) {
        return -1; // rlist ou car inválidos
    }
    // TODO implementar a adição de dados à lista
    return 0;
}

int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo) {
    if (!rlist || !modelo) {
        return -1; // rlist ou modelo inválidos
    }
    // TODO implementar a remoção de dados da lista por modelo
    return 1; // placeholder: não encontrado
}

struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca) {
    if (!rlist) {
        return NULL; // rlist inválida
    }
    // TODO implementar a obtenção de dados da lista por marca
    return NULL; // placeholder: não encontrado
}

struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano) {
    if (!rlist) {
        return NULL; // rlist inválida
    }
    // TODO implementar a obtenção de dados da lista por ano
    return NULL; // placeholder: não encontrado
}

int rlist_order_by_year(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist inválida
    }
    // TODO implementar a ordenação da lista por ano
    return 0;
}

int rlist_size(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist inválida
    }
    // TODO implementar a obtenção do tamanho da lista
    return 0; // placeholder: tamanho 0
}

char **rlist_get_model_list(struct rlist_t *rlist) {
    if (!rlist) {
        return NULL; // rlist inválida
    }
    // TODO implementar a obtenção da lista de modelos
    return NULL; // placeholder: não encontrado
}

int rlist_free_model_list(char **models) {
    if (!models) {
        return -1; // models inválido
    }
    // TODO implementar a libertação da lista de modelos
    return 0;
}