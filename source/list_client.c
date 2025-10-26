#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../include/client_stub.h"
#include "../include/data.h"

void processa_comando(struct rlist_t *rlist, char *linha) {
    char comando[17]; // o maior comando vai até 16 / isto vai guardar o comando
    char argumento[239]; // isto vai guardar o argumento (se houver)
    int args = sscanf(linha, "%17s %239[^\n]", comando, argumento); // separa o comando do argumento e guarda quantos argumentos
                                                                                                                    //foram lidos

    if(strcmp(linha, "add") == 0){
        if(args != 5){
            printf("Erro: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
            return
        }

        struct data_t carro; // cria uma struct carro TODO verifefeiicia indicvidual
        carro.modelo = argumento[0];
        carro.ano = atoi(argumento[1]);
        carro.preco = atof(argumento[2]);
        carro.marca = atoi(argumento[3]);
        carro.combustivel = atoi(argumento[4]);
        if(rlist_add(rlist, &carro) == 0){
            printf("Carro %s adicionado com sucesso.\n", argumento);
        }
        
        else{
            printf("Erro: Não foi possivel adicionar o carro %s.\n", argumento);
            return;
        }        
    }

    else if(strcmp(linha, "remove") == 0){
        if(args != 1){
            printf("Erro: remove <model>.\n");
            return;
        }
        else if(rlist_remove_by_model(rlist, argumento[0]) == 0){
            printf("Carro do modelo %s removido com sucesso.\n", argumento[0]);
            return;
        }
        else{
            printf("Erro: modelo %s não encontrado ou erro ao remover.\n", argumento[0]);
            return;
        }
    }

    //TODO é para dar ao user o quê?? o nome de cada carro? Ou todas as informações
    else if(strcmp(linha, "get_by_year") == 0){
        if(args != 1){
            printf("Erro: get_by_year <year>.\n");
            return;
        }
        struct data_t **carros = rlist_get_by_year(rlist, atoi(argumento[0]));
        if(carros == NULL){     
            printf("Erro: Nenhum carro encontrado para o ano %s.\n", argumento[0]);
            return;
        }
        for(int i = 0; carros[i] != NULL; i++){
            print_car(carros[i]);
            free(carros[i]->modelo);
            free(carros[i]);
        }
        free(carros);
    }

    else if(strcmp(linha, "get_by_marca") == 0){
        if(args != 1){
            printf("Erro: get_by_marca <marca>.\n");
            return;
        }
        struct data_t **carros = rlist_get_by_marca(rlist, argumento[0]);
        if(carros == NULL){
            printf("Erro: Nenhum carro encontrado para a marca %s.\n", argumento[0]);
            return;
        }
        for(int i = 0; carros[i] != NULL; i++){
            print_car(carros[i]);
            free(carros[i]->modelo);
            free(carros[i]);
        }
        free(carros);
    }

    else if(strcmp(linha, "get_list_by_year") == 0){
        if(args != 1){
            printf("Erro: get_list_by_year <year>.\n");
        }
        struct data_t **carros = rlist_get_list_by_year(rlist, atoi(argumento[0]));
        if(carros == NULL){
            printf("Erro: Nenhum carro encontrado para o ano %s.\n", argumento)[0];
            return;
        }
        for(int i = 0; carros[i] != NULL; i++){
            print_car(carros[i]);
        }
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
        char **modelos = rlist_get_model_list(rlist);
        if(modelos == NULL){
            printf("Erro: Nenhum modelo encontrado.\n");
            return;
        }
        for(int i = 0; modelos[i] != NULL; i++){
            print_car(modelos[i]);
        }
    }

    else if(strcmp(linha, "help") == 0 ){
        printf("Comandos disponíveis: \n");
        printf("  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
        printf("  remove <modelo>\n");
        printf("  get_by_marca <marca>\n");
        printf("  get_by_year <ano>\n");
        printf("  get_list_by_year <year>\n");
        printf("  size\n");
        printf("  get_model_list\n");
        printf("  help\n");
        printf("  quit\n\n");
        return;
    }
}

// Função auxiliar para imprimir informações do carro
void print_car(struct data_t *car) {
    if (!car) return;
    
    const char *marcas[] = {"Toyota", "Ford", "Mercedes", "BMW", "Audi"};
    const char *combustiveis[] = {"Gasolina", "Diesel", "Eletrico", "Hibrido"};
    
    printf("Modelo: %s\n", car->modelo ? car->modelo : "N/A");
    printf("  Ano: %d\n", car->ano);
    printf("  Preço: %.2f\n", car->preco);
    printf("  Marca: %s (%d)\n", 
           (car->marca >= 0 && car->marca <= 4) ? marcas[car->marca] : "Desconhecida",
           car->marca);
    printf("  Combustível: %s (%d)\n\n",
           (car->combustivel >= 0 && car->combustivel <= 3) ? combustiveis[car->combustivel] : "Desconhecido",
           car->combustivel);
}

int main (int argc, char *argv[]) {
    // ignorar sinais SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    if(argc != 2){
        fprintf(stderr, "Erro: Use %s <server> <port>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s localhost:8080\n", argv[0]);

        return -1;
    }
    struct rlist_t *rlist = rlist_connect(argv[1]); // conecta ao servidor
    if(rlist == NULL){
        fprintf(stderr, "Erro: Não foi possivel conectar ao servidor %s\n", argv[1]);
        return -1;
    }
    printf("Conexão ao servidor %s estabelecida com sucesso. \n ", argv[1]);

    // Todos os comandos disponíveis
    printf("Comandos disponíveis: \n");
    printf("  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
    printf("  remove <modelo>\n");
    printf("  get_by_marca <marca>\n");
    printf("  get_by_year <ano>\n");
    printf("  get_list_by_year <year>\n");
    printf("  size\n");
    printf("  get_model_list\n");
    printf("  help\n");
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