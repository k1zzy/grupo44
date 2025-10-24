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
    if (!msg || !list) {
        if (msg) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        }
        return -1;
    }

    switch (msg->opcode) {
    case MESSAGE_T__OPCODE__OP_ADD: {
        if (msg->c_type != MESSAGE_T__C_TYPE__CT_DATA || !msg->data) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        Data *pd = msg->data;
        struct data_t car;
        memset(&car, 0, sizeof(car));
        car.ano = pd->ano;
        car.preco = pd->preco;
        car.marca = (enum marca_t)pd->marca;
        car.modelo = pd->modelo ? pd->modelo : "";
        car.combustivel = (enum combustivel_t)pd->combustivel;

        int r = list_add(list, &car);
        if (r < 0) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_ADD + 1
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        break;
    }

    case MESSAGE_T__OPCODE__OP_DEL: {
        if (msg->c_type != MESSAGE_T__C_TYPE__CT_MODEL || msg->n_models < 1 || !msg->models) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        const char *model = msg->models[0];
        int r = list_remove_by_model(list, model);
        if (r < 0) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        break;
    }

    case MESSAGE_T__OPCODE__OP_GET: {
        enum marca_t marca = (enum marca_t)msg->result;
        struct data_t *found = list_get_by_marca(list, marca);
        if (!found) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1; /* not found */
            break;
        }
        Data *pd = malloc(sizeof(Data));
        if (!pd) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        data__init(pd);
        pd->ano = found->ano;
        pd->preco = found->preco;
        pd->marca = (Marca)found->marca;
        pd->modelo = found->modelo ? strdup(found->modelo) : NULL;
        pd->combustivel = (Combustivel)found->combustivel;

        msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1
        msg->c_type = MESSAGE_T__C_TYPE__CT_DATA;
        msg->data = pd;
        break;
    }

    case MESSAGE_T__OPCODE__OP_GETLISTBYYEAR: {
        int year = msg->result;
        struct data_t **arr = list_get_by_year(list, year);
        if (!arr) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1; /* not found / empty */
            break;
        }
        size_t n = 0;
        while (arr[n] != NULL) n++;
        if (n == 0) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = 1;
            break;
        }
        msg->cars = malloc(sizeof(Data *) * n);
        if (!msg->cars) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->n_cars = n;
        for (size_t i = 0; i < n; ++i) {
            Data *pd = malloc(sizeof(Data));
            if (!pd) {
                /* rollback allocated entries */
                for (size_t j = 0; j < i; ++j) {
                    if (msg->cars[j]->modelo) free(msg->cars[j]->modelo);
                    free(msg->cars[j]);
                }
                free(msg->cars);
                msg->cars = NULL;
                msg->n_cars = 0;
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            data__init(pd);
            pd->ano = arr[i]->ano;
            pd->preco = arr[i]->preco;
            pd->marca = (Marca)arr[i]->marca;
            pd->modelo = arr[i]->modelo ? strdup(arr[i]->modelo) : NULL;
            pd->combustivel = (Combustivel)arr[i]->combustivel;
            msg->cars[i] = pd;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_GETLISTBYYEAR + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
        break;
    }

    case MESSAGE_T__OPCODE__OP_ORDER: {
        int r = list_order_by_year(list);
        if (r < 0) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_ORDER + 1
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->result = r;
        break;
    }

    case MESSAGE_T__OPCODE__OP_SIZE: {
        int r = list_size(list);
        if (r < 0) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1
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
        size_t n = 0;
        while (models[n] != NULL) n++;
        msg->models = malloc(sizeof(char *) * n);
        if (!msg->models) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }
        msg->n_models = n;
        for (size_t i = 0; i < n; ++i) {
            msg->models[i] = strdup(models[i] ? models[i] : "");
            if (!msg->models[i]) {
                for (size_t j = 0; j < i; ++j) free(msg->models[j]);
                free(msg->models);
                msg->models = NULL;
                msg->n_models = 0;
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_GETMODELS + 1
        msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
        break;
    }

    default:
        msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    return 0;
}