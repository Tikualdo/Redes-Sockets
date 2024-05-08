//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transaction.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>
 
#include <iostream>
#include <dirent.h>

#include "../include/common.h"
#include "../include/ClientConnection.h"
#include "../include/FTPServer.h"



ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
  if (fd == NULL){
    std::cout << "Connection closed" << std::endl;

    fclose(fd);
    close(control_socket);
    ok = false;
    return ;
  }
    
  ok = true;
  data_socket = -1;
  parar = false;
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
}


int connect_TCP(uint32_t address,  uint16_t  port) {
  struct sockaddr_in sin;
  struct hostent *hent;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = address;
  sin.sin_port = htons(port);
  s = socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0) {
    errexit("No se puede crear el socket: %s\n", strerror(errno));
  }
  auto conexion = connect(s, (struct sockaddr*)&sin, sizeof(sin));
  if(conexion < 0) {
    errexit("No se puede conectar al servidor: %s\n", strerror(errno));
  }

  return s; 
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}
    
#define COMMAND(cmd) strcmp(command, cmd) == 0

/**
 * @brief This method processes the requests.
 * @example See the example for the USER command.
 *    If you think that you have to add other commands feel free to do so.
 *    You are allowed to add auxiliary methods if necessary.
*/
void ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }
  fprintf(fd, "220 Service ready\n");
  while(!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "331 User name ok, need password\n");
    }
    else if (COMMAND("PWD")) {
    
    }
    else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      if(strcmp(arg,"1234") == 0){
        fprintf(fd, "230 User logged in\n");
      } else {
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }
    }
    else if (COMMAND("PORT")) {
      int h1,h2,h3,h4,p1,p2;
      fscanf(fd,"%d,%d,%d,%d,%d,%d",&h1,&h2,&h3,&h4,&p1,&p2);
      printf("(%d,%d,%d,%d,%d,%d)",h1,h2,h3,h4,p1,p2);
      uint32_t address = h4 << 24 | h3 << 16 | h2 << 8 | h1;
      uint16_t port = p1 << 8 | p2;
      data_socket = connect_TCP(address,port);
      fprintf(fd, "200 OK\n");
      fflush(fd);
    }
    else if (COMMAND("PASV")) {
      int s = define_socket_TCP(0);
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      int Result = getsockname(s, (struct sockaddr*)&addr, &len);
      uint16_t pp = addr.sin_port;
      int p1 = pp >> 8;
      int p2 = pp & 0xFF;
      if (Result < 0) {
        fprintf(fd, "421 service not available, closing control connection");
        fflush(fd);
        return;
      }
      fprintf(fd, "227 Entering in passive mode (127,0,0,1,%d,%d)\n",p2,p1);
      len = sizeof(addr);
      fflush(fd);
      Result = accept(s, (struct sockaddr*)&addr, &len);
      if(Result < 0) {
        fprintf(fd, "421 Service not available, closing control connection");
        fflush(fd);
        return;
      }
      data_socket = Result;
    }
    else if(COMMAND("STOR") ) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "150 Estado correcto, abrimos la conexión\n");
      fflush(fd);
      FILE *f = fopen(arg, "wb+");

      if (f == NULL){
        fprintf(fd, "425 No se puede establecer una conexión\n");
        fflush(fd);
        close(data_socket);
        break;
      }

      fprintf(fd, "125 Conexión ya abierta, comenzando transferencia\n");
      char *buffer[MAX_BUFF];

      fflush(fd);

      while(1){
        int b = recv(data_socket, buffer, MAX_BUFF,0);
        fwrite (buffer,1,b,f);
        if (b != MAX_BUFF){break;}
      }

      fprintf(fd,"226 Cerrando conexión\n");
      fflush(fd);
      fclose(f);
      close(data_socket);
      fflush(fd);
    }
    else if (COMMAND("RETR")) {
      std::cout << "->"<< data_socket <<"<<-"<<std::endl;
      fscanf(fd,"%s", arg);
      FILE *f = fopen(arg, "r");
      if (f != NULL) { 
        fprintf(fd, "125 Established conection; making transference. \n");
        fflush(fd);
        char *buffer[MAX_BUFF];
        while(1) {
          int b= fread(buffer,1,MAX_BUFF,f);
          std::cout << "--" << std::endl;
          if (b == 0) {
            break;
          }
          send(data_socket,buffer,b,0);
        }
        
        fprintf(fd, "226 Closing connection.\n");
        fflush(fd);

        close(data_socket);
        fclose(f);
	    }
	    else {
        fprintf(fd, "425 Cant stablish connection.\n");
        fflush(fd);

        close(data_socket);
	    }
    }
    else if (COMMAND("LIST")) {
      DIR *dir = opendir("..");	
      struct dirent *e;
      fprintf(fd, "150 Here comes the directory listing.\n");
      while( e = readdir(dir)) {
        send(data_socket, e->d_name, strlen(e->d_name), 0);
        send(data_socket, "\r\n", 2, 0);
      }
      fprintf(fd, "226 Directory send OK.\n");
      closedir(dir);
      close(data_socket);
    }
    else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");   
    }
    else if (COMMAND("TYPE")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "200 OK\n");   
    }
    else if (COMMAND("QUIT")) {
      fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
      close(data_socket);	
      parar=true;
      break;
    }
    else  {
      fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }
  fclose(fd);  
  return;
};
