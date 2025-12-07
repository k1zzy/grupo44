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
#ifndef _LIST_H
#define _LIST_H /* modulo list */

#include "list-private.h"
#include "data.h"

struct list_t; /* definida list-private.h */

/* cria e inicializa nova lista carros
 * retorna lista ou null erro
 */
struct list_t *list_create();

/* elimina lista liberta memoria
 * retorna 0 ok ou -1 erro
 */
int list_destroy(struct list_t *list);

/* adiciona novo carro lista
 * insere ultima posicao
 * retorna 0 ok ou -1 erro
 */
int list_add(struct list_t *list, struct data_t *car);

/* remove lista primeiro carro modelo indicado
 * retorna 0 ok 1 nao encontrou -1 erro
 */
int list_remove_by_model(struct list_t *list, const char *modelo);

/* obtem primeiro carro marca indicada
 * retorna ponteiro dados ou null erro
 */
struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca);

/* obtem array ponteiros carros ano
 * ultimo elemento null
 * retorna array ou null erro
 */
struct data_t **list_get_by_year(struct list_t *list, int ano);

/* ordena lista carros ano crescente
 * retorna 0 ok ou -1 erro
 */
int list_order_by_year(struct list_t *list);

/* retorna numero carros lista ou -1 erro
 */
int list_size(struct list_t *list);

/* constroi array strings modelos lista
 * ultimo elemento null
 * retorna array ou null erro
 */
char **list_get_model_list(struct list_t *list);

/* liberta memoria array modelos
 * retorna 0 ok ou -1 erro
 */
int list_free_model_list(char **models);

/* devolve array ponteiros terminado null para todos carros
 * cada elemento aponta dados internos lista nao duplicados
 * caller deve libertar apenas array free result nunca os data
 * retorna null erro
 */
struct data_t **list_get_all(struct list_t *list);

#endif
