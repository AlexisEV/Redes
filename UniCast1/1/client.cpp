/* Client code in C */

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
#include <chrono>
#include <string>
#include <atomic>
#include <limits>
#include <fstream>    // Para manejo de archivos
#include <vector>

std::string nickname;
std::atomic<bool> envLista(false);

// Función sencilla para calcular un hash (suma de bytes módulo 100000)
int calcularHash(const char* datos, long tamano) {
    int hash = 0;
    for (long i = 0; i < tamano; i++) {
        hash = (hash + (unsigned char)datos[i]) % 100000;
    }
    return hash;
}

void leerSocket(int socketCliente)
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

    // Mensaje l
    if (buffer[0]=='l'){
      n = read(socketCliente, buffer, tamano-1);
      buffer[n] = '\0';
      printf("\nUsuarios conectados: %s\n", buffer);
      envLista = true;
    }
    // Mensaje m
    else if (buffer[0]=='m'){
      std::string usuarioOrigen;
      std::string mensaje;
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoMensaje = atoi(buffer);

      n = read(socketCliente, buffer, tamanoMensaje);
      buffer[n] = '\0';
      mensaje = buffer;

      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoUsuario = atoi(buffer);

      n = read(socketCliente, buffer, tamanoUsuario);
      buffer[n] = '\0';
      usuarioOrigen = buffer;
      
      printf("\n\n%s: %s\n", usuarioOrigen.c_str(), mensaje.c_str());
    }
    // Mensaje b
    else if (buffer[0]=='b'){
      std::string usuarioOrigen;
      std::string mensaje;
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoMensaje = atoi(buffer);

      n = read(socketCliente, buffer, tamanoMensaje);
      buffer[n] = '\0';
      mensaje = buffer;

      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoUsuario = atoi(buffer);

      n = read(socketCliente, buffer, tamanoUsuario);
      buffer[n] = '\0';
      usuarioOrigen = buffer;
      
      printf("\n\n[broadcast] %s: %s\n", usuarioOrigen.c_str(), mensaje.c_str());
    }
    // Mensaje f
    else if (buffer[0]=='f'){
      std::string emisorArchivo;   
      std::string nombreArchivo;    
      char contenidoArchivo[100000]; // buffer contenido del archivo
      char hashRecibido[6];
      
      // 5B tamaño emisor
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoEmisor = atoi(buffer);

      // nickname emisor
      n = read(socketCliente, buffer, tamanoEmisor);
      buffer[n] = '\0';
      emisorArchivo = buffer;

      // 100B tamaño nombre del archivo
      n = read(socketCliente, buffer, 100);
      buffer[100] = '\0';
      long tamanoNombreArchivo = strtol(buffer, NULL, 10);

      // nombre del archivo
      char *nombreArchivoC = new char[tamanoNombreArchivo + 1];
      n = read(socketCliente, nombreArchivoC, tamanoNombreArchivo);
      nombreArchivoC[n] = '\0';
      nombreArchivo = std::string(nombreArchivoC, tamanoNombreArchivo);
      delete[] nombreArchivoC;

      // 18B tamaño del archivo
      n = read(socketCliente, buffer, 18);
      buffer[18] = '\0';
      long tamanoArchivo = atol(buffer);

      // contenido del archivo
      long totalLeido = 0;
      while (totalLeido < tamanoArchivo) {
          n = read(socketCliente, contenidoArchivo + totalLeido, tamanoArchivo - totalLeido);
          if (n <= 0) break;
          totalLeido += n;
      }

      // 5B hash
      n = read(socketCliente, hashRecibido, 5);
      hashRecibido[5] = '\0';

      // calcular hash
      int hashLocal = calcularHash(contenidoArchivo, tamanoArchivo);
      char hashCalculado[6];
      sprintf(hashCalculado, "%05d", hashLocal);

      // compara hash
      if (strcmp(hashCalculado, hashRecibido) == 0)
          printf("\n[archivo] %s: %s (Hash OK)\n", emisorArchivo.c_str(), nombreArchivo.c_str());
      else
          printf("\n[archivo] %s: %s (Hash INCORRECTO: calculado %s, recibido %s)\n",
                 emisorArchivo.c_str(), nombreArchivo.c_str(), hashCalculado, hashRecibido);

      // guardar archivo
      if (strcmp(hashCalculado, hashRecibido) == 0) {
          std::ofstream archivoSalida(nombreArchivo, std::ios::binary);
          if (!archivoSalida.is_open()){
              std::cerr << "No se pudo crear el archivo " << nombreArchivo << " en disco.\n";
          } else {
              archivoSalida.write(contenidoArchivo, tamanoArchivo);
              archivoSalida.close();
              std::cout << "Archivo " << nombreArchivo << " guardado en la carpeta actual.\n";
          }
      }
    }

  } while (strncmp(buffer, "exit", 4) != 0);
}

