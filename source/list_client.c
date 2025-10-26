#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../include/client_stub.h"
#include "../include/data.h"


void processa_comando(struct rlist_t *rlist, char *linha){
    char comando[17]; // o maior comando vai até 16 / isto vai guardar o comando
    char argumento[239]; // isto vai guardar o argumento (se houver)
    int args = sscanf(linha, "%17s %239[^\n]", comando, argumento); // separa o comando do argumento e guarda quantos argumentos
                                                                                                                    //foram lidos

    if(strcmp(linha, "add") == 0){
        if(args != 1){
            printf("Erro: add <data>.\n");
        }
        //TODO
    }

    else if(strcmp(linha, "remove") == 0){
        if(args != 1){
            printf("Erro: remove <model>.\n");
        }
        else if(rlist_remove_by_model(rlist, argumento) == 0){
            printf("Carro do modelo %s removido com sucesso.\n", argumento);
        }

        printf("Erro: modelo %s não encontrado ou erro ao remover.\n", argumento);
    }

    //TODO é para dar ao user o quê?? o nome de cada carro? Ou todas as informações
    else if(strcmp(linha, "get_by_year") == 0){
        if(args != 1){
            printf("Erro: get_by_year <year>.\n");
        }
        struct data_t **carros = rlist_get_by_year(rlist, atoi(argumento));
        if(carros == NULL){
            printf("Erro: Nenhum carro encontrado para o ano %s.\n", argumento);
            return;
        }
        for(int i = 0; carros[i] != NULL; i++){

        }

        //TODO
    }

    else if(strcmp(linha, "get_by_marca") == 0){
        if(args != 1){
            printf("Erro: get_by_marca <marca>.\n");
        }
        //TODO
    }

    else if(strcmp(linha, "get_list_by_year") == 0){
        if(args != 1){
            printf("Erro: get_list_by_year <year>.\n");
        }
        //TODO
    }
    // tamanho da lista do servidor
    else if(strcmp(linha, "size") == 0){
        if(args != 0){
            printf("Erro: size não recebe argumentos.\n");
        }
        int size = rlist_size(rlist);
        printf("Tamanho da lista: %d\n", size);
    }

    else if(strcmp(linha, "get_model_list") == 0){
        if(args != 0){
            printf("Erro: get_model_list não recebe argumentos.\n");
        }
        //TODO
    }

    else if(strcmp(linha, "quit") == 0){
        printf("Para sair, use 'quit' sozinho.\n");
    }
}

int main (int argc, char *argv[]) {
    // ignorar sinais SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    if(argc != 2){
        fprintf(stderr, "Erro: Use %s <server> <port>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s localhost:8080\n", argv[0]);

        exit(EXIT_FAILURE);
    }
    struct rlist_t *rlist = rlist_connect(argv[1]); // conecta ao servidor
    if(rlist == NULL){
        fprintf(stderr, "Erro: Não foi possivel conectar ao servidor %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    printf("Conexão ao servidor %s estabelecida com sucesso. \n ", argv[1]);

    // Todos os comando
    printf("Comandos disponíveis: \n");
    printf("  add <data>\n");
    printf("  remove <modelo>\n");
    printf("  get_by_year <ano>\n");
    printf("  get_by_marca <marca>\n");
    printf("  get_list_by_year <year>\n");
    printf("  size\n");
    printf("  get_model_list\n");
    printf("  quit\n\n");

    char linha[256];

    while(1){
        printf("- ");
        fflush(stdout); // para que o prompt apareça logo
        // remove o '\n do final do user input
        linha[strcspn(linha, "\n")] = '\0';
        // caso: 'quit'
        if(strcmp(linha, "quit") == 0){
            rlist_disconnect(rlist);
            printf("A desligar... .\n");
            break;
        }

        if(strlen(linha) == 0){
            continue; // ignora linhas vazias
        }

        // executa o comando
        processa_comando(rlist, linha);
    }

    
    
};