# Grupo 44 - Projeto 2 SD
# Elementos:
# - Rodrigo Afonso Numero1
# - Guilherme Ramos Numero2
# - Miguel Ferreira 61879

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -g
PROTOC = protoc-c

# directories
SRC_DIR = source
OBJ_DIR = object
BIN_DIR = binary
LIB_DIR = lib
INC_DIR = include

# src files
CLIENT_SRCS = $(SRC_DIR)/list_client.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_client.c $(SRC_DIR)/sdmessage.pb-c.c
SERVER_SRCS = $(SRC_DIR)/list_server.c $(SRC_DIR)/network_server.c $(SRC_DIR)/list_skel.c $(SRC_DIR)/sdmessage.pb-c.c
LIST_SRCS = $(SRC_DIR)/data.c $(SRC_DIR)/list.c

# .o files
CLIENT_OBJS = $(CLIENT_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SERVER_OBJS = $(SERVER_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
LIST_OBJS = $(LIST_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executáveis
CLIENT_BIN = $(BIN_DIR)/list_client
SERVER_BIN = $(BIN_DIR)/list_server
LIB_LIST = $(LIB_DIR)/liblist.a

# Protocol Buffers
PROTO_FILE = sdmessage.proto
PROTO_SRC = $(SRC_DIR)/sdmessage.pb-c.c
PROTO_HEADER = $(INC_DIR)/sdmessage.pb-c.h

# Creates (at least) targets liblist, list_client and list_server
all: liblist list_client list_server

# Creates a static library with data.c and list.c
liblist: $(LIB_LIST)

$(LIB_LIST): $(LIST_OBJS)
	@mkdir -p $(LIB_DIR)
	ar -rcs $@ $^

# Compiles Client
list_client: $(CLIENT_BIN)

$(CLIENT_BIN): $(CLIENT_OBJS) $(LIB_LIST)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) -L$(LIB_DIR) -llist -lprotobuf-c

# Compiles Server
list_server: $(SERVER_BIN)

$(SERVER_BIN): $(SERVER_OBJS) $(LIB_LIST)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) -L$(LIB_DIR) -llist -lprotobuf-c

# Gerar código dos Protocol Buffers
$(PROTO_SRC) $(PROTO_HEADER): $(PROTO_FILE)
	$(PROTOC) --c_out=$(SRC_DIR) $<
	@mv $(SRC_DIR)/sdmessage.pb-c.h $(INC_DIR)/

# Regra genérica para ficheiros objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependências especiais (precisam dos headers dos Protocol Buffers)
$(OBJ_DIR)/client_stub.o: $(PROTO_HEADER)
$(OBJ_DIR)/network_client.o: $(PROTO_HEADER)
$(OBJ_DIR)/network_server.o: $(PROTO_HEADER)
$(OBJ_DIR)/list_skel.o: $(PROTO_HEADER)
$(OBJ_DIR)/list_client.o: $(PROTO_HEADER)
$(OBJ_DIR)/list_server.o: $(PROTO_HEADER)

# clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)
	rm -f $(PROTO_SRC) $(PROTO_HEADER)

# for debug (apaga-se antes de se entregar)
print:
	@echo "Client OBJS: $(CLIENT_OBJS)"
	@echo "Server OBJS: $(SERVER_OBJS)"
	@echo "List OBJS: $(LIST_OBJS)"

.PHONY: all liblist list_client list_server clean print