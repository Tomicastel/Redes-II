#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CLIENT 1
#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_221 "221 Goodbye\r\n"

void error(char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

    int sock, newsock;
    socklen_t naddr_size ;
    struct sockaddr_in addr, new_addr;
    char data[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    addr.sin_family = AF_INET;
    addr.sin_port = atoi(argv[1]);
    addr.sin_addr.s_addr = INADDR_ANY; //Cualquier cliente puede conectarse

    if (bind (sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        error("Falla bind");
    }

    printf("Bind correcto\n");

    if (listen(sock, MAX_CLIENT) == 0){
        printf("Escuchando..\n");
    } else {
        error("Falla listen");
    }

    while(1){

        naddr_size = sizeof(new_addr);
        newsock = accept(sock, (struct sockaddr*)&new_addr, &naddr_size);
        if (newsock == -1){
            error("Falla accept");
        }

    send(newsock, MSG_220, strlen(MSG_220),0);
    close(newsock);
    }    
    
    close(sock);
    return 0;
}