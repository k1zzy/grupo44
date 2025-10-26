/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */

#include "../include/message-private.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int write_all(int sock, void *buf, int len) {
    char *buf_ptr = (char *)buf;
    int bytes_left = len;
    int bytes_written;
    
    // escrever até que todos os bytes sejam enviados
    while (bytes_left > 0) {
        bytes_written = write(sock, buf_ptr, bytes_left);
        
        if (bytes_written > 0) {
            buf_ptr += bytes_written;
            bytes_left -= bytes_written;
        } else if (errno != EINTR) { // se o erro nao for interrupção
            return -1;
        }
    }
    return len;
}

int read_all(int sock, void *buf, int len) {
    char *buf_ptr = (char *)buf;
    int bytes_left = len;
    int bytes_read;
    
    // ler até que todos os bytes sejam recebidos
    while (bytes_left > 0) {
        bytes_read = read(sock, buf_ptr, bytes_left);
        
        if (bytes_read > 0) {
            buf_ptr += bytes_read;
            bytes_left -= bytes_read;
        } else if (errno != EINTR) { // se o erro nao for interrupção
            return -1;
        }
    }
    return len;
}