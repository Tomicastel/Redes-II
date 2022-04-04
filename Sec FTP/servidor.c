#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CLIENT 1
#define BUFSIZE 1024

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_221 "221 Goodbye\r\n"

void error(char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){

    int sock, newsock, n;
    socklen_t naddr_size ;
    struct sockaddr_in addr, new_addr;
    char data[BUFSIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; //Cualquier cliente puede conectarse
    addr.sin_port = htons(atoi(argv[1]));
    
    if (bind (sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        error("Falla bind");
    }

    printf("Bind correcto\n");

    if (listen(sock, MAX_CLIENT) == 0){
        printf("Escuchando..\n");
    } else {
        error("Falla listen");
    }

    memset(data, 0, sizeof(data));

    for(;;){

        naddr_size = sizeof(new_addr);
        newsock = accept(sock, (struct sockaddr*)&new_addr, &naddr_size);

        if (newsock == -1){
            error("Falla accept");
            break;
        }

        send(newsock, MSG_220, strlen(MSG_220),0);

        while((n = recv(newsock, data, BUFSIZE, 0)) > 0){

            if(strcmp (data, "QUIT\r\n") == 0){
                send(newsock, MSG_221, strlen(MSG_221),0);
                close(newsock);
            }
            else {
                send(newsock, "Operacion invalida", strlen("Operacion invalida"),0);
            }
        
        }     

    }  
    
    return 0;
}