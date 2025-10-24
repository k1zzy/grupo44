#include "list_skel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sdmessage.pb-c.h"

struct list_t *list_skel_init() {
    struct list_t *list = list_create();
    if (!list) {
        return NULL; // falha ao criar a lista
    }
    return list;
}

// TODO ele fala em "todos os recursos usados pelo skeleton", mas quais seriam?
int list_skel_destroy(struct list_t *list) {
    if (!list) {
        return -1; // lista inválida
    }
    int res = list_destroy(list);
    return res;
}

int invoke(MessageT *msg, struct list_t *list) {
    // placeholder para a implementação da função invoke
    return -1; // Placeholder
}