#include <stdio.h>
#include <pthread.h>

/* escrever no server_log os eventos que acontecem no servidor*/
void write_log(int timestamp, char* clientPort, char* eventType, char* operation, char* contentType, char* argument);
