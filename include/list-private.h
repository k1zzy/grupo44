/**
 * @file list-private.h
 *
 * Estrutura interna da lista de automóveis.
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 13/09/2025
 */
#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H

#include "data.h"

/* Nó da lista que contém um carro */
struct car_t
{
	struct data_t *data; // Dados do carro
	struct car_t *next;	 // Próximo nó
};

/* Estrutura da lista de carros */
struct list_t
{
	int size;			// Número de carros na lista
	struct car_t *head; // Cabeça da lista
};

#endif
