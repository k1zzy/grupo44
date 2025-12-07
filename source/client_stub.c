/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */
#include "../include/client_stub.h"
#include "../include/client_stub-private.h"
#include "../include/network_client.h"
#include "../include/message-private.h"
#include "../include/server_log.h"
#include "sdmessage.pb-c.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

static void log_event(struct rlist_t *rlist, const char *eventType, MessageT__Opcode opcode, MessageT__CType c_type, Data *argument) {
    if (!rlist || rlist->sockfd < 0) {
        return;
    }

    char *client_addr_port = make_client_addr_port(rlist->sockfd);
    if (!client_addr_port) {
        return;
    }

    write_log(get_seconds(), client_addr_port, eventType, opcode, c_type, argument);
    free(client_addr_port);
}

struct rlist_t *rlist_connect(char *address_port) {
    struct rlist_t *rlist = NULL;
    rlist = malloc(sizeof(struct rlist_t));
    if (!rlist) {
        return NULL; // erro a alocar memoria
    }
    char *pontos_ptr = strchr(address_port, ':'); // procura o :
    if (!pontos_ptr) {
        free(rlist);
        return NULL; // sem :, formato invalido
    }
    size_t ip = pontos_ptr - address_port; // ptr menos outro da tamanho server
    // +1 pro \0, nao sabemos tamanho
    rlist->server_address = malloc(ip + 1);
    if (!rlist->server_address) {
        free(rlist); // erro a alocar memoria
        return NULL;
    }
    strncpy(rlist->server_address, address_port, ip); // copia server address
    rlist->server_address[ip] = '\0'; // mete terminador string no fim
    // atoi para num char nao numero \0
    // neste caso, o terminator de string '\0'
    rlist->server_port = atoi(pontos_ptr + 1); // converte porto pa int
    // liga ao server
    if (network_connect(rlist) < 0) { 
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    // depois de ligar espera status server
    uint16_t status_size_network;
    if (read_all(rlist->sockfd, &status_size_network, sizeof(status_size_network)) == -1) {
        network_close(rlist);
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    uint16_t status_size = ntohs(status_size_network);
    uint8_t *status_buffer = malloc(status_size);
    if (!status_buffer) {
        network_close(rlist);
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    if (read_all(rlist->sockfd, status_buffer, status_size) == -1) {
        free(status_buffer);
        network_close(rlist);
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    MessageT *status = message_t__unpack(NULL, status_size, status_buffer);
    free(status_buffer);
    if (!status) {
        network_close(rlist);
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    if (status->opcode == MESSAGE_T__OPCODE__OP_BUSY) {
        message_t__free_unpacked(status, NULL);
        network_close(rlist);
        free(rlist->server_address);
        free(rlist);
        return NULL;
    } else if (status->opcode != MESSAGE_T__OPCODE__OP_READY) {
        message_t__free_unpacked(status, NULL);
        network_close(rlist);
        free(rlist->server_address);
        free(rlist);
        return NULL;
    }
    message_t__free_unpacked(status, NULL);

    log_event(rlist, "CONNECT", (MessageT__Opcode)0, (MessageT__CType)0, NULL);

    return rlist;
}

int rlist_disconnect(struct rlist_t *rlist) {
    if (!rlist) {
        return -1; // rlist invalida
    }

    log_event(rlist, "CLOSE", (MessageT__Opcode)0, (MessageT__CType)0, NULL);

    // fecha socket se aberta
    if (rlist->sockfd != -1) {
        // fecha socket
        close(rlist->sockfd);
    }

    free(rlist->server_address); // liberta server address
    free(rlist); // liberta estrutura
    return 0;
}
// adiciona carro
int rlist_add(struct rlist_t *rlist, struct data_t *car) {
    if (!rlist || !car) {
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;

    Data *pd = malloc(sizeof(Data)); 

    if (!pd) { // erro a alocar memoria
        return -1;
    }

    // preenche struct data com dados
    data__init(pd);
    pd->ano = car->ano;
    pd->preco = car->preco;
    pd->marca = (Marca)car->marca;
    pd->modelo = car->modelo ? strdup(car->modelo) : NULL;
    pd->combustivel = (Combustivel)car->combustivel;

    msg.data = pd;

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

    MessageT *resp = network_send_receive(rlist, &msg); // envia msg e recebe resposta

    if (pd->modelo) {
        free(pd->modelo);
    }

    free(pd);

    if (!resp) {
        return -1;
    }

    // verifica se deu (opcode 11)
    int result = (resp->opcode == MESSAGE_T__OPCODE__OP_ADD + 1) ? 0 : -1;
    message_t__free_unpacked(resp, NULL);
    return result;
}
// remove carro modelo
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

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

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
// obtem carro marca
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca) {
    if (!rlist) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
    msg.result = (int32_t)marca;

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

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
// obtem carros ano
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano) {
    if (!rlist) return NULL;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg.result = (int32_t)ano;

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

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
// ordena lista ano
int rlist_order_by_year(struct rlist_t *rlist) {
    if (!rlist) return -1;

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) return -1;
    int res = (resp->c_type == MESSAGE_T__C_TYPE__CT_RESULT) ? resp->result : -1;
    message_t__free_unpacked(resp, NULL);
    return (res == 0) ? 0 : -1;
}
// obtem tamanho lista
int rlist_size(struct rlist_t *rlist) {
    if (!rlist) {
        return -1;
    }

    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

    MessageT *resp = network_send_receive(rlist, &msg);
    if (!resp) {
        return -1;
    }
    
    int size = (resp->c_type == MESSAGE_T__C_TYPE__CT_RESULT) ? resp->result : -1;
    message_t__free_unpacked(resp, NULL);
        
    return size;
}
// obtem lista modelos
char **rlist_get_model_list(struct rlist_t *rlist) {
    if (!rlist) {
        return NULL;
    }
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    log_event(rlist, "REQUEST", msg.opcode, msg.c_type, msg.data);

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
// liberta lista modelos
int rlist_free_model_list(char **models) {
    if (!models) {
        return -1; // models invalido
    }
    for (size_t i = 0; models[i] != NULL; ++i) {
        free(models[i]); // liberta cada string
    }
    free(models); // liberta lista
    return 0;
}