#ifndef ZOOKEEPER_UTILS_H
#define ZOOKEEPER_UTILS_H

#include <zookeeper/zookeeper.h>

/* workaround declaracoes falta algumas versoes zk */
extern int zoo_create(zhandle_t *zh, const char *path, const char *value,
                      int valuelen, const struct ACL_vector *acl, int flags,
                      char *path_buffer, int path_buffer_len);

extern int zoo_wget(zhandle_t *zh, const char *path, watcher_fn watcher,
                    void *watcherCtx, char *buffer, int *buffer_len,
                    struct Stat *stat);

extern int zoo_wget_children(zhandle_t *zh, const char *path,
                             watcher_fn watcher, void *watcherCtx,
                             struct String_vector *strings);

extern int zoo_get(zhandle_t *zh, const char *path, int watch, char *buffer,
                   int *buffer_len, struct Stat *stat);

/**
 * conecta zookeeper
 * @param host_port endereco server zookeeper (ex 127.0.0.1:2181)
 * @return handle ligacao zk ou null falha
 */
// liga ao zookeeper
zhandle_t *zookeeper_connect(const char *host_port);

/**
 * cria no root chain (/chain) se nao existir
 * @param zh handle zk
 * @return 0 sucesso -1 falha
 */
// cria o no principal
int zookeeper_create_chain_node(zhandle_t *zh);

/**
 * cria no efemero sequencial para server em /chain
 * @param zh handle zk
 * @param server_ip_port string ip:port server para guardar no no
 * @param node_path_buffer buffer para guardar path no criado (ex /chain/node-0000000001)
 * @param buffer_len tamanho buffer
 * @return 0 sucesso -1 falha
 */
// regista o servidor
int zookeeper_create_server_node(zhandle_t *zh, const char *server_ip_port,
                                 char *node_path_buffer, int buffer_len);

/**
 * obtem lista filhos /chain e mete watcher
 * @param zh handle zk
 * @param children estrutura para guardar lista filhos
 * @param watcher_ctx contexto para funcao watcher
 * @return 0 sucesso -1 falha
 */
// ve quem sao os filhos
int zookeeper_get_children(zhandle_t *zh, struct String_vector *children,
                           watcher_fn watcher);

#endif // ZOOKEEPER_UTILS_H