int main(void)
{
  char buffer[256];
  struct sockaddr_in direccion;
  int resultado;
  int socketCliente = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  int n;

  if (-1 == socketCliente)
  {
    perror("No se pudo crear el socket");
    exit(EXIT_FAILURE);
  }

  memset(&direccion, 0, sizeof(struct sockaddr_in));

  direccion.sin_family = AF_INET;
  direccion.sin_port = htons(45000);
  resultado = inet_pton(AF_INET, "127.0.0.1", &direccion.sin_addr);

  if (0 > resultado)
  {
    perror("error: primer parámetro no es una familia de direcciones válida");
    close(socketCliente);
    exit(EXIT_FAILURE);
  }
  else if (0 == resultado)
  {
    perror("la cadena (segundo parámetro) no contiene una dirección IP válida");
    close(socketCliente);
    exit(EXIT_FAILURE);
  }

  if (-1 == connect(socketCliente, (const struct sockaddr *)&direccion, sizeof(struct sockaddr_in)))
  {
    perror("falló el connect");
    close(socketCliente);
    exit(EXIT_FAILURE);
  }

  // Mensaje N para registro
  std::cout << "Ingresa tu nickname: ";
  std::getline(std::cin, nickname);
  sprintf(buffer, "%05dN%s\0", (int)nickname.length() + 1, nickname.c_str());
  write(socketCliente, buffer, nickname.length() + 1 + 5);
  std::thread(leerSocket, socketCliente).detach();

  bool salir = false;
  while (!salir)
  {
    // Menú
    int opcion;
    std::cout << "\n=== MENÚ PRINCIPAL ===" << std::endl;
    std::cout << "1. Ver lista de usuarios conectados" << std::endl;
    std::cout << "2. Mandar mensaje" << std::endl;
    std::cout << "3. Salir" << std::endl;
    std::cout << "4. Mandar mensaje broadcast" << std::endl;
    std::cout << "5. Mandar archivo" << std::endl;
    std::cout << "Seleccione una opción: ";
    std::cin >> opcion;

    // Mensaje L
    if (opcion == 1)
    {
      strcpy(buffer, "00001L"); 
      buffer[6] = '\0';
      write(socketCliente, buffer, 6);
      while(!envLista);
      envLista = false;
    }
    // Mensaje M 
    else if (opcion == 2)
    {
      std::string destino;
      std::string mensaje;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "\nNombre Destinatario: ";
      std::getline(std::cin, destino);
      std::cout << "\nMensaje: ";
      std::getline(std::cin, mensaje);
      sprintf(buffer, "%05dM%05d%s%05d%s", (int)(destino.length() + mensaje.length() + 1),
              (int)mensaje.length(), mensaje.c_str(), (int)destino.length(), destino.c_str());
      write(socketCliente, buffer, mensaje.length() + destino.length() + 16); // 16 = 5+5+5+1
    }
    // Mensaje Q 
    else if (opcion == 3)
    {
      strcpy(buffer, "00001Q");
      buffer[6] = '\0';
      write(socketCliente, buffer, 6);
      salir = true;
    }
    // Mensaje B
    else if (opcion == 4)
    {
      std::string mensaje;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "\nMensaje: ";
      std::getline(std::cin, mensaje);
      sprintf(buffer, "%05dB%05d%s", (int)(mensaje.length() + 1), (int)mensaje.length(), mensaje.c_str());
      printf("Mensaje B a enviar: %s\n", buffer);
      write(socketCliente, buffer, mensaje.length() + 11); // 11 = 5+1+5
    }
    // Mensaje F
    else if (opcion == 5)
    {
      std::string destino;
      std::string rutaArchivo;
      std::string nombreArchivo;
      
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      std::cout << "\nNombre Destinatario: ";
      std::getline(std::cin, destino);
      std::cout << "Ruta del archivo a enviar: ";
      std::getline(std::cin, rutaArchivo);

      // nombre del archivo sin ruta
      size_t posBarra = rutaArchivo.find_last_of("/\\");
      if (posBarra != std::string::npos){
        nombreArchivo = rutaArchivo.substr(posBarra + 1);
      }  
      else{
        nombreArchivo = rutaArchivo;
      }

      // abre el archivo en modo binario y se lee
      std::ifstream archivoEntrada(rutaArchivo, std::ios::in | std::ios::binary);
      if (!archivoEntrada.is_open()){
         std::cerr << "No se pudo abrir el archivo " << rutaArchivo << "\n";
         continue;
      }
      std::vector<char> datosArchivo((std::istreambuf_iterator<char>(archivoEntrada)),
                                     std::istreambuf_iterator<char>());
      long tamanoArchivo = datosArchivo.size();
      archivoEntrada.close();

      // hash del archivo
      int valorHash = calcularHash(datosArchivo.data(), tamanoArchivo);
      char hashStr[6];
      sprintf(hashStr, "%05d", valorHash);

      // campo nombre archivo
      long tamanoNomArchivo = nombreArchivo.size();
      char campoTamanoNombre[101];
      sprintf(campoTamanoNombre, "%0100ld", tamanoNomArchivo);

      // tamaños
      long tamanoDestino = destino.size();
      long dataLen = 1 + 5 + tamanoDestino + 100 + tamanoNomArchivo + 18 + tamanoArchivo + 5;
      long tamanoMensajeTotal = 5 + dataLen;
      char *bufferEnvio = new char[tamanoMensajeTotal];
      memset(bufferEnvio, 0, tamanoMensajeTotal);

      // 5B dataLen
      sprintf(bufferEnvio, "%05ld", dataLen);
      int posEnvio = 5;

      // 1B tipo F
      bufferEnvio[posEnvio++] = 'F';

      // 5B tamaño nickaname destinatario
      {
         char tmp[10];
         sprintf(tmp, "%05ld", tamanoDestino);
         memcpy(bufferEnvio+posEnvio, tmp, 5);
         posEnvio += 5;
      }

      // nickname destinatario
      memcpy(bufferEnvio+posEnvio, destino.c_str(), tamanoDestino);
      posEnvio += tamanoDestino;

      // 100B longitud rchivo
      memcpy(bufferEnvio+posEnvio, campoTamanoNombre, 100);
      posEnvio += 100;

      // nombre archivo
      memcpy(bufferEnvio+posEnvio, nombreArchivo.c_str(), tamanoNomArchivo);
      posEnvio += tamanoNomArchivo;

      // 18B tamaño archivo
      {
         char tmp[19];
         sprintf(tmp, "%018ld", tamanoArchivo);
         memcpy(bufferEnvio+posEnvio, tmp, 18);
         posEnvio += 18;
      }

      // Contenido archivo
      memcpy(bufferEnvio+posEnvio, datosArchivo.data(), tamanoArchivo);
      posEnvio += tamanoArchivo;

      // 5B hash
      memcpy(bufferEnvio+posEnvio, hashStr, 5);
      posEnvio += 5;

      //envio
      write(socketCliente, bufferEnvio, tamanoMensajeTotal);
      std::cout << "Archivo enviado.\n";
      delete[] bufferEnvio;
    }
    else
    {
      std::cout << "Opción no válida." << std::endl;
    }
  }
  
  shutdown(socketCliente, SHUT_RDWR);
  close(socketCliente);
  return 0;
}