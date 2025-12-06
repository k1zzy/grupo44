/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira
 * (61879)
 */

#include "../include/client_stub.h" // Para rlist_connect e rlist_disconnect
#include "../include/list_skel.h"
#include "../include/network_server.h"
#include "../include/zookeeper_utils.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Helper to get local IP
int get_local_ip(char *buffer, size_t buflen) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return -1;

  struct sockaddr_in serv;
  memset(&serv, 0, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = inet_addr("8.8.8.8"); // Google DNS
  serv.sin_port = htons(53);

  // No actual connection used, just route lookup
  if (connect(sock, (const struct sockaddr *)&serv, sizeof(serv)) < 0) {
    close(sock);
    return -1;
  }

  struct sockaddr_in local;
  socklen_t namelen = sizeof(local);
  if (getsockname(sock, (struct sockaddr *)&local, &namelen) < 0) {
    close(sock);
    return -1;
  }

  const char *p = inet_ntop(AF_INET, &local.sin_addr, buffer, buflen);
  close(sock);
  return (p != NULL) ? 0 : -1;
}

// Variáveis globais para gestão da replicação
static zhandle_t *zh = NULL;
static char my_node_path[512];
static char *next_server_addr = NULL;
static struct rlist_t *next_server_conn = NULL;
static int is_tail = 0;

void sig_term_handler(int signum) {
  (void)signum; // como nao se usa o parametro, suprime o warning no make
  printf("\nShutting down server...\n");
  network_server_request_shutdown();
  if (zh) {
    zookeeper_close(zh);
  }
  if (next_server_conn) {
    rlist_disconnect(next_server_conn);
  }
  if (next_server_addr) {
    free(next_server_addr);
  }
}

