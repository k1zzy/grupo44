/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */
#ifndef _NETWORK_CLIENT_H
#define _NETWORK_CLIENT_H

#include "client_stub.h"
#include "sdmessage.pb-c.h"

/* funcao connect:
 * - obtem endereco server (struct sockaddr_in) struct rlist
 * - liga ao server
 * - guarda info necessaria (socket) rlist
 * - retorna 0 ok ou -1 erro
 */
int network_connect(struct rlist_t *rlist);

/* funcao send receive:
 * - obtem socket rlist_t
 * - serializa msg
 * - envia msg serializada server
 * - espera resposta server
 * - deserializa msg resposta
 * - trata erros comunicacao
 * - retorna msg deserializada ou null erro
 */
MessageT *network_send_receive(struct rlist_t *rlist, MessageT *msg);

/* fecha ligacao network_connect()
 * retorna 0 ok ou -1 erro
 */
int network_close(struct rlist_t *rlist);

#endif