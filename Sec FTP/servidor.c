#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <arpa/inet.h>

#define MAX_CLIENT 1
#define BUFSIZE 512
#define USRSIZE 100

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

//Funcion que imprime un mensaje de error por standar error
void error(char *msg){
    perror(msg);
    exit(1);
}

//Funcion que valida el usuario comparandolo con el fichero
bool check_user(char *user, char *pass){

    FILE *file;
    char string[USRSIZE], read[USRSIZE];

    //Creo un string con el usuario y contraseña y lo guardo en string    
    strcat(strcpy(string,user),":");
    strcat(string,pass);  
    strcat(string,"\n");      

    file = fopen("ftpusers.txt","r");
    if (file == NULL){
        error("No se pudo leer el archivo");
        return false;
    }

    //Leo el fichero ftpusers.txt y busco el string(user y pass) solicitado
    while(fgets(read, sizeof(read),file) != NULL) {

        if(strcmp (string,read) == 0){
            return true;
        }
    }

    fclose(file);
    return false;
}

//Funcion que lee los datos del usuario y llama a check_user para validarlo
bool authenticate(int sock){

    char data[BUFSIZE];
    char msg331 [BUFSIZE], msg230 [BUFSIZE];
    char user [USRSIZE], pass [USRSIZE];
    char *token = NULL;
    char delimitador_us [] = "USER \r\n", delimitador_pas [] = "PASS \r\n";

    //Leemos el username del cliente
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    token = strtok(data, delimitador_us);

    //Guardamos el username en la variable user
    memset(user,0,sizeof(user));
    strcpy(user,token);

    memset(msg331,0,sizeof(msg331));
    sprintf(msg331,MSG_331,user);

    send(sock, msg331, sizeof(msg331),0);

    token = NULL;
    
    //Leemos la password del cliente
    memset(data,0,sizeof(data));
    if (recv(sock, data, BUFSIZE,0) < 0) {
        error("No se pudo leer");
    }

    token = strtok(data, delimitador_pas);

    //Guardamos la password en la variable pass
    memset(pass,0,sizeof(pass));
    strcpy(pass,token);

    //Utilizo la funcion check_user para validar el usuario
    memset(msg230,0,sizeof(msg230));
    if(check_user(user, pass) == true){
        sprintf(msg230,MSG_230,user);
        send(sock, msg230, sizeof(msg230),0);

        return true;
    }
    else {
        send(sock, MSG_530, sizeof(MSG_530),0);

        return false;
    }
}

int main(int argc, char *argv[]){

    //Defino variables que voy a utilizar
    int sock, newsock, n;
    socklen_t naddr_size ;
    struct sockaddr_in addr, new_addr;
    char data[BUFSIZE];

    if(argc != 2){
        error("Ingresar ./srvFtp <puerto> ");
    }

    //Creo un socket TCP llamado sock
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        error("No se pudo crear el socket");
    }

    printf("Socket creado\n");

    //Asigno a la estructura el tipo de socket que voy a utilizar, direccion y puerto
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(atoi(argv[1]));
    
    //Asocio el socket a la direccion
    if (bind (sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        error("Falla bind");
    }

    printf("Bind correcto\n");

    //Indico que el servidor esta listo para recibir peticiones
    if (listen(sock, MAX_CLIENT) == 0){
        printf("Escuchando..\n");
    } else {
        error("Falla listen");
    }

    //Creo un bucle infinito para que el servidor quede esuchando, y acepte conexiones con clientes
    for(;;){

        naddr_size = sizeof(new_addr);
        newsock = accept(sock, (struct sockaddr*)&new_addr, &naddr_size);

        if (newsock == -1){
            error("Falla accept");
            break;
        }

        send(newsock, MSG_220, strlen(MSG_220),0);

        memset(data, 0, sizeof(data));
        if (authenticate(newsock) == true){

            //Leo los datos que manda el cliente y valido las operaciones
            while((n = recv(newsock, data, BUFSIZE, 0)) > 0){

                if(strcmp (data, "QUIT\r\n") == 0){
                    send(newsock, MSG_221, strlen(MSG_221),0);
                    close(newsock);
                }
                else {
                    send(newsock, "Operacion invalida", strlen("Operacion invalida"),0);
                }
            }     
        }else {
            close(newsock);
        }

    } 
    
    //Cierro el socket
    close(sock);

    return 0;
}