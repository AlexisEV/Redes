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

void read_thread(int socket) {
    char buffer[256];
    int n;
    while (true) {
        n = read(socket, buffer, 100);
        if(n<=0){
          break;
        }
        buffer[n] = '\0';
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        printf("\nServer: %s\n", buffer);
    }
}

int main(void) {
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    int n;
    char buffer[256];

    if (-1 == SocketFD) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

    if (Res <= 0) {
        perror("Invalid address");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in))) {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    // thread read
    std::thread t(read_thread, SocketFD);
    t.detach();

    // write
    while (true) {
        //printf("Enter a msg: ");
        fgets(buffer, 100, stdin);
        n = strlen(buffer);
        buffer[n] = '\0';
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        write(SocketFD, buffer, 100);
    }

    shutdown(SocketFD, SHUT_RDWR);
    close(SocketFD);
    return 0;
}