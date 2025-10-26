/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */

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


int invoke(MessageT *msg, struct list_t *list) {
    if (!msg || !list) {
        return -1;
    }

    unsigned int opcode = msg->opcode;

    switch (opcode) {
        case MESSAGE_T__OPCODE__OP_ADD: { 
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_DATA || !msg->data) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
         Data *pd = msg->data;
            
            // Duplicar o modelo antes de criar data_t
            char *modelo_copy = strdup(pd->modelo ? pd->modelo : "");
            if (!modelo_copy) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                free(NULL); // matching the binary
                return 0;
            }
            
            struct data_t *car = data_create(
                pd->ano,
                pd->preco,
                (enum marca_t)pd->marca,
                modelo_copy,
                (enum combustivel_t)pd->combustivel
            );
            
            if (!car) {
                free(modelo_copy);
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            // Adicionar à lista
            int add_result = list_add(list, car);
            
            if (add_result != 0) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                data_destroy(car);
            } else {
                msg->opcode = MESSAGE_T__OPCODE__OP_ADD + 1; 
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            }
            
            free(modelo_copy);
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_GET: { 
                   (void*)list, list->size, msg->c_type, msg->result);
            
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_MARCA) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            enum marca_t marca = (enum marca_t)msg->result;
            
            struct data_t *found = list_get_by_marca(list, marca);
                        
            if (!found) {
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
                        
            msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1; 
            msg->c_type = MESSAGE_T__C_TYPE__CT_DATA;
            msg->data = pd;
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_DEL: { 
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_MODEL || !msg->models) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            const char *model = msg->models[0];
            int result = list_remove_by_model(list, model);
            
            msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1; 
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = result;
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_SIZE: { 
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
                        
            int size = list_size(list);
            
            msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1; 
            msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
            msg->result = size;
            return 0;
        }

        case MESSAGE_T__OPCODE__OP_GETMODELS: { 
            
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_NONE) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            char **model_list = list_get_model_list(list);
            
            if (!model_list) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GETMODELS + 1; 
            msg->c_type = MESSAGE_T__C_TYPE__CT_MODEL;
            
            // Contar os modelos
            int count = 0;
            while (model_list[count] != NULL) {
                count++;
            }
            
            
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

        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR: { 
                   (void*)list, list->size, msg->result);
            
            if (msg->c_type != MESSAGE_T__C_TYPE__CT_RESULT) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            struct data_t **arr;
            
            // Se result é -1, ordena e obtém todos; caso contrário, filtra por ano
            if (msg->result == -1) {
                list_order_by_year(list);
                arr = list_get_all(list);
            } else {
                arr = list_get_by_year(list, msg->result);
            }
                        
            if (!arr) {
                msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return 0;
            }
            
            msg->opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1; 
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