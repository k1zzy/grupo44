/**
 * @file data.c
 * @brief implementacao da struct data_t e funcoes
 *
 * implementacao funcoes criar destruir duplicar substituir dados carro definidos em data.h
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/data.h"
#include "../include/data-private.h"

struct data_t *data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {
    char *new_modelo = validate_params(ano, preco, marca, modelo, combustivel); // verifica params e strdup modelo
    if (!new_modelo) {
        return NULL; // erro a alocar memoria
    }

    struct data_t *new_data = (struct data_t *) malloc(sizeof(struct data_t));
    if (!new_data) {
        return NULL; // erro a alocar memoria
    }

    new_data->ano = ano;
    new_data->preco = preco;
    new_data->marca = marca;
    new_data->modelo = new_modelo; // atribui string alocada
    new_data->combustivel = combustivel;

    return new_data;
}

int data_destroy(struct data_t *data) {
    if (!data) {
        return -1; // pointer null
    }
    free(data->modelo); // liberta memoria modelo
    free(data); // liberta struct
    return 0;
}

struct data_t *data_dup(struct data_t *data) {
    if (!data) {
        return NULL; // pointer null
    }
    return data_create(data->ano, data->preco, data->marca, data->modelo, data->combustivel);
}

int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {
    if (!data) {
        return -1; // pointer null
    }

    char *new_modelo = validate_params(ano, preco, marca, modelo, combustivel); // verifica params e strdup modelo
    if (!new_modelo) {
        return -1; // erro a alocar memoria
    }

    // substitui dados
    data->ano = ano;
    data->preco = preco;
    data->marca = marca;

    free(data->modelo); // liberta string antiga modelo
    data->modelo = new_modelo; // atribui nova string

    data->combustivel = combustivel;

    return 0;
}

char *validate_params(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel)
{
    if (ano < 1886 || ano > 2100) { // 1886 primeiro carro
        return NULL; // ano invalido
    }
    if (preco < 0.0f) { // preco 0 ok
        return NULL; // preco invalido
    }
    if (marca < MARCA_TOYOTA || marca > MARCA_MERCEDES) {
        return NULL; // marca invalida
    }
    if (combustivel < COMBUSTIVEL_GASOLINA || combustivel > COMBUSTIVEL_HIBRIDO) {
        return NULL; // combustivel invalido
    }
    if (!modelo || strlen(modelo) == 0) {
        return NULL; // modelo invalido
    }

    return strdup(modelo);
}