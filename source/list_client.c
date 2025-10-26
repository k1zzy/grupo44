#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../include/client_stub.h"
#include "../include/data.h"

// variavel global para controlar desconexão
static struct rlist_t *g_rlist = NULL;

// handler de sinais
void sig_term_handler(int signum) {
    (void)signum; // suprime o warning no make
    printf("\nDesconectando...\n");
    if (g_rlist) {
        rlist_disconnect(g_rlist);
        g_rlist = NULL;
    }
    exit(0);
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

void processa_comando(struct rlist_t *rlist, char *linha) {
    const char *delim = " \t\r\n";
    char *comando = strtok(linha, delim);
    
    if (!comando) {
        return;
    }

    if(strcmp(comando, "add") == 0){
        char *modelo = strtok(NULL, delim);
        char *ano_str = strtok(NULL, delim);
        char *preco_str = strtok(NULL, delim);
        char *marca_str = strtok(NULL, delim);
        char *combustivel_str = strtok(NULL, delim);
        
        if(!modelo || !ano_str || !preco_str || !marca_str || !combustivel_str){
            printf("Erro: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
            return;
        }

        struct data_t carro;
        carro.modelo = modelo;
        carro.ano = atoi(ano_str);
        carro.preco = atof(preco_str);
        carro.marca = atoi(marca_str);
        carro.combustivel = atoi(combustivel_str);
        
        if(rlist_add(rlist, &carro) == 0){
            printf("Carro adicionado com sucesso.\n");
        }
        else{
            printf("Erro: Não foi possivel adicionar o carro.\n");
        }        
    }

    else if(strcmp(comando, "remove") == 0){
        char *modelo = strtok(NULL, delim);
        
        if(!modelo){
            printf("Erro: remove <modelo>.\n");
            return;
        }
        
        if(rlist_remove_by_model(rlist, modelo) == 0){
            printf("Carro do modelo %s removido com sucesso.\n", modelo);
        }
        else{
            printf("Erro: modelo %s não encontrado ou erro ao remover.\n", modelo);
        }
    }

    else if(strcmp(comando, "get_by_year") == 0){
        char *ano_str = strtok(NULL, delim);
        
        if(!ano_str){
            printf("Erro: get_by_year <ano>.\n");
            return;
        }
        
        int ano = atoi(ano_str);
        struct data_t **carros = rlist_get_by_year(rlist, ano);
        
        if(carros == NULL){     
            printf("Erro: Nenhum carro encontrado para o ano %d.\n", ano);
            return;
        }
        
        for(int i = 0; carros[i] != NULL; i++){
            print_car(carros[i]);
            free(carros[i]->modelo);
            free(carros[i]);
        }
        free(carros);
    }

    else if(strcmp(comando, "get_by_marca") == 0){
        char *marca_str = strtok(NULL, delim);
        
        if(!marca_str){
            printf("Erro: get_by_marca <marca>.\n");
            return;
        }
        
        int marca = atoi(marca_str);
        struct data_t *carro = rlist_get_by_marca(rlist, marca);
        
        if(carro == NULL){
            printf("Erro: Nenhum carro encontrado para a marca %d.\n", marca);
            return;
        }
        
        print_car(carro);
        free(carro->modelo);
        free(carro);
    }

    else if(strcmp(comando, "get_list_ordered_by_year") == 0){
        struct data_t **carros = rlist_get_by_year(rlist, -1);
        
        if(carros == NULL){
            printf("Erro: Nenhum carro encontrado.\n");
            return;
        }
        
        for(int i = 0; carros[i] != NULL; i++){
            print_car(carros[i]);
            free(carros[i]->modelo);
            free(carros[i]);
        }
        free(carros);
    }
    // tamanho da lista do servidor
    else if(strcmp(comando, "size") == 0){
        int size = rlist_size(rlist);
        if(size == -1){
            printf("Erro ao obter tamanho da lista.\n");
        }
        else{
            printf("Tamanho da lista: %d\n", size);
        }
    }

    else if(strcmp(comando, "get_model_list") == 0){
        char **modelos = rlist_get_model_list(rlist);
        
        if(modelos == NULL){
            printf("Erro: Nenhum modelo encontrado.\n");
            return;
        }
        
        for(int i = 0; modelos[i] != NULL; i++){
            printf("Modelo: %s\n", modelos[i]);
        }
        rlist_free_model_list(modelos);
    }

    else if(strcmp(comando, "help") == 0){
        printf("Comandos disponíveis: \n");
        printf("  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
        printf("  remove <modelo>\n");
        printf("  get_by_marca <marca:0-4>\n");
        printf("  get_by_year <ano>\n");
        printf("  get_list_ordered_by_year\n");
        printf("  size\n");
        printf("  get_model_list\n");
        printf("  help\n");
        printf("  quit\n\n");
    }
    
    else{
        printf("Comando inválido. Digite 'help' para ver os comandos disponíveis.\n");
    }
}

int main (int argc, char *argv[]) {
    // configurar handlers de sinais
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sig_term_handler;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);   // CTRL+C
    sigaction(SIGTERM, &act, NULL);  // kill command
    
    // ignorar sinais SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    
    if(argc != 2){
        fprintf(stderr, "Argumentos inválidos!\n");
        fprintf(stderr, "Uso: %s <server>:<port>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s localhost:8080\n", argv[0]);
        return -1;
    }
    
    struct rlist_t *rlist = rlist_connect(argv[1]);
    if(rlist == NULL){
        fprintf(stderr, "Erro: Não foi possivel conectar ao servidor %s\n", argv[1]);
        return -1;
    }
    
    g_rlist = rlist; // Guardar globalmente para handler de sinais
    
    printf("Ligado a %s\n", argv[1]);

    // Mostrar comandos disponíveis
    printf("Comandos disponíveis: \n");
    printf("  add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
    printf("  remove <modelo>\n");
    printf("  get_by_marca <marca:0-4>\n");
    printf("  get_by_year <ano>\n");
    printf("  get_list_ordered_by_year\n");
    printf("  size\n");
    printf("  get_model_list\n");
    printf("  help\n");
    printf("  quit\n\n");

    char linha[1024];

    while(1){
        printf("Command: ");
        fflush(stdout);
        
        if(!fgets(linha, sizeof(linha), stdin)){
            printf("\nEOF/erro em stdin. A terminar...\n");
            break;
        }
        
        // remove o '\n' do final
        linha[strcspn(linha, "\n")] = '\0';
        
        // ignora linhas vazias
        if(strlen(linha) == 0){
            continue;
        }
        
        // caso: 'quit'
        if(strcmp(linha, "quit") == 0){
            break;
        }

        // executa o comando
        processa_comando(rlist, linha);
    }
    
    rlist_disconnect(rlist);
    g_rlist = NULL;
    return 0;
} 