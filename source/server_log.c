#include <stdio.h>

void write_log(int timestamp, const char *clientPort, const char *eventType,
               const char *operation, const char *contentType, const char *argument) {
    FILE *log_file = fopen("server_log.txt", "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }

    fprintf(log_file, "%d %s %s", timestamp, clientPort, eventType);

    if (operation)   fprintf(log_file, " %s", operation);
    if (contentType) fprintf(log_file, " %s", contentType);
    if (argument)    fprintf(log_file, " %s", argument);

    fprintf(log_file, "\n");
    fclose(log_file);
}