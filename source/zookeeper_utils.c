#include "../include/zookeeper_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global watcher for connection state
void connection_watcher(zhandle_t *zh, int type, int state, const char *path,
                        void *watcherCtx) {
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      printf("Connected to ZooKeeper server!\n");
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
      printf("ZooKeeper session expired!\n");
    }
  }
}

zhandle_t *zookeeper_connect(const char *host_port) {
  zhandle_t *zh = zookeeper_init(host_port, connection_watcher, 2000, 0, 0, 0);
  if (!zh) {
    fprintf(stderr, "Error connecting to ZooKeeper server at %s\n", host_port);
    return NULL;
  }
  return zh;
}

int zookeeper_create_chain_node(zhandle_t *zh) {
  if (!zh)
    return -1;

  int ret =
      zoo_create(zh, "/chain", NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
  if (ret == ZOK) {
    printf("Created /chain node\n");
    return 0;
  } else if (ret == ZNODEEXISTS) {
    // Node already exists, which is fine
    return 0;
  } else {
    fprintf(stderr, "Error creating /chain node: %d\n", ret);
    return -1;
  }
}

int zookeeper_create_server_node(zhandle_t *zh, const char *server_ip_port,
                                 char *node_path_buffer, int buffer_len) {
  if (!zh || !server_ip_port || !node_path_buffer)
    return -1;

  char path[512];
  // Create an ephemeral sequential node
  int ret =
      zoo_create(zh, "/chain/node-", server_ip_port, strlen(server_ip_port) + 1,
                 &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE,
                 node_path_buffer, buffer_len);

  if (ret != ZOK) {
    fprintf(stderr, "Error creating server node: %d\n", ret);
    return -1;
  }

  printf("Created server node: %s\n", node_path_buffer);
  return 0;
}

int zookeeper_get_children(zhandle_t *zh, struct String_vector *children,
                           watcher_fn watcher) {
  if (!zh || !children)
    return -1;

  // Use zoo_wget_children to specify the watcher function
  int ret = zoo_wget_children(zh, "/chain", watcher, NULL, children);

  if (ret != ZOK) {
    fprintf(stderr, "Error getting children of /chain: %d\n", ret);
    return -1;
  }

  return 0;
}
