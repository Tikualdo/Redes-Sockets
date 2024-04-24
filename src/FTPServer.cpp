//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//                        Main class of the FTP server
// 
//****************************************************************************

#include <cerrno>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 #include <unistd.h>


#include <pthread.h>

#include <list>

#include "../include/common.h"
#include "../include/FTPServer.h"
#include "../include/ClientConnection.h"


int define_socket_TCP(int port) {

  int fd_socket;
  struct sockaddr_in serv_addr;

  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_socket < 0) {
    perror("ERROR opening socket");
    return -1;
  }

  // Inicializar serv_addr a cero
  bzero((char *) &serv_addr, sizeof(serv_addr));

  // Configurar serv_addr
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  // Vincular el socket a la dirección y puerto especificados
  if (bind(fd_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    close(fd_socket);
    return -1;
  }

  // Escuchar conexiones entrantes
  listen(fd_socket, 5);

  return fd_socket;
}

/**
 * @brief This function is executed when the thread is executed.
*/
void* run_client_connection(void *c) {
  ClientConnection *connection = (ClientConnection *)c;
  connection->WaitForRequests();

  return NULL;
}



FTPServer::FTPServer(int port) {
  this->port = port;
}

/**
 * @brief Stoping of the server
*/
void FTPServer::stop() {
  close(msock);
  shutdown(msock, SHUT_RDWR);
}

/**
 * @brief Starting of the server
*/
void FTPServer::run() {
  struct sockaddr_in fsin;
  int ssock;
  socklen_t alen = sizeof(fsin);
  msock = define_socket_TCP(port);  // This function must be implemented by you.
  while (1) {
    pthread_t thread;
    ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
    if(ssock < 0){
      errexit("Fallo en el accept: %s\n", strerror(errno));
    }
    ClientConnection *connection = new ClientConnection(ssock);
    
    // Here a thread is created in order to process multiple
    // requests simultaneously
    pthread_create(&thread, NULL, run_client_connection, (void*)connection); 
  }
}
