#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024

void error(char *msg){
    perror(msg);
    exit (1);
}

char *read_input(){

    char *input = malloc (BUFSIZE);

    if(fgets(input, BUFSIZE, stdin)){
        return strtok(input, "\n");
    }
    
    return NULL;
}

void quit(int sock){

    send(sock,"QUIT\r\n",strlen("QUIT\r\n"),0);
    
    char data[BUFSIZE];

    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s\n", data); 

    close(sock);
}

void operacion(int sock){

    char *input, *op;
    char data[BUFSIZE];

    while(1){
        printf("Operacion: ");
        input = read_input();
        op = strtok(input, "");
    
        if (strcmp (op, "quit") == 0){
            quit(sock);
            break;
        } 
        else {
            send(sock,op,strlen(op),0);

            memset(data,0,sizeof(data));
             if (recv(sock, data, BUFSIZE,0) < 0) {
                error("No se pudo leer");
            }

            printf("%s\n\n",data);

            continue;
        }

        free(input);
    }
    free(input);
}

int main(int argc, char *argv[]){

    int sock;
    struct sockaddr_in addr;
    char data[BUFSIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){ 
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        error("No se pudo conectar al servidor");
    }

    printf("Conectado al servidor\n");

    memset(data,0,sizeof(data));
    if (read(sock, data, sizeof(data)) < 0) {
        error("No se pudo leer");
    }
    
    printf("%s\n", data);

    operacion(sock);

    return 0;
}
