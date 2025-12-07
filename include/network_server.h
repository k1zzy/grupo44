/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */
#ifndef _NETWORK_SERVER_H
#define _NETWORK_SERVER_H

#include "client_stub.h"
#include "list.h"
#include "sdmessage.pb-c.h"

/* funcao prepara socket rececao pedidos ligacao porto
 * retorna descritor socket ou -1 erro
 */
int network_server_init(short port);

/* ciclo principal rede:
 * - aceita conexao cliente
 * - recebe mensagem network_receive
 * - entrega mensagem deserializada skeleton processar
 * - espera resposta skeleton
 * - envia resposta cliente network_send
 * nao deve retornar a menos que erro (-1)
 */
int network_main_loop(int listening_socket, struct list_t *list);

/* recebe mensagem:
 * - lee bytes rede client_socket
 * - deserializa bytes e constroi mensagem pedido
 *   reservando memoria messaget
 * retorna mensagem pedido ou null erro
 */
MessageT *network_receive(int client_socket);

/* envia mensagem:
 * - serializa mensagem resposta msg
 * - envia mensagem serializada client_socket
 * retorna 0 ok ou -1 erro
 */
int network_send(int client_socket, MessageT *msg);

/* liberta recursos network_server_init()
 * fechar socket
 * retorna 0 ok ou -1 erro
 */
int network_server_close(int socket);

/* sinaliza termino e fecha fds internos (desbloqueia accept/read)*/
void network_server_request_shutdown(void);

/* espera threads terminarem e liberta recursos aux */
void network_server_join_threads(void);

/* define server sucessor propagacao escrita */
// void network_server_set_successor(struct rlist_t *successor);

#endif
