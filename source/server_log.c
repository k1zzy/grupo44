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

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

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
        fprintf(log_file, " %d %d", opcode, c_type);

    if (argument) {
        if (argument->marca) {
            fprintf(log_file, " %d", argument->marca);
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