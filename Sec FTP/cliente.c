#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#define BUFSIZE 512

//Funcion que imprime un mensaje de error por standar error
void error(char *msg){
    perror(msg);
    exit (1);
}

//Funcion que lee por standard input y almacena una cadena de caracteres
char *read_input(){

    char *input = malloc (BUFSIZE);

    if(fgets(input, BUFSIZE, stdin)){
        return strtok(input, "\n");
    }
    
    return NULL;
}

//Envia un mensaje al servidor
void send_msg (int sock, char *op, char *param){

    char data[BUFSIZE];

    memset(data,0,sizeof(data));
    if(param != NULL){
        sprintf(data, "%s %s\r\n",op,param);
    }
    else {
        sprintf(data, "%s\r\n",op);
    }
    
    send(sock, data, sizeof(data),0);
}

//Funcion para autenticar al cliente
void authenticate (int sock){

    char *input = NULL;
    char data[BUFSIZE];

    //Pido el nombre de usuario al cliente
    printf("username: ");
    input = read_input();

    //Envio el comando USER con el nombre al servidor
    send_msg(sock, "USER", input);

    free(input);

    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n", data);

    //Pido la contraseña al cliente
    printf("password: ");
    input = read_input();

    //Envio el comando PASS con la contraseña al servidor
    send_msg(sock, "PASS", input);

    free(input);

    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n", data);

    //Si el login es incorrecto, cierro la conexion
    if(strcmp (data, "530 Login incorrect\r\n") == 0){
        close (sock);
        exit (-1);
    }
}

//Funcion que finaliza la conexion con el servidor
void quit(int sock){

    send_msg(sock,"QUIT",NULL);
    
    char data[BUFSIZE];

    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n", data); 

    close(sock);
}

//Evaluo las operaciones recibidas y mando mensaje al servidor
void operation(int sock){

    char *input, *op;
    char data[BUFSIZE];

    while(1){
        printf("Operacion: ");
        input = read_input();
        if (input == NULL)
            continue;
        op = strtok(input, " ");
    
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

            printf("%s \n\n",data);

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

    if(argc != 3){
        error("Ingresar ./clFtp <direccion IP> <puerto> ");
    }

    //Creo un socket TCP llamado sock
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){ 
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    //Asigno a la estructura el tipo de socket que voy a utilizar, direccion y puerto
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    //Solicito conectarme al socket (sock)
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        error("No se pudo conectar al servidor");
    }

    printf("Conectado al servidor\n");

    //Limpio el buffer de la varible data
    memset(data,0,sizeof(data));

    //Leo los datos que me manda el servidor
    if (read(sock, data, sizeof(data)) < 0) {
        error("No se pudo leer");
    }
    
    printf("%s\n", data);

    //LLamo a la funcion authenticate para validar al usuario
    authenticate(sock);

    //Llamo a la funcion operacion y le paso el socket
    operation(sock);

    return 0;
}
