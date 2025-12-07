/**
 * @file list.h
 * @brief Interface para a estrutura da lista de automóveis.
 *
 * define funcoes manipulacao lista ligada carros
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 13/09/2025
 */
#ifndef _CLIENT_STUB_H
#define _CLIENT_STUB_H

#include "data.h"
#include "list.h"

/* remote list contem info comunicacao server
 * definida pelo grupo em client_stub-private.h
 */

struct rlist_t; /* definida list-private.h */

/* funcao estabelece associacao client server
 * address_port string <hostname>:<port>
 * retorna rlist preenchida ou null erro
 */
struct rlist_t *rlist_connect(char *address_port);

/* termina associacao client server fecha ligacao liberta memoria
 * retorna 0 ok ou -1 erro
 */
int rlist_disconnect(struct rlist_t *rlist);

/* adiciona novo carro lista remota
 * insere ultima posicao
 * retorna 0 ok ou -1 erro
 */
int rlist_add(struct rlist_t *rlist, struct data_t *car);

/* remove lista remota primeiro carro modelo indicado
 * retorna 0 ok 1 nao encontrou -1 erro
 */
int rlist_remove_by_model(struct rlist_t *rlist, const char *modelo);

/* obtem primeiro carro marca indicada
 * retorna ponteiro dados ou null erro
 */
struct data_t *rlist_get_by_marca(struct rlist_t *rlist, enum marca_t marca);

/* obtem array ponteiros carros ano
 * ultimo elemento null
 * retorna array ou null erro
 */
struct data_t **rlist_get_by_year(struct rlist_t *rlist, int ano);

/* ordena lista remota ano crescente
 * retorna 0 ok ou -1 erro
 */
int rlist_order_by_year(struct rlist_t *rlist);

/* retorna numero carros lista remota ou -1 erro
 */
int rlist_size(struct rlist_t *rlist);

/* constroi array strings modelos lista remota
 * ultimo elemento null
 * retorna array ou null erro
 */
char **rlist_get_model_list(struct rlist_t *rlist);

/* liberta memoria array modelos
 * retorna 0 ok ou -1 erro
 */
int rlist_free_model_list(char **models);

#endif
