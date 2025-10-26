#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// TODO: adicionar includes necessários

int main(int argc, char *argv[]) {
    (void)argc; // unused parameter
    (void)argv; // unused parameter
    
    // ignorar sinais SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    // TODO: implementar lógica do servidor
    
    return 0;
}
