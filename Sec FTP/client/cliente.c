#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#define BUFSIZE 512
#define IP "127.0.0.1"

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
    while (1){
        printf("username: ");
        input = read_input();

        if (input == NULL)
            continue;
        else
            break;
    }

    //Envio el comando USER con el nombre al servidor
    send_msg(sock, "USER", input);

    free(input);

#ifdef DEBUG
    printf("Mensaje enviado del servidor: ");
#endif
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n", data);

    //Pido la contraseña al cliente
    while (1){
        printf("password: ");
        input = read_input();

        if (input == NULL)
            continue;
        else
            break;
    }

    //Envio el comando PASS con la contraseña al servidor
    send_msg(sock, "PASS", input);

    free(input);

#ifdef DEBUG
    printf("Mensaje enviado del servidor: ");
#endif
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

#ifdef DEBUG
    printf("Mensaje enviado del servidor: ");
#endif
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n", data); 

    close(sock);
}

void get(int sock, char *file_name){

    int data_sock, newsock; 
    int data_port, bytes_read;
    struct sockaddr_in cliente;
    socklen_t addr_size;
    FILE *file;
    char data[BUFSIZE];

    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0){ 
        error("No se pudo crear el socket");
    }  

#ifdef DEBUG
    printf("Socket creado para transferencia de datos\n");
#endif

    int sz_cliente = sizeof(cliente);
    getsockname(sock, (struct sockaddr*) &cliente, &sz_cliente);

#ifdef DEBUG
    printf("Mi puerto es %d\n",ntohs(cliente.sin_port));
#endif

    data_port = ntohs(cliente.sin_port);
    data_port += 1;

#ifdef DEBUG
    printf("Mi nuevo puerto es %d\n",data_port);
#endif

    cliente.sin_family = AF_INET;
    cliente.sin_addr.s_addr = inet_addr(IP);
    cliente.sin_port = htons(data_port);

    if (bind(data_sock, (struct sockaddr*)&cliente, sizeof(cliente)) < 0){
        error("Falla bind");
    }

#ifdef DEBUG
    printf("Bind correcto\n");
#endif   

    if (listen(data_sock, 1) == 0){
        printf("Escuchando..\n");
    } else {
        error("Falla listen");
    }

    send_msg(sock,"RETR",file_name);

#ifdef DEBUG
    printf("Mensaje enviado del servidor: ");
#endif
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n",data);

    addr_size = sizeof(cliente);
    newsock = accept(data_sock, (struct sockaddr*)&cliente,&addr_size);
    if (newsock == -1){
        error("Falla accept");
    }

    file = fopen(file_name,"w");

    while(bytes_read == BUFSIZE){

        memset(data,0,sizeof(data));
        if (bytes_read = read(newsock, data, sizeof(data)) < 0) {
             error("No se pudo leer");
        }   
#ifdef DEBUG
        printf("El archivo ha sido recibido, tamaño: %d", bytes_read);
#endif
        fwrite(data, 1, bytes_read, file);
    }

#ifdef DEBUG
    printf("Mensaje enviado del servidor: ");
#endif
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    printf("%s \n",data);

    close(data_sock);
    close(newsock);
    fclose(file);

}


//Evaluo las operaciones recibidas y mando mensaje al servidor
void operation(int sock){

    char *input = NULL, *op = NULL, *param = NULL;
    char data[BUFSIZE];

    while(1){
        printf("Operacion: ");
        input = read_input();
        if (input == NULL)
            continue;
        op = strtok(input, " ");
    
        if (strcmp(op, "get") == 0) {
            param = strtok(NULL, " ");
            get(sock, param);
        }
        else if (strcmp (op, "quit") == 0){
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
    struct sockaddr_in servidor;
    char data[BUFSIZE];

    if(argc != 3){
        printf("Ingresar: %s <direccion IP> <puerto>\n",argv[0]);
        exit(1);
    }

    //Creo un socket TCP llamado sock
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){ 
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    //Asigno a la estructura el tipo de socket que voy a utilizar, direccion y puerto del servidor
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = inet_addr(argv[1]);
    servidor.sin_port = htons(atoi(argv[2]));

    //Solicito conectarme al socket (sock)
    if (connect(sock, (struct sockaddr*)&servidor, sizeof(servidor)) < 0){
        error("No se pudo conectar al servidor");
    }
/*
#ifdef DEBUG
    int size_cliente = sizeof(cliente);
    getsockname(sock, (struct sockaddr*) &cliente, &size_cliente);
    printf("Mi puerto es %d\n",ntohs(cliente.sin_port));
#endif
*/
    printf("Conectado al servidor\n");

#ifdef DEBUG
    printf("Mensaje enviado del servidor: ");
#endif
    //Leo mensaje de bienvenida que me manda el servidor
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }
    
    printf("%s \n", data);

    //LLamo a la funcion authenticate para validar al usuario
    authenticate(sock);

    //Llamo a la funcion operacion y le paso el socket
    operation(sock);

    return 0;
}
