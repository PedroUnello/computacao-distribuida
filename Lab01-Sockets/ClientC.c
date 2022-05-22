#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

int main(){
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in conn;
    conn.sin_addr.s_addr = inet_addr("127.0.0.1");
    conn.sin_family = AF_INET;
    conn.sin_port = htons(8080);
    int tamEnd = sizeof(conn);
    printf("\nConnected = %d", connect(server, (struct sockaddr*)&conn, sizeof(conn)));
    char* encodeMsg = "Hello\n";
    char receive[1024] = {0};
    send(server, encodeMsg, strlen(encodeMsg), 0);
    recv(server, receive, sizeof(receive), 0);
    printf("%s\n", receive);
    encodeMsg = "meu nome é Computação Distribuída ;-)\n";
    send(server, encodeMsg, strlen(encodeMsg), 0);
    recv(server, receive, sizeof(receive), 0);
    printf("%s\n", receive);
    return 0;
}