// Função para comparar strings (para qsort)
int compare_strings(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

// Callback para mudanças nos filhos de /chain
void chain_watcher(zhandle_t *zzh, int type, int state, const char *path,
                   void *watcherCtx) {
  (void)state;
  (void)path;
  (void)watcherCtx;
  if (type == ZOO_CHILD_EVENT) {
    printf("Detected change in /chain children. Updating topology...\n");

    struct String_vector children;
    if (zookeeper_get_children(zzh, &children, chain_watcher) != 0) {
      fprintf(stderr, "Error getting children in watcher\n");
      return;
    }

    // Ordenar os filhos lexicograficamente
    qsort(children.data, children.count, sizeof(char *), compare_strings);

    // Encontrar a minha posição
    int my_index = -1;
    char *my_node_name = strrchr(my_node_path, '/');
    if (my_node_name) {
      my_node_name++; // Avançar a barra
    } else {
      my_node_name = my_node_path;
    }

    for (int i = 0; i < children.count; i++) {
      if (strcmp(children.data[i], my_node_name) == 0) {
        my_index = i;
        break;
      }
    }

    if (my_index == -1) {
      fprintf(stderr, "Could not find my node in /chain children!\n");
      // Talvez o nosso nó tenha sido apagado?
    } else {
      // Verificar sucessor
      if (my_index < children.count - 1) {
        // Tenho sucessor
        char next_node_path[512];
        snprintf(next_node_path, sizeof(next_node_path), "/chain/%s",
                 children.data[my_index + 1]);

        char buffer[1024];
        int buffer_len = sizeof(buffer);
        int ret = zoo_get(zzh, next_node_path, 0, buffer, &buffer_len, NULL);

        if (ret == ZOK) {
          buffer[buffer_len] = '\0';
          printf("My successor is %s at %s\n", children.data[my_index + 1],
                 buffer);

          // Se o sucessor mudou, reconectar
          if (next_server_addr == NULL ||
              strcmp(next_server_addr, buffer) != 0) {
            if (next_server_conn) {
              rlist_disconnect(next_server_conn);
              next_server_conn = NULL;
            }
            if (next_server_addr)
              free(next_server_addr);
            next_server_addr = strdup(buffer);

            printf("Connecting to new successor: %s\n", next_server_addr);
            next_server_conn = rlist_connect(next_server_addr);
            if (!next_server_conn) {
              fprintf(stderr, "Failed to connect to successor %s\n",
                      next_server_addr);
            }

            // Atualizar no network_server (precisamos de uma função para isso)
            network_server_set_successor(next_server_conn);
          }
        }
        is_tail = 0;
      } else {
        // Sou a cauda
        printf("I am the TAIL.\n");
        if (next_server_conn) {
          rlist_disconnect(next_server_conn);
          next_server_conn = NULL;
        }
        if (next_server_addr) {
          free(next_server_addr);
          next_server_addr = NULL;
        }
        network_server_set_successor(NULL);
        is_tail = 1;
      }
    }

    // Libertar memória da lista de filhos
    // (A biblioteca ZK pode ter funções específicas para libertar
    // String_vector, mas geralmente é free no data e struct)
    // deallocate_String_vector(&children); // Se disponível, senão iterar e
    // free
  }
}

int main(int argc, char *argv[]) {
  // verificar argumentos
  if (argc != 3) {
    printf("Invalid arguments!\n");
    printf("Usage: list-server <zookeeper_ip:port> <server_port>\n");
    return -1;
  }

  char *zk_addr = argv[1];
  short port = (short)atoi(argv[2]);

  // se a porta for inválida
  if (port <= 1023) {
    printf("Bad port number\n");
    return -1;
  }

  // handlers de sinais
  struct sigaction act;
  memset(&act, 0, sizeof(act));      // meter a 0 por segurança
  act.sa_handler = sig_term_handler; // definir a função handler
  sigemptyset(&act.sa_mask);         // limpar a máscara de sinais
  sigaction(SIGINT, &act, NULL);     // CTRL+C
  sigaction(SIGTERM, &act, NULL);    // kill command

  // Ignorar SIGPIPE para não dar crash
  signal(SIGPIPE, SIG_IGN);

  // inicia o servidor de rede
  int listening_socket = network_server_init(port);
  if (listening_socket == -1) {
    printf("Error initializing network \n");
    return -1;
  }

  // abre a lista
  struct list_t *list = list_skel_init();
  if (!list) {
    printf("Error initializing list \n");
    network_server_close(listening_socket);
    return -1;
  }

  // Ligar ao ZooKeeper
  zh = zookeeper_connect(zk_addr);
  if (!zh) {
    printf("Error connecting to ZooKeeper\n");
    network_server_close(listening_socket);
    list_skel_destroy(list);
    return -1;
  }

  // Criar nó /chain se não existir
  zookeeper_create_chain_node(zh);

  // Obter o meu IP
  char my_ip[64];
  if (get_local_ip(my_ip, sizeof(my_ip)) != 0) {
    // Fallback to localhost if network fails
    strcpy(my_ip, "127.0.0.1");
    fprintf(stderr, "Failed to get real IP, using localhost fallback.\n");
  }

  char my_addr[256];
  snprintf(my_addr, sizeof(my_addr), "%s:%d", my_ip, port);

  // Criar o meu nó efémero sequencial
  if (zookeeper_create_server_node(zh, my_addr, my_node_path,
                                   sizeof(my_node_path)) != 0) {
    printf("Error creating server node in ZooKeeper\n");
    // cleanup
    return -1;
  }

  // Obter filhos e determinar topologia inicial
  struct String_vector children;
  if (zookeeper_get_children(zh, &children, chain_watcher) == 0) {
    // Ordenar e verificar
    qsort(children.data, children.count, sizeof(char *), compare_strings);

    // Encontrar a minha posição
    int my_index = -1;
    char *my_node_name = strrchr(my_node_path, '/');
    if (my_node_name)
      my_node_name++;
    else
      my_node_name = my_node_path;

    for (int i = 0; i < children.count; i++) {
      if (strcmp(children.data[i], my_node_name) == 0) {
        my_index = i;
        break;
      }
    }

    if (my_index > 0) {
      // Tenho antecessor, preciso de sincronizar estado
      char prev_node_path[512];
      snprintf(prev_node_path, sizeof(prev_node_path), "/chain/%s",
               children.data[my_index - 1]);

      char buffer[1024];
      int buffer_len = sizeof(buffer);
      int ret = zoo_get(zh, prev_node_path, 0, buffer, &buffer_len, NULL);

      if (ret == ZOK) {
        buffer[buffer_len] = '\0';
        printf("Syncing state from predecessor %s at %s\n",
               children.data[my_index - 1], buffer);

        struct rlist_t *prev_server = rlist_connect(buffer);
        if (prev_server) {
          // Obter lista completa
          struct data_t **all_cars =
              rlist_get_by_year(prev_server, -1); // -1 para obter todos
          if (all_cars) {
            for (int k = 0; all_cars[k] != NULL; k++) {
              list_add(list, all_cars[k]);
              data_destroy(all_cars[k]); // Libertar a cópia recebida
            }
            free(all_cars);
          }
          rlist_disconnect(prev_server);
          printf("State synchronization complete.\n");
        } else {
          fprintf(stderr, "Failed to connect to predecessor for sync!\n");
        }
      }
    }

    // Chamar o watcher logicamente para configurar o sucessor inicial
    chain_watcher(zh, ZOO_CHILD_EVENT, 0, NULL, NULL);
  }

  // main loop
  network_main_loop(listening_socket, list);

  // Aguardar que todas as threads terminem
  network_server_join_threads();

  // fechar servidor e destruir lista
  network_server_close(listening_socket);
  list_skel_destroy(list);

  return 0;
}
