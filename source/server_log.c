/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */
#include "../include/server_log.h"
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <sdmessage.pb-c.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

char *translate_op_code(MessageT__Opcode opcode) {
    switch (opcode) {
        case MESSAGE_T__OPCODE__OP_BAD: return "OP_BAD";
        case MESSAGE_T__OPCODE__OP_ADD: return "OP_ADD";
        case MESSAGE_T__OPCODE__OP_GET: return "OP_GET";
        case MESSAGE_T__OPCODE__OP_DEL: return "OP_DEL";
        case MESSAGE_T__OPCODE__OP_SIZE: return "OP_SIZE";
        case MESSAGE_T__OPCODE__OP_GETMODELS: return "OP_GETMODELS";
        case MESSAGE_T__OPCODE__OP_GETLISTBYTEAR: return "OP_GETLISTBYTEAR";
        case MESSAGE_T__OPCODE__OP_ORDER: return "OP_ORDER";
        case MESSAGE_T__OPCODE__OP_BUSY: return "OP_BUSY";
        case MESSAGE_T__OPCODE__OP_READY: return "OP_READY";
        case MESSAGE_T__OPCODE__OP_ERROR: return "OP_ERROR";
        default: return "UNKNOWN_OP";
    }
}

char *translate_op_c_type(MessageT__CType c_type) {
    switch (c_type) {
        case MESSAGE_T__C_TYPE__CT_BAD: return "CT_BAD";
        case MESSAGE_T__C_TYPE__CT_DATA: return "CT_DATA";
        case MESSAGE_T__C_TYPE__CT_MARCA: return "CT_MARCA";
        case MESSAGE_T__C_TYPE__CT_YEAR: return "CT_YEAR";
        case MESSAGE_T__C_TYPE__CT_MODEL: return "CT_MODEL";
        case MESSAGE_T__C_TYPE__CT_RESULT: return "CT_RESULT";
        case MESSAGE_T__C_TYPE__CT_LIST: return "CT_LIST";
        default: return "UNKNOWN_CT";
    }
}

char *translate_marca(Marca marca) {
    switch (marca) {
        case MARCA__MARCA_TOYOTA: return "Toyota";
        case MARCA__MARCA_BMW: return "Bmw";
        case MARCA__MARCA_RENAULT: return "Renault";
        case MARCA__MARCA_AUDI: return "Audi";
        case MARCA__MARCA_MERCEDES: return "Mercedes";
        default: return "UNKNOWN_MARCA";
    }
}

char *make_client_addr_port(int sockfd) {
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len) < 0) {
        return NULL;
    }

    char ip[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &local_addr.sin_addr, ip, sizeof(ip))) {
        return NULL;
    }

    uint16_t port = ntohs(local_addr.sin_port);
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    size_t len = strlen(ip) + 1 + strlen(port_str) + 1;
    char *addr_port = malloc(len);
    if (!addr_port) {
        return NULL;
    }

    snprintf(addr_port, len, "%s:%s", ip, port_str);
    return addr_port;
}

void write_log(int timestamp, const char *clientAddressPort, const char *eventType, MessageT__Opcode opcode, MessageT__CType c_type, Data *argument) {
    pthread_mutex_lock(&log_mutex);

    FILE *log_file = fopen("server.log", "a");
    if (log_file == NULL) {
        perror("Failed to open/create log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    fprintf(log_file, "%d %s %s", timestamp, clientAddressPort, eventType);

    if (opcode != 0)
        fprintf(log_file, " %s %s", translate_op_code(opcode), translate_op_c_type(c_type));

    if (argument) {
        if (argument->marca) {
            fprintf(log_file, " %s", translate_marca(argument->marca));
        }
        if (argument->modelo) {
            fprintf(log_file, " %s", argument->modelo);
        }
        if (argument->ano) {
            fprintf(log_file, " %d", argument->ano);
        }
    }
    fprintf(log_file, "\n");


    fclose(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

int get_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}