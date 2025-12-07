/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 4/10/2025
 */
#ifndef _LIST_SKEL_H
#define _LIST_SKEL_H

#include "sdmessage.pb-c.h"
#include "list.h"

/* inicia skeleton lista
 * main() servidor deve chamar antes invoke()
 * retorna tabela criada ou null erro
 */
struct list_t *list_skel_init();

/* liberta memoria skeleton e recursos
 * retorna 0 ok ou -1 erro
 */
int list_skel_destroy(struct list_t *list);

/* executa operacao opcode msg na lista
 * usa mesma struct messaget devolver resultado
 * retorna 0 ok ou -1 erro
 */
int invoke(MessageT *msg, struct list_t *list);

#endif
