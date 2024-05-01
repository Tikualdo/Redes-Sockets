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
      fscanf(fd, "%s", arg);
      int i1, i2, i3, i4, p5, p6;
      if (sscanf(arg, "%d,%d,%d,%d,%d,%d", &i1, &i2, &i3, &i4, &p5, &p6) != 6) {
        fprintf(fd, "501 Syntax error in parameters or arguments.\n");
      } else {
        uint32_t address = (i1 << 24) | (i2 << 16) | (i3 << 8) | i4;
        uint16_t port = (p5 << 8) | p6;
        data_socket = connect_TCP(address, port);
        if (data_socket == -1) {
          fprintf(fd, "425 Can't open data connection.\n");
        } else {
          fprintf(fd, "200 PORT command successful.\n");
        }
      }
    }
    else if (COMMAND("PASV")) {
      struct sockaddr_in fsin;
      socklen_t slen = sizeof(fsin);
      int s = define_socket_TCP(0);
      getsockname(s, (struct sockaddr*)&fsin, &slen);
      uint16_t port = fsin.sin_port;
      int p1 = (port >> 8) & 0xff;
      int p2 = port & 0xff;
      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d)\n", p1, p2);

      slen = sizeof(fsin);
      data_socket = accept(s, (struct sockaddr*)&fsin, &slen);
    }
    else if(COMMAND("STOR") ) {
      fscanf(fd, "%s", arg);
      FILE* file = fopen(arg, "w");
      if(file == NULL) {
        fprintf(fd, "550 File not found.\n");
      }
      char buffer[MAX_BUFF];
      fprintf(fd, "150 File status okay; about to open data connection.\n");
      while(true) {
        int recv_data = recv(data_socket, buffer, MAX_BUFF, 0);
        fwrite(buffer, 1, recv_data, file);
        if(recv_data == 0) {
          break;
        }
      }
      fprintf(fd, "226 Transfer complete.\n");
      fclose(file);
      close(data_socket);
    }
    else if (COMMAND("RETR")) {
      fscanf(fd, "%s", arg);
      fd = fopen(arg, "r");
      if(fd == NULL) {
        fprintf(fd, "550 File not found.\n");
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        while(true) {
          char buffer[MAX_BUFF];
          size_t bytes_read = fread(buffer, 1, MAX_BUFF, fd);
          send(data_socket, buffer, bytes_read, 0);
          if(bytes_read < MAX_BUFF) {
            break;
          }
        }
        fprintf(fd, "226 Transfer complete.\n");
        fclose(fd);
        close(data_socket);
      }
    }
    else if (COMMAND("LIST")) {
      DIR *dir = opendir(".");	
      struct dirent *e;
      fprintf(fd, "150 Here comes the directory listing.\n");
      while( e = readdir(dir)) {
        send(data_socket, e->d_name, strlen(e->d_name), 0);
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
