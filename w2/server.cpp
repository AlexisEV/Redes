#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <atomic>

std::atomic<bool> running(true);

void read_thread(int clientSocket)
{
  char buffer[256];
  int n;
  while (true)
  {
    n = read(clientSocket, buffer, 100);
    if (n <= 0 || strncmp(buffer, "exit", 4) == 0)
    {
      //printf("ERROR o exit\n");
      running = false;
      break;
    }
    buffer[n] = '\0';
    printf("\nCliente: %s\n", buffer);
  }
}

int main(void)
{
  struct sockaddr_in stSockAddr;
  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];
  int n;

  if (SocketFD == -1)
  {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(45000);
  stSockAddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)) == -1)
  {
    perror("error bind failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  if (listen(SocketFD, 1) == -1)
  {
    perror("error listen failed");
    close(SocketFD);
    exit(EXIT_FAILURE);
  }

  for (;;)
  {
    int ClientFD = accept(SocketFD, NULL, NULL);
    running = true;
    if (ClientFD < 0)
    {
      perror("error accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    // thread read
    std::thread t(read_thread, ClientFD);
    t.detach();

    // write
    while (running)
    {
      // printf("Enter a msg: ");
      if (fgets(buffer, 100, stdin) == nullptr)
      {
        break;
      }
      if (!running)
      {
        break;
      }
      n = strlen(buffer);
      buffer[n] = '\0';
      write(ClientFD, buffer, 100);
      if (strncmp(buffer, "exit", 4) == 0)
      {
        break;
      }
    }
    printf("Conexion finalizada...\n");
    fflush(stdout);
    shutdown(ClientFD, SHUT_RDWR);
    close(ClientFD);
  }
  close(SocketFD);
  return 0;
}