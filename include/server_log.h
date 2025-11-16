#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sdmessage.pb-c.h"

/* escrever no server_log os eventos que acontecem no servidor*/
void write_log(int timestamp, const char *clientAddressPort, const char *eventType, MessageT__Opcode opcode, MessageT__CType c_type, Data *argument);

int get_seconds();

char *make_client_addr_port(int sockfd);
    