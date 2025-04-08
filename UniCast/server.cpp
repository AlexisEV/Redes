/* Server code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream> // std::cout
#include <thread>   // std::thread, std::this_thread::sleep_for

#include <map>
#include <string>

using namespace std;

std::map<string,int> mapSockets;



void readSocketThread(int cliSocket, string nickname)
{
  char buffer[300];
  int n, tamano;
  do
  {
    n = read(cliSocket, buffer, 5);
    buffer[n]='\0';
    tamano = atoi(buffer);
    n = read(cliSocket,buffer,1);
    buffer[n]='\0';

    //printf("Codigo recibido: %s\n",buffer);

    //MENSAJE l
    if (buffer[0]=='L'){
      std::string msg;
      std::string  coma = "";
      for (auto it = mapSockets.cbegin(); it != mapSockets.cend(); ++it){
        if((*it).first!=nickname){
          msg += coma + (*it).first;
          coma = ",";
        }
      }
      sprintf(buffer,"%05dl%s\0",msg.length()+1,msg.c_str());
      //printf("Mensaje l a ser enviado: %s\n",buffer);
      write(mapSockets[nickname],buffer,msg.length()+1+5);
    }

    //MENSAJE m
    else if (buffer[0]=='M'){
      std::string msg;
      std::string dest;
      n = read(cliSocket, buffer, 5);
      buffer[n]='\0';
      int tamanoMsg = atoi(buffer);

      //printf("\ntamano mensaje: %d\n", tamanoMsg);

      n = read(cliSocket,buffer,tamanoMsg);
      buffer[n]='\0';
      msg = buffer;
      n = read(cliSocket,buffer,5);
      buffer[n]='\0';
      int tamanoDest = atoi(buffer);
      n = read(cliSocket,buffer,tamanoDest);
      buffer[n]='\0';
      dest = buffer;
      sprintf(buffer,"%05dm%05d%s%05d%s\0",msg.length()+nickname.length()+1,msg.length(),msg.c_str(),nickname.length(),nickname.c_str());
      //printf("\nMensaje m a ser enviado %s\n",buffer);
      write(mapSockets[dest],buffer,msg.length()+nickname.length()+5+5+5+1);
    }
    
    //MENSAJE E
    else if (buffer[0]=='E'){
      printf("\nEl cliente %s ha salido del chat\n",nickname.c_str());
      break;
    }

  } while (strncmp(buffer, "exit", 4) != 0);
  shutdown(cliSocket, SHUT_RDWR);
  close(cliSocket);
  mapSockets.erase (nickname);
}

int main(void)
{
  struct sockaddr_in stSockAddr;
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];
  int n;

  if (-1 == SocketFD)
  {
    perror("can not create socket");
    exit(EXIT_FAILURE);
  }

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(45000);
  stSockAddr.sin_addr.s_addr = INADDR_ANY;

  if (-1 == bind(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
  {
    perror("error bind failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (-1 == listen(SocketFD, 10))
  {
    perror("error listen failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  for (;;)
  {
    printf("%s\n","Esperando cliente...\n");
    int ClientFD = accept(SocketFD, NULL, NULL);

    if (0 > ClientFD)
    {
      perror("error accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    n=read(ClientFD,buffer,5);
    buffer[n]='\0'; 
    //printf("%s\n",buffer);
    int tamano =  atoi(buffer);
    n=read(ClientFD,buffer,1);
    buffer[n]='\0'; 
    if (buffer[0] == 'N' ){
      n=read(ClientFD,buffer,tamano-1);
      buffer[n]='\0';
      mapSockets[buffer]=ClientFD;
      printf("\nWe have a new Client: %s\n", buffer);
      std::thread(readSocketThread, ClientFD,buffer).detach();
      //std::thread(writeSocketThread, ClientFD).detach();
    }
    else{
      shutdown(ClientFD, SHUT_RDWR);
      close(ClientFD);
    }
 
  }

  close(SocketFD);
  return 0;
}
