/* Client code in C */

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

struct Protocolo
{
  int id_dest;       // id del cliente, 0 para enviar a todos
  int id_orig; 
  char mensaje[100]; // mensaje
};

void readSocketThread(int clisocket)
{
  char buffer[300];
  Protocolo rcvP;
  do
  {
    int n = read(clisocket, &rcvP, sizeof(Protocolo));
    if (n <= 0) {
      break;
    }
    n = sizeof(rcvP.mensaje);
    rcvP.mensaje[n] = '\0';
    printf("\nUser %d: %s", rcvP.id_orig, rcvP.mensaje);
  } while (strncmp(rcvP.mensaje, "exit", 4) != 0);
}

int main(void)
{
  char buffer[256];
  struct sockaddr_in stSockAddr;
  int Res;
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  int n;

  if (-1 == SocketFD)
  {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(45000);
  Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

  if (0 > Res)
  {
    perror("error: first parameter is not a valid address family");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
  else if (0 == Res)
  {
    perror("char string (second parameter does not contain valid ipaddress");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
  {
    perror("connect failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }
  std::thread(readSocketThread, SocketFD).detach();
  Protocolo p;

  printf("id del destinatario: ");
  scanf("%d", &p.id_dest);
  getchar();

  while (true)
  {
    //printf("Ingrese el mensaje: ");
    fgets(p.mensaje, 100, stdin);
    n = sizeof(p.mensaje);
    p.mensaje[n] = '\0';
    // Enviar el protocolo completo al servidor.
    write(SocketFD, &p, sizeof(Protocolo));
    if (strncmp(p.mensaje, "exit", 4) == 0){
      break;
    }
  }

  shutdown(SocketFD, SHUT_RDWR);

  close(SocketFD);
  return 0;
}
