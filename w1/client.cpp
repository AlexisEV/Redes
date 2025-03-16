 /* Client code in C */
 
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <iostream>
  #include <string>
 
  int main(void)
  {
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    int n;
    char buffer[256];
 
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

    printf("Conectado con server, escriba su mensaje:\n");
    while(true){
      printf("Tu: ");
      memset(buffer,0,256);
      fgets(buffer,256,stdin);
      buffer[strcspn(buffer,"\n")] = '\0';

      if(write(SocketFD,buffer,strlen(buffer)) == -1){
        perror("ERROR writing to socket");
        break;
      }

      if(strcmp(buffer,"chau") == 0){
        break;
      }

      memset(buffer,0,256);
      n = read(SocketFD, buffer, 255);
      if(n<0){
        printf("ERROR rading from socket");
        break;
      }
      buffer[n]='\0';
      printf("Server: %s\n",buffer);
    }

    
    /* perform read write operations ... */
 
    shutdown(SocketFD, SHUT_RDWR);
 
    close(SocketFD);
    return 0;
  }
