#include "../include/client_stub.h"
#include "../include/client_stub-private.h"
#include "../include/network_client.h"
#include "sdmessage.pb-c.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    // estabelece ligação com o servidor (dando um socket à rlist)
    if (network_connect(rlist) < 0) { 
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    return rlist;
}

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
// adicionar carro
int rlist_add(struct rlist_t *rlist, struct data_t *car) {
    if (!rlist || !car) {
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;

    Data *pd = malloc(sizeof(Data)); 

    if (!pd) { // se falhou a alocação de memória
        return -1;
    }

    // preencher a estrutura Data com os dados do carro
    data__init(pd);
    pd->ano = car->ano;
    pd->preco = car->preco;
    pd->marca = (Marca)car->marca;
    pd->modelo = car->modelo ? strdup(car->modelo) : NULL;
    pd->combustivel = (Combustivel)car->combustivel;

    msg.data = pd;

    MessageT *resp = network_send_receive(rlist, &msg); // enviar a mensagem e receber a resposta

    if (pd->modelo) {
        free(pd->modelo);
    }

    free(pd);

    if (!resp) {
        return -1;
    }

    // Verificar se a operação foi bem-sucedida (opcode = OP_ADD + 1 = 11)
    int result = (resp->opcode == MESSAGE_T__OPCODE__OP_ADD + 1) ? 0 : -1;
    message_t__free_unpacked(resp, NULL);
    return result;
}
// remover carro por modelo
int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo) {
    if (!rlist || !modelo) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MODEL;

    msg.n_models = 1;
    msg.models = malloc(sizeof(char *));
    if (!msg.models) return -1;
    msg.models[0] = strdup(modelo);
    if (!msg.models[0]) {
        free(msg.models);
        return -1;
    }

    MessageT *resp = network_send_receive(rlist, &msg);

    free(msg.models[0]);
    free(msg.models);

    if (!resp) return -1;
    int out = -1;
    if (resp->c_type == MESSAGE_T__C_TYPE__CT_RESULT) out = resp->result;
    message_t__free_unpacked(resp, NULL);
    /* protocolo: 0 = removed, 1 = not found, -1 = error */
    return out;
}
// obter carro por marca
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca) {
    if (!rlist) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
    msg.result = (int32_t)marca;

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) return NULL;

    struct data_t *out = NULL;
    if (resp->c_type == MESSAGE_T__C_TYPE__CT_DATA && resp->data) {
        Data *pd = resp->data;
        const char *modelo = pd->modelo ? pd->modelo : "";
        out = data_create(pd->ano, pd->preco, (enum marca_t)pd->marca, modelo, (enum combustivel_t)pd->combustivel);
    }
    message_t__free_unpacked(resp, NULL);
    return out;
}
// obter carros por ano
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano) {
    if (!rlist) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg.result = (int32_t)ano;

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) return NULL;

    struct data_t **out = NULL;
    if (resp->c_type == MESSAGE_T__C_TYPE__CT_LIST && resp->n_cars > 0 && resp->cars) {
        size_t n = resp->n_cars;
        out = calloc(n + 1, sizeof(struct data_t *));
        if (out) {
            for (size_t i = 0; i < n; ++i) {
                if (resp->cars[i]) {
                    Data *pd = resp->cars[i];
                    const char *modelo = pd->modelo ? pd->modelo : "";
                    out[i] = data_create(pd->ano, pd->preco, (enum marca_t)pd->marca, modelo, (enum combustivel_t)pd->combustivel);
                } else {
                    out[i] = NULL;
                }
            }
            out[n] = NULL;
        }
    }
    message_t__free_unpacked(resp, NULL);
    return out;
}
// ordenar lista por ano
int rlist_order_by_year(struct rlist_t *rlist) {
    if (!rlist) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) return -1;
    int res = (resp->c_type == MESSAGE_T__C_TYPE__CT_RESULT) ? resp->result : -1;
    message_t__free_unpacked(resp, NULL);
    return (res == 0) ? 0 : -1;
}
// obter tamanho da lista
int rlist_size(struct rlist_t *rlist) {
    if (!rlist) {
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) {
        return -1;
    }
    
    int size = (resp->c_type == MESSAGE_T__C_TYPE__CT_RESULT) ? resp->result : -1;
    message_t__free_unpacked(resp, NULL);
        
    return size;
}
// obter lista de modelos
char **rlist_get_model_list(struct rlist_t *rlist) {
    if (!rlist) {
        return NULL;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) return NULL;

    char **out = NULL;
    if (resp->c_type == MESSAGE_T__C_TYPE__CT_MODEL && resp->n_models > 0 && resp->models) {
        size_t n = resp->n_models;
        out = calloc(n + 1, sizeof(char *));
        if (out) {
            for (size_t i = 0; i < n; ++i) {
                out[i] = resp->models[i] ? strdup(resp->models[i]) : strdup("");
            }
            out[n] = NULL;
        }
    }
    message_t__free_unpacked(resp, NULL);
    return out;
}
// libertar lista de modelos
int rlist_free_model_list(char **models) {
    if (!models) {
        return -1; // models inválido
    }
    for (size_t i = 0; models[i] != NULL; ++i) {
        free(models[i]); // libertar cada string individualmente
    }
    free(models); // libertar a lista de strings
    return 0;
}