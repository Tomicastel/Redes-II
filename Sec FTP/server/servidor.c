#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <arpa/inet.h>

#define MAX_CLIENT 4
#define BUFSIZE 512
#define USRSIZE 100
#define OPSIZE 10
#define PSIZE 100
#define IP "127.0.0.1"

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %d bytes\r\n"
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

    file = fopen("./ftpusers.txt","r");
    if (file == NULL){
        error("No se pudo abrir el archivo");
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

void retr(int sock, char *file_name){

    int data_port, bytes_read, data_sock, size;
    FILE *file;
    struct sockaddr_in servidor;
    char data[BUFSIZE];
    char msg550[BUFSIZE];
    char msg299[BUFSIZE];
    struct stat sz;

    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0){ 
        error("No se pudo crear el socket");
    }  

#ifdef DEBUG
    printf("Socket creado para transferencia de datos\n");
#endif

    int sz_servidor = sizeof(servidor);
    getsockname(sock, (struct sockaddr*) &servidor, &sz_servidor);

#ifdef DEBUG
    printf("Puerto del cliente %d\n",ntohs(servidor.sin_port));
#endif

    data_port = ntohs(servidor.sin_port);
    data_port += 1;

    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = inet_addr(IP);
    servidor.sin_port = htons(data_port);

    file = fopen (file_name,"r");

    if (file == NULL){
        memset(msg550,0,sizeof(msg550));
        sprintf(msg550,MSG_550,file_name);
        send(sock, msg550, sizeof(msg550),0);

        exit(-1);
    } 
    else {

        if(stat(file_name, &sz) == -1){
			perror("stat");
			exit(-1);
		}

        size = sz.st_size;

        memset(msg299,0,sizeof(msg299));
        sprintf(msg299,MSG_299,file_name,size);
		send(sock, msg299, sizeof(msg299),0);
    
        if (connect(data_sock, (struct sockaddr*)&servidor, sizeof(servidor)) < 0){
            error("No se pudo conectar al servidor");
        }

        memset(data,0,sizeof(data));
        while(!feof (file)){
            bytes_read = fread(data, sizeof(char),BUFSIZE,file);
            write(data_sock, data, bytes_read);
        }

        send(sock,MSG_226,sizeof(MSG_226),0);
    }

    fclose(file);
    close(data_sock);
    
}


void operation(int sock){

    char op[OPSIZE], param[PSIZE];
    int n;
    char data[BUFSIZE];
    char *token = NULL;
   
    while(1){   

        //Leo los datos que manda el cliente y valido las operaciones
        memset(data,0,sizeof(data));
        if (read(sock, data, sizeof(BUFSIZE)) < 0) {
            error("No se pudo leer");
        }

        token = strtok(data, " ");
        memset(op,0,sizeof(op));
        strcpy(op,token);

        if (strcmp(op, "RETR") == 0){
            token = strtok(NULL," ");
        	strcpy(param,token);
            retr(sock,param);
        }
        if (strcmp(op, "QUIT") == 0){
            send(sock, MSG_221, strlen(MSG_221), 0);
            close(sock);
            break;
        }
        else {
            send(sock, "Operacion invalida", strlen("Operacion invalida"), 0);
        }
    }
}      


int main(int argc, char *argv[]){

    //Defino variables que voy a utilizar
    int sock, newsock;
    socklen_t naddr_size ;
    struct sockaddr_in addr, new_addr;

    if(argc != 2){
        printf("Ingresar: %s <puerto>\n", argv[0]);
        exit(1);
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
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0){
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

        //Envio el mensaje de bienvenida
        send(newsock, MSG_220, strlen(MSG_220),0);

        if (authenticate(newsock) == true){
            operation(newsock);
        } 
    } 
    
    //Cierro el socket
    close(sock);

    return 0;
}