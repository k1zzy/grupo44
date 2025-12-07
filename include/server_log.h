/* Projeto: Sistemas Distribuídos 2025/2026
 * Grupo 44
 * Autores: Rodrigo Afonso (61839), Guilherme Ramos (61840), Miguel Ferreira (61879)
 */

#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sdmessage.pb-c.h"

/* escreve log eventos servidor*/
void write_log(int timestamp, const char *clientAddressPort, const char *eventType, MessageT__Opcode opcode, MessageT__CType c_type, Data *argument);

/* traduz opcode string */
char *translate_op_code(MessageT__Opcode opcode);

/* traduz c_type string */
char *translate_op_c_type(MessageT__CType c_type);

/* traduz marca string */
char *translate_marca(Marca marca);

/* obtem timestamp segundos */
int get_seconds();

/* cria string ip:porta cliente */
char *make_client_addr_port(int sockfd);
    