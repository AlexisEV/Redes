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
#include <chrono>
#include <list>
#include <map>

std::list<int> ListaClientes;
std::map<int, int> mapSocketID;
int idActual = 1;

struct Protocolo
{
  int id_dest;       // id destinatario, 0 para enviar a todos
  int id_orig;       // id origen
  char mensaje[100]; // mensaje
};

/*
void writeSocketThread(int clisocket){
  char buffer[300];
  int n;
  do
  {
    printf("Enter a msg:");
    fgets(buffer, 100, stdin);
    n = strlen(buffer);
    buffer[n] = '\0';
    n = write(clisocket, buffer, 100);
  } while (strncmp(buffer, "exit", 4) != 0);
  shutdown(clisocket, SHUT_RDWR);
  close(clisocket);
}
*/
void serverExitThread(int SocketFD)
{
  char buffer[100];
  int n;
  while (true)
  {
    // Lee la línea de comando desde la terminal
    fgets(buffer, sizeof(buffer), stdin);
    // Eliminar el salto de línea
    n = sizeof(buffer);
    buffer[n] = '\0';
    if (strncmp(buffer, "exit",4) == 0)
    {
      printf("cerrando servidor...\n");
      shutdown(SocketFD, SHUT_RDWR);
      close(SocketFD);
      exit(0);
    }
  }
}

void readSocketThread(int clisocket)
{
  Protocolo p;
  do
  {
    int n = read(clisocket, &p, sizeof(Protocolo));
    if (n <= 0)
    {
      break;
    }
    p.id_orig = mapSocketID[clisocket];
    n = sizeof(p.mensaje);
    p.mensaje[n] = '\0';

    if (strncmp(p.mensaje, "exit", 4) == 0)
    {
      break;
    }
    for (std::list<int>::iterator it = ListaClientes.begin(); it != ListaClientes.end(); it++)
    {
      // saltar al cliente que envia
      if (*it == clisocket)
      {
        continue;
      }
      if (p.id_dest == 0)
      { // enviar a todos
        write(*it, &p, sizeof(Protocolo));
      }
      else
      {
        // enviar mensaje al cliente con el id de destino
        if (mapSocketID[*it] == p.id_dest)
          write(*it, &p, sizeof(Protocolo));
      }
    }
  } while (strncmp(p.mensaje, "exit", 4) != 0);
  shutdown(clisocket, SHUT_RDWR);
  close(clisocket);
  ListaClientes.remove(clisocket);
}

int main(void)
{
  struct sockaddr_in stSockAddr;
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

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
  std::thread(serverExitThread,SocketFD).detach();
  for (;;)
  {
    int ConnectFD = accept(SocketFD, NULL, NULL);

    if (0 > ConnectFD)
    {
      perror("error accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    mapSocketID[ConnectFD] = idActual++;
    ListaClientes.push_back(ConnectFD);
    printf("Se unio un nuevo cliente con el id %d\n", mapSocketID[ConnectFD]);
    std::thread(readSocketThread, ConnectFD).detach();
    
    // std::thread(writeSocketThread,ConnectFD).detach();
    /* perform read write operations ... */
  }

  close(SocketFD);
  return 0;
}
