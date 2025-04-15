/* Server code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>   // std::cout
#include <thread>     // std::thread, std::this_thread::sleep_for
#include <map>
#include <string>
#include <vector>

using namespace std;

std::map<string,int> mapaSockets;

void leerSocket(int socketCliente, string nickname)
{
  char buffer[300];
  int n, tamano;
  do
  {
    n = read(socketCliente, buffer, 5);
    if(n <= 0) break;
    buffer[n] = '\0';
    tamano = atoi(buffer);

    n = read(socketCliente, buffer, 1);
    if(n <= 0) break;
    buffer[n] = '\0';

    // MENSAJE l
    if (buffer[0]=='L'){
      std::string mensaje;
      std::string coma = "";
      for (auto it = mapaSockets.cbegin(); it != mapaSockets.cend(); ++it){
        if((*it).first != nickname){
          mensaje += coma + (*it).first;
          coma = ",";
        }
      }
      sprintf(buffer, "%05dl%s\0", (int)mensaje.length() + 1, mensaje.c_str());
      write(mapaSockets[nickname], buffer, mensaje.length() + 6); // 6 = 5+1
    }
    // MENSAJE m
    else if (buffer[0]=='M'){
      std::string mensaje;
      std::string destino;
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoMensaje = atoi(buffer);

      n = read(socketCliente, buffer, tamanoMensaje);
      buffer[n] = '\0';
      mensaje = buffer;
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoDestino = atoi(buffer);
      n = read(socketCliente, buffer, tamanoDestino);
      buffer[n] = '\0';
      destino = buffer;
      sprintf(buffer, "%05dm%05d%s%05d%s\0", (int)mensaje.length() + (int)nickname.length() + 1, (int)mensaje.length(), mensaje.c_str(), (int)nickname.length(), nickname.c_str());
      write(mapaSockets[destino], buffer, mensaje.length() + nickname.length() + 16);
    }
    // MENSAJE b
    else if (buffer[0]=='B'){
      printf("\nEntrando a mensaje B\n");
      std::string mensaje;
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoMensaje = atoi(buffer);
      n = read(socketCliente, buffer, tamanoMensaje);
      buffer[n] = '\0';
      mensaje = buffer;
      sprintf(buffer, "%05db%05d%s%05d%s\0", (int)mensaje.length() + (int)nickname.length() + 1, (int)mensaje.length(), mensaje.c_str(), (int)nickname.length(), nickname.c_str());
      printf("\nMensaje b a ser enviado %s\n", buffer);
      
      for (auto it = mapaSockets.cbegin(); it != mapaSockets.cend(); ++it) {
        if (mapaSockets[it->first] != mapaSockets[nickname]){
          write(mapaSockets[it->first], buffer, mensaje.length() + nickname.length() + 16);
        } 
      }
    }
    // MENSAJE f
    else if (buffer[0]=='F'){
      //printf("\nEntrando a mensaje F (archivo)\n");

      // 5B tamaño destinatario 
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoDest = atoi(buffer);

      char nombreDestino[256];
      n = read(socketCliente, nombreDestino, tamanoDest);
      nombreDestino[n] = '\0';

      // 100B tamaño nombre archvo
      n = read(socketCliente, buffer, 100);
      buffer[100] = '\0';
      long tamanoNombreArchivo = strtol(buffer, NULL, 10);

      // nombre del archivo
      char *nombreArchivoC = new char[tamanoNombreArchivo + 1];
      n = read(socketCliente, nombreArchivoC, tamanoNombreArchivo);
      nombreArchivoC[n] = '\0';
      std::string nombreArchivo = std::string(nombreArchivoC, tamanoNombreArchivo);
      delete[] nombreArchivoC;

      // 18B tamaño del archivo
      n = read(socketCliente, buffer, 18);
      buffer[18] = '\0';
      long tamanoArchivo = atol(buffer);

      // contenido del archivo
      char *datosArchivo = new char[tamanoArchivo];
      long totalLeido = 0;
      while (totalLeido < tamanoArchivo) {
          n = read(socketCliente, datosArchivo + totalLeido, tamanoArchivo - totalLeido);
          if (n <= 0) break;
          totalLeido += n;
      }

      // 5B hash
      char hashCampo[6];
      n = read(socketCliente, hashCampo, 5);
      hashCampo[5] = '\0';

      //envio de mensaje f
      int tamanoEmisor = nickname.size();
      long dataLen_f = 1           // 1B tipo 'f'
                   + 5             // 5B tamaño emisor
                   + tamanoEmisor  // nickname emisor (nickname)
                   + 100           // 100B tamaño nombre del archivo
                   + tamanoNombreArchivo   // nombre del archivo
                   + 18            // 18B tamaño del archivo
                   + tamanoArchivo // contenido del archivo
                   + 5;            // 5B hash

      long totalMensaje_f = 5 + dataLen_f;
      char *bufferEnvio = new char[totalMensaje_f];
      memset(bufferEnvio, 0, totalMensaje_f);

      // escribir 5B dataLen_f
      sprintf(bufferEnvio, "%05ld", dataLen_f);

      int posEnvio = 5;
      // escribir 1B tipo f
      bufferEnvio[posEnvio++] = 'f';
      
      // escriir 5B tamaño emisor
      {
         char tmp[6];
         sprintf(tmp, "%05d", tamanoEmisor);
         memcpy(bufferEnvio+posEnvio, tmp, 5);
         posEnvio += 5;
      }

      // escribir nickname emisor
      memcpy(bufferEnvio+posEnvio, nickname.c_str(), tamanoEmisor);
      posEnvio += tamanoEmisor;

      // esscribir 100B tamaño nombre del archivo
      {
         char tmp[101];
         sprintf(tmp, "%0100ld", tamanoNombreArchivo);
         memcpy(bufferEnvio+posEnvio, tmp, 100);
         posEnvio += 100;
      }

      // escribir nombre del archivo
      memcpy(bufferEnvio+posEnvio, nombreArchivo.c_str(), tamanoNombreArchivo);
      posEnvio += tamanoNombreArchivo;

      // escribir 18B tamaño archivo
      {
         char tmp[19];
         sprintf(tmp, "%018ld", tamanoArchivo);
         memcpy(bufferEnvio+posEnvio, tmp, 18);
         posEnvio += 18;
      }

      // escribir contenido del archivo
      memcpy(bufferEnvio+posEnvio, datosArchivo, tamanoArchivo);
      posEnvio += tamanoArchivo;

      // escribir 5B hash
      memcpy(bufferEnvio+posEnvio, hashCampo, 5);
      posEnvio += 5;

      // enviar al destinatario
      if(mapaSockets.find(nombreDestino) != mapaSockets.end()){
         int socketDestino = mapaSockets[nombreDestino];
         write(socketDestino, bufferEnvio, totalMensaje_f);
         printf("[Servidor] Reenviando archivo de %s a %s\n", nickname.c_str(), nombreDestino);
      } 
      else {
         printf("[Servidor] Destinatario %s no encontrado\n", nombreDestino);
      }
      delete[] bufferEnvio;
      delete[] datosArchivo;
    }

    // MENSAJE Q
    else if (buffer[0]=='Q'){
      printf("\nEl cliente %s ha salido del chat\n", nickname.c_str());
      break;
    }

  } while (strncmp(buffer, "exit", 4) != 0);
  shutdown(socketCliente, SHUT_RDWR);
  close(socketCliente);
  mapaSockets.erase(nickname);
}

int main(void)
{
  struct sockaddr_in direccion;
  int socketServidor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];
  int n;

  if (-1 == socketServidor)
  {
    perror("No se pudo crear el socket");
    exit(EXIT_FAILURE);
  }

  memset(&direccion, 0, sizeof(struct sockaddr_in));

  direccion.sin_family = AF_INET;
  direccion.sin_port = htons(45000);
  direccion.sin_addr.s_addr = INADDR_ANY;

  if (-1 == bind(socketServidor, (const struct sockaddr *)&direccion, sizeof(struct sockaddr_in)))
  {
    perror("Error en bind");
    close(socketServidor);
    exit(EXIT_FAILURE);
  }

  if (-1 == listen(socketServidor, 10))
  {
    perror("Error en listen");
    close(socketServidor);
    exit(EXIT_FAILURE);
  }

  for (;;)
  {
    printf("%s\n", "Esperando cliente...");
    int socketCliente = accept(socketServidor, NULL, NULL);

    if (0 > socketCliente)
    {
      perror("Error en accept");
      close(socketServidor);
      exit(EXIT_FAILURE);
    }
    n = read(socketCliente, buffer, 5);
    buffer[n] = '\0';
    int tamano = atoi(buffer);
    n = read(socketCliente, buffer, 1);
    buffer[n] = '\0';
    if (buffer[0] == 'N' ){
      n = read(socketCliente, buffer, tamano-1);
      buffer[n] = '\0';
      mapaSockets[buffer] = socketCliente;
      printf("\nSe ha conectado el Cliente: %s\n", buffer);
      std::thread(leerSocket, socketCliente, std::string(buffer)).detach();
    }
    else{
      shutdown(socketCliente, SHUT_RDWR);
      close(socketCliente);
    }
  }

  close(socketServidor);
  return 0;
}