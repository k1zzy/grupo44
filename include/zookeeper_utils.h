#ifndef ZOOKEEPER_UTILS_H
#define ZOOKEEPER_UTILS_H

#include <zookeeper/zookeeper.h>

/* Workaround for missing declarations in some ZK versions/configs */
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
 * Connects to the ZooKeeper server.
 * @param host_port The ZooKeeper server address (e.g., "127.0.0.1:2181").
 * @return A handle to the ZooKeeper connection, or NULL on failure.
 */
zhandle_t *zookeeper_connect(const char *host_port);

/**
 * Creates the root chain node (/chain) if it doesn't exist.
 * @param zh The ZooKeeper handle.
 * @return 0 on success, -1 on failure.
 */
int zookeeper_create_chain_node(zhandle_t *zh);

/**
 * Creates an ephemeral sequential node for a server under /chain.
 * @param zh The ZooKeeper handle.
 * @param server_ip_port The IP:Port string of the server to be stored in the
 * node.
 * @param node_path_buffer Buffer to store the created node path (e.g.,
 * "/chain/node-0000000001").
 * @param buffer_len Size of the buffer.
 * @return 0 on success, -1 on failure.
 */
int zookeeper_create_server_node(zhandle_t *zh, const char *server_ip_port,
                                 char *node_path_buffer, int buffer_len);

/**
 * Gets the list of children of /chain and sets a watcher.
 * @param zh The ZooKeeper handle.
 * @param children Structure to store the list of children.
 * @param watcher_ctx Context to be passed to the watcher function.
 * @return 0 on success, -1 on failure.
 */
int zookeeper_get_children(zhandle_t *zh, struct String_vector *children,
                           watcher_fn watcher);

#endif // ZOOKEEPER_UTILS_H
