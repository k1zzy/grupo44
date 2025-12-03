/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira
 * (61879)
 */

#include "../include/client_stub.h"
#include "../include/data.h"
#include "../include/zookeeper_utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// Variáveis globais
static struct rlist_t *head_server = NULL;
static struct rlist_t *tail_server = NULL;
static zhandle_t *zh = NULL;

// handler de sinais
void sig_term_handler(int signum) {
  (void)signum; // suprime o warning no make
  printf("\nDesconectando...\n");
  if (head_server)
    rlist_disconnect(head_server);
  if (tail_server && tail_server != head_server)
    rlist_disconnect(tail_server);
  if (zh)
    zookeeper_close(zh);
  exit(0);
}

// Função para comparar strings (para qsort)
int compare_strings(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

// Callback para mudanças nos filhos de /chain
void chain_watcher(zhandle_t *zzh, int type, int state, const char *path,
                   void *watcherCtx) {
  if (type == ZOO_CHILD_EVENT) {
    printf("Detected change in /chain children. Updating Head/Tail...\n");

    struct String_vector children;
    if (zookeeper_get_children(zzh, &children, chain_watcher) != 0) {
      fprintf(stderr, "Error getting children in watcher\n");
      return;
    }

    if (children.count == 0) {
      printf("No servers available!\n");
      if (head_server) {
        rlist_disconnect(head_server);
        head_server = NULL;
      }
      if (tail_server) {
        rlist_disconnect(tail_server);
        tail_server = NULL;
      }
      return;
    }

    // Ordenar os filhos
    qsort(children.data, children.count, sizeof(char *), compare_strings);

    // Head é o primeiro, Tail é o último
    char head_path[512];
    char tail_path[512];
    snprintf(head_path, sizeof(head_path), "/chain/%s", children.data[0]);
    snprintf(tail_path, sizeof(tail_path), "/chain/%s",
             children.data[children.count - 1]);

    char head_addr[1024];
    char tail_addr[1024];
    int len = sizeof(head_addr);

    // Obter endereço do Head
    if (zoo_get(zzh, head_path, 0, head_addr, &len, NULL) == ZOK) {
      head_addr[len] = '\0';
      printf("New Head: %s\n", head_addr);
      // Reconectar se mudou (simplificado: reconecta sempre ou verifica se é o
      // mesmo) Para simplificar, vamos assumir que o rlist_connect lida com
      // isso ou reconectamos
      if (head_server)
        rlist_disconnect(head_server);
      head_server = rlist_connect(head_addr);
    }

    // Obter endereço do Tail
    len = sizeof(tail_addr);
    if (zoo_get(zzh, tail_path, 0, tail_addr, &len, NULL) == ZOK) {
      tail_addr[len] = '\0';
      printf("New Tail: %s\n", tail_addr);
      if (tail_server)
        rlist_disconnect(tail_server);
      tail_server = rlist_connect(tail_addr);
    }
  }
}

// Função auxiliar para imprimir informações do carro
void print_car(struct data_t *car) {
  if (!car)
    return;

  const char *marcas[] = {"Toyota", "Ford", "Mercedes", "BMW", "Audi"};
  const char *combustiveis[] = {"Gasolina", "Diesel", "Eletrico", "Hibrido"};

  printf("Modelo: %s\n", car->modelo ? car->modelo : "N/A");
  printf("  Ano: %d\n", car->ano);
  printf("  Preço: %.2f\n", car->preco);
  printf("  Marca: %s (%d)\n",
         (car->marca >= 0 && car->marca <= 4) ? marcas[car->marca]
                                              : "Desconhecida",
         car->marca);
  printf("  Combustível: %s (%d)\n\n",
         (car->combustivel >= 0 && car->combustivel <= 3)
             ? combustiveis[car->combustivel]
             : "Desconhecido",
         car->combustivel);
}

void processa_comando(char *linha) {
  const char *delim = " \t\r\n";
  char *comando = strtok(linha, delim);

  if (!comando) {
    return;
  }

  // Operações de escrita -> Head
  if (strcmp(comando, "add") == 0) {
    if (!head_server) {
      printf("Error: No connection to Head server.\n");
      return;
    }

    char *modelo = strtok(NULL, delim);
    char *ano_str = strtok(NULL, delim);
    char *preco_str = strtok(NULL, delim);
    char *marca_str = strtok(NULL, delim);
    char *combustivel_str = strtok(NULL, delim);

    if (!modelo || !ano_str || !preco_str || !marca_str || !combustivel_str) {
      printf(
          "Erro: add <modelo> <ano> <preco> <marca:0-4> <combustivel:0-3>\n");
      return;
    }

    int ano = atoi(ano_str);
    float preco = atof(preco_str);
    int marca = atoi(marca_str);
    int combustivel = atoi(combustivel_str);

    // verificações
    if (ano < 1886 || ano > 2100) {
      printf("Erro: ano deve ser entre 1886 e 2100.\n");
      return;
    }
    if (preco < 0) {
      printf("Erro: preço deve ser um valor positivo.\n");
      return;
    }
    if (marca < 0 || marca > 4) {
      printf("Erro: marca deve ser um valor entre 0 e 4.\n");
      return;
    }
    if (combustivel < 0 || combustivel > 3) {
      printf("Erro: combustível deve ser um valor entre 0 e 3.\n");
      return;
    }

    // construir o carro
    struct data_t carro;
    carro.modelo = modelo;
    carro.ano = ano;
    carro.preco = preco;
    carro.marca = marca;
    carro.combustivel = combustivel;

    // adicionar um carro ao servidor HEAD
    if (rlist_add(head_server, &carro) == 0) {
      printf("Carro adicionado com sucesso.\n");
    } else {
      printf("Erro: Não foi possivel adicionar o carro.\n");
    }
  }
  // remover um carro de um modelo dado -> Head
  else if (strcmp(comando, "remove") == 0) {
    if (!head_server) {
      printf("Error: No connection to Head server.\n");
      return;
    }

    char *modelo = strtok(NULL, delim);

    if (!modelo) {
      printf("Erro: remove <modelo>.\n");
      return;
    }

    if (rlist_remove_by_model(head_server, modelo) == 0) {
      printf("Carro do modelo %s removido com sucesso.\n", modelo);
    } else {
      printf("Erro: modelo %s não encontrado ou erro ao remover.\n", modelo);
    }
  }
  // Operações de leitura -> Tail
  else if (strcmp(comando, "get_by_year") == 0) {
    if (!tail_server) {
      printf("Error: No connection to Tail server.\n");
      return;
    }

    char *ano_str = strtok(NULL, delim);
    if (!ano_str) {
      printf("Erro: get_by_year <ano>.\n");
      return;
    }

    int ano = atoi(ano_str);
    struct data_t **carros = rlist_get_by_year(tail_server, ano);

    if (carros == NULL) {
      printf("Erro: Nenhum carro encontrado para o ano %d.\n", ano);
      return;
    }
    // imprime os carros encontrados
    for (int i = 0; carros[i] != NULL; i++) {
      print_car(carros[i]);
      free(carros[i]->modelo); // libertar memoria alocada para o modelo
      free(carros[i]);         // libertar memoria alocada para a struct data_t
    }
    free(carros);
  }
  // obter carro por marca -> Tail
  else if (strcmp(comando, "get_by_marca") == 0) {
    if (!tail_server) {
      printf("Error: No connection to Tail server.\n");
      return;
    }

    char *marca_str = strtok(NULL, delim);

    if (!marca_str) {
      printf("Erro: get_by_marca <marca>.\n");
      return;
    }

    int marca = atoi(marca_str);
    struct data_t *carro = rlist_get_by_marca(tail_server, marca);

    if (carro == NULL) {
      printf("Erro: Nenhum carro encontrado para a marca %d.\n", marca);
      return;
    }

    print_car(carro);
    free(carro->modelo); // libertar memoria alocada para o modelo
    free(carro);         // libertar memoria alocada para a struct data_t
  }
  // obter lista ordenada de uma ano específico -> Tail
  else if (strcmp(comando, "get_list_ordered_by_year") == 0) {
    if (!tail_server) {
      printf("Error: No connection to Tail server.\n");
      return;
    }

    struct data_t **carros = rlist_get_by_year(tail_server, -1);

    if (carros == NULL) {
      printf("Erro: Nenhum carro encontrado.\n");
      return;
    }
    // imprime os carros encontrados
    for (int i = 0; carros[i] != NULL; i++) {
      print_car(carros[i]);
      free(carros[i]->modelo); // libertar memoria alocada para o modelo
      free(carros[i]);         // libertar memoria alocada para a struct data_t
    }
    free(carros);
  }
  // tamanho da lista do servidor -> Tail
  else if (strcmp(comando, "size") == 0) {
    if (!tail_server) {
      printf("Error: No connection to Tail server.\n");
      return;
    }

    int size = rlist_size(tail_server);
    if (size == -1) {
      printf("Erro ao obter tamanho da lista.\n");
    } else {
      printf("Tamanho da lista: %d\n", size);
    }
  }
  // obter lista de modelos -> Tail
  else if (strcmp(comando, "get_model_list") == 0) {
    if (!tail_server) {
      printf("Error: No connection to Tail server.\n");
      return;
    }

    char **modelos = rlist_get_model_list(tail_server);

    if (modelos == NULL) {
      printf("Erro: Nenhum modelo encontrado.\n");
      return;
    }
    // imprime os modelos encontrados
    for (int i = 0; modelos[i] != NULL; i++) {
      printf("Modelo: %s\n", modelos[i]);
    }
    rlist_free_model_list(
        modelos); // libertar memoria alocada para a lista de modelos
  }
  // imprime todos os comandos do sistema
  else if (strcmp(comando, "help") == 0) {
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

  else {
    printf("Comando inválido. Digite 'help' para ver os comandos "
           "disponíveis.\n"); // quando um comando não é reconhecido
  }
}

int main(int argc, char *argv[]) {
  // configurar handlers de sinais
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_handler = sig_term_handler;
  sigemptyset(&act.sa_mask);
  sigaction(SIGINT, &act, NULL);  // CTRL+C
  sigaction(SIGTERM, &act, NULL); // kill command

  // ignorar sinais SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  if (argc != 2) {
    fprintf(stderr, "Argumentos inválidos!\n");
    fprintf(stderr, "Uso: %s <zookeeper_ip:port>\n", argv[0]);
    return -1;
  }

  // Conectar ao ZooKeeper
  zh = zookeeper_connect(argv[1]);
  if (!zh) {
    fprintf(stderr, "Erro ao conectar ao ZooKeeper\n");
    return -1;
  }

  // Obter filhos e configurar Head/Tail inicial
  chain_watcher(zh, ZOO_CHILD_EVENT, 0, NULL, NULL);

  printf("Ligado ao ZooKeeper em %s\n", argv[1]);

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
  // loop principal de leitura de comandos
  while (1) {
    printf("Command: ");
    fflush(stdout);

    if (!fgets(linha, sizeof(linha), stdin)) {
      printf("\nEOF/erro em stdin. A terminar...\n");
      break;
    }

    // remove o '\n' do final para facilitar comparações
    linha[strcspn(linha, "\n")] = '\0';

    // ignora linhas vazias
    if (strlen(linha) == 0) {
      continue;
    }

    // caso: 'quit'
    if (strcmp(linha, "quit") == 0) {
      break;
    }

    // executa o comando
    processa_comando(linha);
  }

  if (head_server)
    rlist_disconnect(head_server);
  if (tail_server && tail_server != head_server)
    rlist_disconnect(tail_server);
  zookeeper_close(zh);
  return 0;
}