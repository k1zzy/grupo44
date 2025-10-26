#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "../include/network_server.h"
#include "../include/list_skel.h"

void sig_term_handler(int signum) {
    (void)signum; // como nao se usa o parametro, suprime o warning no make
    printf("\nShutting down server...\n");
    network_server_request_shutdown();
}

int main(int argc, char *argv[]) {
    // verificar argumentos
    if (argc != 2) {
        printf("Invalid arguments!");
        printf("Usage: list-server <port>");
        return -1;
    }
    
    // porta para int
    short port = (short)atoi(argv[1]);
    
    // se a porta for inválida
    if (port <= 1023) {
        printf("Bad port number");
        return -1;
    }
    
    // handlers de sinais
    struct sigaction act;
    memset(&act, 0, sizeof(act));  // meter a 0 por segurança
    act.sa_handler = sig_term_handler; // definir a função handler
    sigemptyset(&act.sa_mask); // limpar a máscara de sinais
    sigaction(SIGINT, &act, NULL);  // CTRL+C
    sigaction(SIGTERM, &act, NULL); // kill command
    
    // Ignorar SIGPIPE
    signal(SIGPIPE, SIG_IGN); // para nao crashar
    
    // inicia o servidor de rede
    int listening_socket = network_server_init(port);
    if (listening_socket == -1) {
        printf("Error initializing network");
        return -1;
    }
    
    // abre a lista
    struct list_t *list = list_skel_init();
    if (!list) {
        printf("Error initializing list");
        network_server_close(listening_socket);
        return -1;
    }
    
    // main loop
    network_main_loop(listening_socket, list);
    
    // fechar servidor e destruir lista
    network_server_close(listening_socket);
    list_skel_destroy(list);
    
    return 0;
}
