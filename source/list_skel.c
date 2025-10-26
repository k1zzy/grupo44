#include "../include/list_skel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/sdmessage.pb-c.h"
#include "../include/list.h"
#include "../include/data.h"
#include "../include/data-private.h"


struct list_t *list_skel_init() {
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

int invokeOLD(MessageT *msg, struct list_t *list) {
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
        msg->opcode = MESSAGE_T__OPCODE__OP_ADD + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
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
        msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
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

        msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        msg->data = pd;
        break;
    }

    case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR: {
        int year = msg->result;
        struct data_t **arr = list_get_by_year(list, year);
        if (!arr) {
            msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
            msg->opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1; // TODO nao sei se quando e vazio conta como sucesso
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
        msg->opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1;
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
        msg->opcode = MESSAGE_T__OPCODE__OP_ORDER + 1;
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
        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
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
        msg->opcode = MESSAGE_T__OPCODE__OP_GETMODELS + 1;
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

int invoke(MessageT *msg, struct list_t *list) {
    if (!msg || !list) {
        return -1;
    }

    unsigned int opcode = msg->opcode;

    switch (opcode) {
        case MESSAGE_T__OPCODE__OP_ADD: { // opcode 10
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_DATA || !msg->data) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            Data *pd = msg->data;
            
            printf("[DEBUG] OP_ADD: modelo=%s, ano=%d, preco=%.2f, marca=%d, combustivel=%d\n",
                   pd->modelo ? pd->modelo : "NULL", pd->ano, pd->preco, pd->marca, pd->combustivel);
            
            // Duplicar o modelo antes de criar data_t
            char *modelo_copy = strdup(pd->modelo ? pd->modelo : "");
            if (!modelo_copy) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                free(NULL); // matching the binary
                return 0;
            }
            
            // Criar struct data_t usando data_create
            // data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel)
            struct data_t *car = data_create(
                pd->ano,
                pd->preco,
                (enum marca_t)pd->marca,
                modelo_copy,
                (enum combustivel_t)pd->combustivel
            );
            
            if (!car) {
                printf("[DEBUG] OP_ADD: Falha ao criar data_t\n");
                free(modelo_copy);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            // Adicionar à lista
            int add_result = list_add(list, car);
            printf("[DEBUG] OP_ADD: list_add retornou %d\n", add_result);
            
            if (add_result != 0) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                data_destroy(car);
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ADD + 1; // 11
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                printf("[DEBUG] OP_ADD: Carro adicionado com sucesso! Tamanho da lista: %d\n", list_size(list));
            }
            
            free(modelo_copy);
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_GET: { // opcode 20
            printf("[DEBUG] OP_GET: list=%p, list->size=%d, c_type=%d, result=%d\n", 
                   (void*)list, list->size, msg->c_type, msg->result);
            
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_MARCA) {
                printf("[DEBUG] OP_GET: c_type incorreto (esperado CT_MARCA=20, recebido=%d)\n", msg->c_type);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            enum marca_t marca = (enum marca_t)msg->result;
            printf("[DEBUG] OP_GET: Procurando marca=%d\n", marca);
            
            struct data_t *found = list_get_by_marca(list, marca);
            
            printf("[DEBUG] OP_GET: list_get_by_marca retornou %p\n", (void*)found);
            
            if (!found) {
                printf("[DEBUG] OP_GET: Nenhum carro encontrado\n");
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            // Alocar e inicializar Data protobuf
            Data *pd = malloc(sizeof(Data));
            if (!pd) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            data__init(pd);
            pd->ano = found->ano;
            pd->preco = found->preco;
            pd->marca = (Marca)found->marca;
            pd->modelo = strdup(found->modelo);
            
            if (!pd->modelo) {
                free(pd);
                msg->data = NULL;
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            pd->combustivel = (Combustivel)found->combustivel;
            
            printf("[DEBUG] OP_GET: Retornando carro modelo=%s, ano=%d\n", pd->modelo, pd->ano);
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1; // 21
            msg->c_type = MESSAGE_T__C_TYPE__CT_DATA;
            msg->data = pd;
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_DEL: { // opcode 30
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_MODEL || !msg->models) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            const char *model = msg->models[0];
            int result = list_remove_by_model(list, model);
            
            msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1; // 31
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = result;
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_SIZE: { // opcode 40
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            printf("[DEBUG] OP_SIZE: list=%p, list->size=%d\n", (void*)list, list->size);
            
            int size = list_size(list);
            
            printf("[DEBUG] OP_SIZE: list_size() retornou %d\n", size);
            
            msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1; // 41
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = size;
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_GETMODELS: { // opcode 50
            printf("[DEBUG] OP_GETMODELS: list=%p, list->size=%d\n", (void*)list, list->size);
            
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
                printf("[DEBUG] OP_GETMODELS: c_type incorreto (%d)\n", msg->c_type);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            char **model_list = list_get_model_list(list);
            printf("[DEBUG] OP_GETMODELS: list_get_model_list retornou %p\n", (void*)model_list);
            
            if (!model_list) {
                printf("[DEBUG] OP_GETMODELS: model_list é NULL\n");
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GETMODELS + 1; // 51
            msg->c_type = MESSAGE_T__C_TYPE__CT_MODEL;
            
            // Contar modelos
            int count = 0;
            while (model_list[count] != NULL) {
                count++;
            }
            
            printf("[DEBUG] OP_GETMODELS: count=%d\n", count);
            
            msg->n_models = count;
            msg->models = malloc(sizeof(char*) * count);
            
            if (!msg->models) {
                list_free_model_list(model_list);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            // Copiar modelos
            for (int i = 0; i < count; i++) {
                msg->models[i] = strdup(model_list[i]);
                if (!msg->models[i]) {
                    // Rollback em caso de erro
                    for (int j = 0; j < i; j++) {
                        free(msg->models[j]);
                    }
                    free(msg->models);
                    list_free_model_list(model_list);
                    msg->models = NULL;
                    msg->n_models = 0;
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    return 0;
                }
            }
            
            list_free_model_list(model_list);
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR: { // opcode 60
            printf("[DEBUG] OP_GETLISTBYTEAR: list=%p, list->size=%d, result=%d\n", 
                   (void*)list, list->size, msg->result);
            
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_RESULT) {
                printf("[DEBUG] OP_GETLISTBYTEAR: c_type incorreto (%d)\n", msg->c_type);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            struct data_t **arr;
            
            // Se result é -1, ordena e obtém todos; caso contrário, filtra por ano
            if (msg->result == -1) {
                printf("[DEBUG] OP_GETLISTBYTEAR: Ordenando por ano e obtendo todos\n");
                list_order_by_year(list);
                arr = list_get_all(list);
            } else {
                printf("[DEBUG] OP_GETLISTBYTEAR: Filtrando por ano=%d\n", msg->result);
                arr = list_get_by_year(list, msg->result);
            }
            
            printf("[DEBUG] OP_GETLISTBYTEAR: arr=%p\n", (void*)arr);
            
            if (!arr) {
                printf("[DEBUG] OP_GETLISTBYTEAR: arr é NULL\n");
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1; // 61
            msg->c_type = MESSAGE_T__C_TYPE__CT_LIST;
            
            // Contar elementos
            int m = 0;
            while (arr[m] != NULL) {
                m++;
            }
            
            msg->n_cars = m;
            msg->cars = malloc(sizeof(Data*) * m);
            
            if (!msg->cars) {
                free(arr);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            // Converter cada data_t para Data protobuf
            for (int n = 0; n < m; n++) {
                Data *pd = malloc(sizeof(Data));
                if (!pd) {
                    // Rollback
                    for (int ii = 0; ii < n; ii++) {
                        free(msg->cars[ii]);
                    }
                    free(msg->cars);
                    free(arr);
                    msg->cars = NULL;
                    msg->n_cars = 0;
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    return 0;
                }
                
                data__init(pd);
                pd->ano = arr[n]->ano;
                pd->preco = arr[n]->preco;
                pd->marca = (Marca)arr[n]->marca;
                pd->modelo = strdup(arr[n]->modelo);
                
                if (!pd->modelo) {
                    // Rollback incluindo o atual
                    for (int jj = 0; jj <= n; jj++) {
                        if (msg->cars[jj]) {
                            free(msg->cars[jj]);
                        }
                    }
                    free(msg->cars);
                    free(arr);
                    msg->cars = NULL;
                    msg->n_cars = 0;
                    msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    return 0;
                }
                
                pd->combustivel = (Combustivel)arr[n]->combustivel;
                msg->cars[n] = pd;
            }
            
            free(arr);
            return 0;
        }

        default:
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;
    }
}