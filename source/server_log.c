/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */
#include "../include/server_log.h"
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(int timestamp, const char *clientPort, const char *eventType,
               const char *operation, const char *contentType, const char *argument) {
    pthread_mutex_lock(&log_mutex);

    FILE *log_file = fopen("server_log.txt", "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    fprintf(log_file, "%d %s %s", timestamp, clientPort, eventType);

    if (operation)   fprintf(log_file, " %s", operation);
    if (contentType) fprintf(log_file, " %s", contentType);
    if (argument)    fprintf(log_file, " %s", argument);

    fprintf(log_file, "\n");
    fclose(log_file);

    pthread_mutex_unlock(&log_mutex);
}