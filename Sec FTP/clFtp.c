#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void error(char *msg){
    perror(msg);
    exit (1);
}

int main(int argc, char *argv[]){

    int sock;
    struct sockaddr_in addr;
    char data[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){ 
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = atoi(argv[2]);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        error("No se pudo conectar al servidor");
    }

    printf("Conectado al servidor\n");

    memset(data,0,sizeof(data));
    if (read(sock, data, sizeof(data)) < 0) {
        error("No se pudo leer");
    }
    
    printf("Mensaje recibido del servidor: %s\n", data);

    close(sock);
    return 0;
}
