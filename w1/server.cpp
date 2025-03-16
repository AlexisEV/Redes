  /* Server code in C */
 
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <iostream>
  #include <cstring>
 
  int main(void)
  {
    struct sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    char buffer[256];
    int n;
 
    if(-1 == SocketFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }
 
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;
 
    if(-1 == bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("error bind failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    if(-1 == listen(SocketFD, 10))
    {
      perror("error listen failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    for(;;)
    {
      int ConnectFD = accept(SocketFD, NULL, NULL);
 
      if(0 > ConnectFD)
      {
        perror("error accept failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
      }
 
 
     //bzero(buffer,256);
     do{
      memset(buffer,0,256);
      n = read(ConnectFD,buffer,255);
      if (n < 0){
        perror("ERROR reading from socket");
        break;
      }
      buffer[n] = '\0';
      printf("Client: %s\n",buffer);
      if(strcmp(buffer,"chau") == 0){
        printf("Conversacion finalizada...\n");
        break;
      }
      printf("Tu: ");
      memset(buffer,0,256);
      fgets(buffer,256,stdin);
      buffer[strcspn(buffer,"\n")] = '\0';

      n = write(ConnectFD,buffer,strlen(buffer));
      if(n<0){
        perror("ERROR writiing to socket");
        break;
      }
      }while(strcmp(buffer,"chau") != 0);
    
 
     /* perform read write operations ... */
 
      shutdown(ConnectFD, SHUT_RDWR);
 
      close(ConnectFD);
    }
 
    close(SocketFD);
    return 0;
  }
