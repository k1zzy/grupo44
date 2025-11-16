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

/* escrever no server_log os eventos que acontecem no servidor*/
void write_log(int timestamp, const char *clientAddressPort, const char *eventType, MessageT__Opcode opcode, MessageT__CType c_type, Data *argument);

/* traduzir opcode para string */
char *translate_op_code(MessageT__Opcode opcode);

/* traduzir c_type para string */
char *translate_op_c_type(MessageT__CType c_type);

/* traduzir marca para string */
char *translate_marca(Marca marca);

/* obter o timestamp em segundos */
int get_seconds();

/* criar string com o formato "IP:PORTA" do cliente */
char *make_client_addr_port(int sockfd);
    