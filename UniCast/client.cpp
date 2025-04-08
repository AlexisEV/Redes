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
#include <string>
#include <atomic>

std::string nickname;
std::atomic<bool> envLista(false);

void readSocketThread(int cliSocket)
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

    //Mensaje l
    if (buffer[0]=='l'){
      n = read(cliSocket,buffer,tamano-1);
      buffer[n]='\0';
      printf("\nUsuarios conectados: %s\n",buffer);
      envLista = true;
    }

    //Mensaje m
    else if (buffer[0]=='m'){
      std::string origUser;
      std::string msg;
      n = read(cliSocket,buffer,5);
      buffer[n] = '\0';
      int tamanoMsg = atoi(buffer);

      n = read(cliSocket,buffer,tamanoMsg);
      buffer[n] = '\0';
      msg = buffer;

      n = read(cliSocket,buffer,5);
      buffer[n] = '\0';
      int tamanoOrig = atoi(buffer);

      n = read(cliSocket,buffer,tamanoOrig);
      buffer[n] = '\0';
      origUser = buffer;
      
      printf("\n\n%s: %s\n",origUser.c_str(),msg.c_str());
    }

  } while (strncmp(buffer, "exit", 4) != 0);
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

  // Mensaje N para registro
  std::cout << "Ingresa tu nickname: ";
  std::getline(std::cin, nickname);
  sprintf(buffer, "%05dN%s\0", nickname.length() + 1, nickname.c_str());
  //printf("%s\n",buffer);
  write(SocketFD,buffer,nickname.length()+1+5);
  std::thread(readSocketThread, SocketFD).detach();

  bool salir = false;
  while (!salir)
  {
    //Menu
    int opcion;
    std::cout << "\n=== MENÚ PRINCIPAL ===" << std::endl;
    std::cout << "1. Ver lista de usuarios conectados" << std::endl;
    std::cout << "2. Mandar mensaje" << std::endl;
    std::cout << "3. Salir" << std::endl;
    std::cout << "Seleccione una opción: ";
    std::cin >> opcion;

    //Mensaje L
    if (opcion == 1)
    {
      int tamano;
      strcpy(buffer, "00001L"); 
      buffer[6] = '\0';
      write(SocketFD,buffer,6);
      while(!envLista);
      envLista = false;
    }

    //Mensaje M
    else if (opcion == 2)
    {
      std::string dest;
      std::string msg;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
      std::cout << "\nNombre Destinatario: ";
      std::getline(std::cin,dest);
      std::cout << "\nMensaje: ";
      std::getline(std::cin,msg);
      sprintf(buffer,"%05dM%05d%s%05d%s",dest.length()+msg.length()+1,msg.length(),msg.c_str(),dest.length(),dest.c_str());
      //printf("Mensaje M a enviar: %s\n", buffer);
      write(SocketFD,buffer,msg.length()+dest.length()+5+5+5+1);
    }

    //Mensaje E
    else if (opcion == 3)
    {
      strcpy(buffer, "00001E");
      buffer[6] = '\0';
      write(SocketFD,buffer,6);
      salir = true;
    }
    else
    {
      std::cout << "Opción no válida." << std::endl;
    }
  }
  /*
  do
  {
    fgets(buffer, 100, stdin);
    n = strlen(buffer);
    buffer[n] = '\0';
    n = write(SocketFD, buffer, 100);
  } while (strncmp(buffer, "exit", 4) != 0);
  */

  shutdown(SocketFD, SHUT_RDWR);

  close(SocketFD);
  return 0;
}
