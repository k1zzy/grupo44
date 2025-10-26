/** 
* Projeto: Sistemas Distribuídos 2025/2026
* Grupo 44
* Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
*/

#include "../include/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/list-private.h"
#include "../include/data.h"


struct list_t *list_create(){
    // Allocate memory for the list structure
    struct list_t *list = malloc(sizeof(struct list_t));
    if (list == NULL) {
        return NULL; // Allocation error
    }
    list -> size = 0; // When created, the list is empty
    list -> head = NULL; // Not sure about it

    return list;
}

int list_destroy(struct list_t *list){
    if(list == NULL){
        return -1;  // Error
    }
    struct car_t *current = list -> head; //Starts on first car
    struct car_t *next;
    while(current != NULL){
        next = current -> next;
        if(current -> data != NULL){
            data_destroy(current -> data); // Delete data of current car
        }
        free(current);
        current = next;
    } // Destroy each car of the list
    free(list); // Desalocate list structure
    return 0;
}

int list_add(struct list_t *list, struct data_t *car){
    if(list == NULL || car == NULL){
        return -1;
    }
    
    struct car_t *new_car = malloc(sizeof(struct car_t));
    if (new_car == NULL) return -1;
    
    new_car->data = car;
    new_car->next = NULL;

    // empty list
    if(list->head == NULL){
        list->head = new_car;
    } else {
        struct car_t *current = list->head;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_car;
    }
    
    list->size++;
    return 0;
}

int list_remove_by_model(struct list_t *list, const char *modelo){
    if(list == NULL || modelo == NULL || list->head == NULL){
        return -1;
    }
    
    struct car_t *current = list->head;
    struct car_t *previous = NULL;  
    
    while(current != NULL){
        if(current->data != NULL && current->data->modelo != NULL && 
           strcmp(current->data->modelo, modelo) == 0){
            
            if(previous == NULL){
                list->head = current->next;
            } else {
                previous->next = current->next;
            }
            
            data_destroy(current->data);
            free(current);
            list->size--;
            return 0;  
        }
        
        previous = current; 
        current = current->next;
    }
    
    return 1; 
}

struct data_t *list_get_by_marca(struct list_t *list, enum marca_t marca){
    if(list == NULL || list -> head == NULL){
        return NULL; // Error
    }
    struct car_t *current = list -> head;
    while(current != NULL){
        if (current -> data -> marca == marca){
            return current -> data;
        }
        current = current -> next;
    }
    return NULL; // if not found
}

struct data_t **list_get_by_year(struct list_t *list, int ano){
    if(list == NULL || list->head == NULL){
        return NULL;
    }
    
    int count = 0;
    struct car_t *current = list->head;
    
    // Count cars by specified year
    while(current != NULL){
        if(current->data != NULL && current->data->ano == ano){
            count++;
        }
        current = current->next;
    }
    
    if(count == 0) return NULL;
    
    struct data_t **array = malloc((count + 1) * sizeof(struct data_t *));
    if(array == NULL) return NULL;
    
    // fill array
    current = list->head;
    int i = 0;
    while(current != NULL){
        if(current->data != NULL && current->data->ano == ano){
            array[i] = current->data;
            i++;
        }
        current = current->next;
    }
    
    array[count] = NULL; // NULL-terminate the array
    
    return array;
}

// TO DO
int list_order_by_year(struct list_t *list){
    if(list == NULL || list->size <= 1){
        return 0;  // Error
    }
    
    
    int trocado;
    struct car_t *current;
    
    do {
        trocado = 0;
        current = list->head;
        
        while(current != NULL && current->next != NULL){
            if(current->data->ano > current->next->data->ano){
                struct data_t *temp = current->data;
                current->data = current->next->data;
                current->next->data = temp;
                trocado = 1;
            }
            current = current->next;
        }
    } while(trocado);
    
    return 0;
}

int list_size(struct list_t *list){
    if(list == NULL){
        return -1; // Error
    }
    return list -> size;
}

char **list_get_model_list(struct list_t *list){
    if(list == NULL || list -> size == 0){
        return NULL; // Error
    }

    char **models = malloc((list -> size + 1) * sizeof(char *));
    if(models == NULL){
        return NULL; // Error
    }

    struct car_t *current = list -> head;
    int i = 0;
    while(current != NULL){
        models[i] = strdup(current -> data -> modelo);
        if(models[i] == NULL){
            list_free_model_list(models);
            return NULL; // Error
        }
        i++;
        current = current -> next;
    }
    models[i] = NULL; // Ultima posicao nula
    return models;
}

int list_free_model_list(char **models){
    if(models == NULL){
        return -1; // Error
    }
    int i = 0;
    while(models[i] != NULL){
        free(models[i]);
        i++;
    }
    free(models);
    return 0;  
}


struct data_t **list_get_all(struct list_t *list){
    if(list == NULL || list -> size == 0){
        return NULL; // Error
    }

    struct data_t **array = malloc((list -> size + 1) * sizeof(struct data_t *));
    if(array == NULL){
        return NULL; // Error
    }

    struct car_t *current = list -> head;
    int i = 0;
    while(current != NULL){
        array[i] = current -> data;
        i++;
        current = current -> next;
    }
    array[i] = NULL; // Ultima posicao nula
    return array;
}
