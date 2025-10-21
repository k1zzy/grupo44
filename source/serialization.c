/** 
* Projeto: Sistemas Distribuídos 2025/2026
* Grupo 44
* Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "../include/data.h"
#include "../include/list.h" 
#include "../include/serialization.h"
#include "../include/list-private.h"


int car_to_buffer(struct data_t *car, char **car_buf) {
    if (!car || !car_buf) {
        return -1;
    }

    int modelo_len = strlen(car->modelo);
    int total_size = 4 + 4 + 4 + 4 + (4 + modelo_len); // ano, preco, marca, combustivel, modelo (len+bytes)

    *car_buf = malloc(total_size);
    if (!*car_buf) return -1;

    char *p = *car_buf;

    int net_ano = htonl(car->ano);
    int net_preco = htonl((int)(car->preco * 100));
    int net_marca = htonl(car->marca); //enums sao guardados como inteiros
    int net_combustivel = htonl(car->combustivel);
    int net_modelo_len = htonl(modelo_len);

    memcpy(p, &net_ano, 4); p += 4;
    memcpy(p, &net_preco, 4); p += 4;
    memcpy(p, &net_marca, 4); p += 4;
    memcpy(p, &net_combustivel, 4); p += 4;
    memcpy(p, &net_modelo_len, 4); p += 4;
    memcpy(p, (const void *)car->modelo, modelo_len); p += modelo_len;

    return total_size;
}




struct data_t *buffer_to_car(char *car_buf) {
    if (!car_buf) return NULL;

    struct data_t *car = malloc(sizeof(struct data_t));
    if (!car) return NULL;

    char *p = car_buf;

    int net_ano, net_preco, net_marca, net_combustivel, net_modelo_len;

    memcpy(&net_ano, p, 4); p += 4;
    car->ano = ntohl(net_ano);

    memcpy(&net_preco, p, 4); p += 4;
    car->preco = ntohl(net_preco) / 100.0;

    memcpy(&net_marca, p, 4); p += 4;
    car->marca = ntohl(net_marca);

    memcpy(&net_combustivel, p, 4); p += 4;
    car->combustivel = ntohl(net_combustivel);

    memcpy(&net_modelo_len, p, 4); p += 4;
    int modelo_len = ntohl(net_modelo_len);
    char *modelo_buf = malloc(modelo_len + 1);            // alocar mem
    memcpy(modelo_buf, p, modelo_len); p += modelo_len;
    modelo_buf[modelo_len] = '\0';                  // terminar com null byte
    car->modelo = modelo_buf;

    return car;
}


int car_list_to_buffer(struct list_t *list, char **list_buf) {
    if (list == NULL || list_buf == NULL) {
        return -1;
    }

    // primeiro calcular o tamanho total necessário
    int car_count = 0;
    int total_size = 4; // 4 bytes para o número de carros
    struct car_t *current = list->head;
    while (current != NULL) {
        char *car_buf = NULL;
        int car_size = car_to_buffer(current->data, &car_buf);
        if (car_size < 0) {
            return -1;
        }
        total_size += 4 + car_size; // 4 bytes para o tamanho + tamanho do carro
        free(car_buf);
        car_count++;
        current = current->next;
    }

    *list_buf = malloc(total_size);
    if (*list_buf == NULL) {
        return -1;
    }

    // escreve o número de carros
    char *p = *list_buf;
    int net_car_count = htonl(car_count);
    memcpy(p, &net_car_count, 4); p += 4;

    // escreve cada carro: [tamanho][buffer]
    current = list->head;
    while (current != NULL) {
        char *car_buf = NULL;
        int car_size = car_to_buffer(current->data, &car_buf);
        int net_car_size = htonl(car_size);
        memcpy(p, &net_car_size, 4); p += 4;
        memcpy(p, car_buf, car_size); p += car_size;
        free(car_buf);
        current = current->next;
    }

    return total_size;
}

struct list_t *buffer_to_car_list(char *list_buf) {
    if (list_buf == NULL) {
        return NULL;
    }

    char *p = list_buf;
    int net_car_count;
    memcpy(&net_car_count, p, 4); p += 4;
    int car_count = ntohl(net_car_count);

    struct list_t *list = list_create();
    if (list == NULL) {
        return NULL;
    }

    for (int i = 0; i < car_count; i++) {
        int net_car_size;
        memcpy(&net_car_size, p, 4); p += 4;
        int car_size = ntohl(net_car_size);
        struct data_t *car = buffer_to_car(p);
        if (car == NULL) {
            list_destroy(list);
            return NULL;
        }
        list_add(list, car);
        p += car_size;
    }
    return list;
}
