/**
 * @file data.c
 * @brief Implementação da estrutura de dados data_t e das respetivas funções.
 *
 * Este ficheiro contém a implementação das funções para criar, destruir,
 * duplicar e substituir dados de um automóvel, conforme definido em data.h.
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
    char *new_modelo = validate_params(ano, preco, marca, modelo, combustivel); // verifica params e faz strdup do modelo
    if (!new_modelo) {
        return NULL; // erro ao alocar memória
    }

    struct data_t *new_data = (struct data_t *) malloc(sizeof(struct data_t));
    if (!new_data) {
        return NULL; // erro ao alocar memória
    }

    new_data->ano = ano;
    new_data->preco = preco;
    new_data->marca = marca;
    new_data->modelo = new_modelo; // atribuir a string alocada
    new_data->combustivel = combustivel;

    return new_data;
}

int data_destroy(struct data_t *data) {
    if (!data) {
        return -1; // passado NULL pointer
    }
    free(data->modelo); // libertar memória alocada para o modelo
    free(data); // libertar a estrutura data_t
    return 0;
}

struct data_t *data_dup(struct data_t *data) {
    if (!data) {
        return NULL; // passado NULL pointer
    }
    return data_create(data->ano, data->preco, data->marca, data->modelo, data->combustivel);
}

int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel) {
    if (!data) {
        return -1; // passado NULL pointer
    }

    char *new_modelo = validate_params(ano, preco, marca, modelo, combustivel); // verifica params e faz strdup do modelo
    if (!new_modelo) {
        return -1; // erro ao alocar memória
    }

    // substituir os dados
    data->ano = ano;
    data->preco = preco;
    data->marca = marca;

    free(data->modelo); // libertar a string antiga do modelo
    data->modelo = new_modelo; // atribuir a nova string

    data->combustivel = combustivel;

    return 0;
}

char *validate_params(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel)
{
    if (ano < 1886 || ano > 2100) { // 1886 = primeiro carro (Benz Patent-Motorwagen)
        return NULL; // ano inválido
    }
    if (preco < 0.0f) { // permitir preço 0 (carros oferecidos, etc)
        return NULL; // preço inválido
    }
    if (marca < MARCA_TOYOTA || marca > MARCA_MERCEDES) {
        return NULL; // marca inválida
    }
    if (combustivel < COMBUSTIVEL_GASOLINA || combustivel > COMBUSTIVEL_HIBRIDO) {
        return NULL; // combustível inválido
    }
    if (!modelo || strlen(modelo) == 0) {
        return NULL; // modelo inválido
    }

    return strdup(modelo);
}