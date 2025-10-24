#include "../include/list_skel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/sdmessage.pb-c.h"
#include "../include/list.h"
#include "../include/data.h"
#include "../include/data-private.h"


int list_skel_init() {
    struct list_t *list = list_create();
    if (!list) {
        return NULL;
    }
    return list;
}

int list_skel_destroy(struct list_t *list) {
    if (!list) return -1;
    return list_destroy(list);
}

int invoke(MessageT *msg, struct list_t *list) {
    if (!msg || !list) return -1;

    switch (msg->opcode) {
    case MESSAGE_T__OPCODE__OP_ADD: {
        /* request: msg->data (Data). response: CT_RESULT with result from list_add */
        if (msg->c_type != MESSAGE_T__C_TYPE__CT_DATA || !msg->data)
            return -1;
        Data *pd = msg->data;
        /* converter Data -> struct data_t */
        struct data_t car;
        memset(&car, 0, sizeof(car));
        car.ano = pd->ano;
        car.preco = pd->preco;
        car.marca = (enum marca_t)pd->marca;
        car.modelo = pd->modelo ? pd->modelo : "";
        car.combustivel = (enum combustivel_t)pd->combustivel;

        int r = list_add(list, &car);
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        /* leave msg->data as-is (it will be freed by message_t__free_unpacked) */
        break;
    }

    case MESSAGE_T__OPCODE__OP_DEL: {
        /* request: msg->models[0] contains model string. response: CT_RESULT */
        if (msg->c_type != MESSAGE_T__C_TYPE__CT_MODEL || msg->n_models < 1 || !msg->models)
            return -1;
        const char *model = msg->models[0];
        int r = list_remove_by_model(list, model);
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        break;
    }

    case MESSAGE_T__OPCODE__OP_GET: {
        /* request: marca in msg->result. response: CT_DATA with single Data or CT_RESULT if none */
        enum marca_t marca = (enum marca_t)msg->result;
        struct data_t *found = list_get_by_marca(list, marca);
        if (!found) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1; /* not found */
            break;
        }
        /* build protobuf Data from struct data_t and attach to msg */
        Data *pd = malloc(sizeof(Data));
        if (!pd) {
            /* free returned struct? leave to caller/list implementation */
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = -1;
            break;
        }
        data__init(pd);
        pd->ano = found->ano;
        pd->preco = found->preco;
        pd->marca = (Marca)found->marca;
        pd->modelo = found->modelo ? strdup(found->modelo) : NULL;
        pd->combustivel = (Combustivel)found->combustivel;

        msg->c_type = MESSAGE_T__C_TYPE__CT_DATA;
        msg->data = pd;
        /* do not free 'found' here (depends on list implementation ownership) */
        break;
    }

    case MESSAGE_T__OPCODE__OP_GETLISTBYYEAR: {
        /* request: year in msg->result. response: CT_LIST with cars array */
        int year = msg->result;
        struct data_t **arr = list_get_by_year(list, year);
        if (!arr) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1; /* not found / empty */
            break;
        }

        /* count items */
        size_t n = 0;
        while (arr[n] != NULL) n++;

        if (n == 0) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1;
            /* free arr if list_get_by_year allocated it - leave responsibility to list implementation */
            break;
        }

        /* allocate protobuf Data* array */
        msg->cars = malloc(sizeof(Data *) * n);
        if (!msg->cars) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = -1;
            break;
        }
        msg->n_cars = n;
        for (size_t i = 0; i < n; ++i) {
            Data *pd = malloc(sizeof(Data));
            if (!pd) { /* on allocation failure set what we have and return error */
                msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
                msg->result = -1;
                /* free previously allocated */
                for (size_t j = 0; j < i; ++j) {
                    if (msg->cars[j]->modelo) free(msg->cars[j]->modelo);
                    free(msg->cars[j]);
                }
                free(msg->cars);
                msg->cars = NULL;
                msg->n_cars = 0;
                break;
            }
            data__init(pd);
            pd->ano = arr[i]->ano;
            pd->preco = arr[i]->preco;
            pd->marca = (Marca)arr[i]->marca;
            pd->modelo = arr[i]->modelo ? strdup(arr[i]->modelo) : NULL;
            pd->combustivel = (Combustivel)arr[i]->combustivel;
            msg->cars[i] = pd;
        }
        if (msg->c_type != MESSAGE_T__C_TYPE__CT_RESULT) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
        }
        /* do not free arr or its elements here - leave to list implementation or caller policy */
        break;
    }

    case MESSAGE_T__OPCODE__OP_ORDER: {
        int r = list_order_by_year(list);
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        break;
    }

    case MESSAGE_T__OPCODE__OP_SIZE: {
        int r = list_size(list);
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        break;
    }

    case MESSAGE_T__OPCODE__OP_GETMODELS: {
        char **models = list_get_model_list(list);
        if (!models) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1;
            break;
        }
        /* count models */
        size_t n = 0;
        while (models[n] != NULL) n++;

        msg->models = malloc(sizeof(char *) * n);
        if (!msg->models) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = -1;
            break;
        }
        msg->n_models = n;
        for (size_t i = 0; i < n; ++i) {
            msg->models[i] = strdup(models[i] ? models[i] : "");
        }
        msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
        /* do not free 'models' here - follow list API ownership semantics */
        break;
    }

    default:
        return -1;
    }

    return 0;
}