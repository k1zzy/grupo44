/**
 * @file data-private.h
 *
 * Funções privadas para manipulação de dados de automóveis.
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */
#ifndef _DATA_PRIVATE_H
#define _DATA_PRIVATE_H

#include "data.h"

/* Função que valida os parâmetros de entrada e retorna o ponteiro para o strdup do modelo ou NULL caso
 * os parâmetros sejam inválidos ou o strdup falhe.
 */
char *validate_params(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel);

#endif
