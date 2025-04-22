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
#include <vector>

using namespace std;

std::map<string, int> mapaSockets;

struct Partida
{
  char tablero[9] = {'_', '_', '_', '_', '_', '_', '_', '_', '_'};
  string jugadorO; // quien empieza
  string jugadorX;
  vector<string> espectadores;
  char turno = 'O';
  bool activa = false;
} partida;

string jugadorEnEspera; // 1er J que llegó y espera rival

/* Forward declarations */
void enviarM(int sock, const string &msg); // m
void enviarX_aTodos();
void enviarT(const string &nick, char simbolo);
void finalizarPartida(char resultado); // 'W','L','E'
bool ganador(char s);
bool tableroLleno();

void leerSocket(int socketCliente, string nickname)
{
  char *buffer = (char *)malloc(10);
  int n, tamano;
  do
  {
    n = read(socketCliente, buffer, 5);
    if (n <= 0)
      break;
    buffer[n] = '\0';
    tamano = atoi(buffer);

    n = read(socketCliente, buffer, 1);
    if (n <= 0)
      break;
    buffer[n] = '\0';

    buffer = (char *)realloc(buffer, tamano + 1);
    // MENSAJE l
    if (buffer[0] == 'L')
    {
      std::string mensaje;
      std::string coma = "";
      for (auto it = mapaSockets.cbegin(); it != mapaSockets.cend(); ++it)
      {
        if ((*it).first != nickname)
        {
          mensaje += coma + (*it).first;
          coma = ",";
        }
      }
      sprintf(buffer, "%05dl%s\0", (int)mensaje.length() + 1, mensaje.c_str());
      write(mapaSockets[nickname], buffer, mensaje.length() + 6); // 6 = 5+1
    }
    // MENSAJE m
    else if (buffer[0] == 'M')
    {
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
      sprintf(buffer, "%05dm%05d%s%05d%s\0", (int)mensaje.length() + (int)nickname.length() + 1 + 5 + 5, (int)mensaje.length(), mensaje.c_str(), (int)nickname.length(), nickname.c_str());
      write(mapaSockets[destino], buffer, mensaje.length() + nickname.length() + 16);
    }
    // MENSAJE b
    else if (buffer[0] == 'B')
    {
      printf("\nEntrando a mensaje B\n");
      std::string mensaje;
      n = read(socketCliente, buffer, 5);
      buffer[n] = '\0';
      int tamanoMensaje = atoi(buffer);
      n = read(socketCliente, buffer, tamanoMensaje);
      buffer[n] = '\0';
      mensaje = buffer;
      sprintf(buffer, "%05db%05d%s%05d%s\0", (int)mensaje.length() + (int)nickname.length() + 1 + 5 + 5, (int)mensaje.length(), mensaje.c_str(), (int)nickname.length(), nickname.c_str());
      printf("\nMensaje b a ser enviado %s\n", buffer);

      for (auto it = mapaSockets.cbegin(); it != mapaSockets.cend(); ++it)
      {
        if (mapaSockets[it->first] != mapaSockets[nickname])
        {
          write(mapaSockets[it->first], buffer, mensaje.length() + nickname.length() + 16);
        }
      }
    }
    // MENSAJE f
    else if (buffer[0] == 'F')
    {
      // printf("\nEntrando a mensaje F (archivo)\n");

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
      while (totalLeido < tamanoArchivo)
      {
        n = read(socketCliente, datosArchivo + totalLeido, tamanoArchivo - totalLeido);
        if (n <= 0)
          break;
        totalLeido += n;
      }

      // 5B hash
      char hashCampo[6];
      n = read(socketCliente, hashCampo, 5);
      hashCampo[5] = '\0';

      // envio de mensaje f
      int tamanoEmisor = nickname.size();
      long dataLen_f = 1                     // 1B tipo 'f'
                       + 5                   // 5B tamaño emisor
                       + tamanoEmisor        // nickname emisor (nickname)
                       + 100                 // 100B tamaño nombre del archivo
                       + tamanoNombreArchivo // nombre del archivo
                       + 18                  // 18B tamaño del archivo
                       + tamanoArchivo       // contenido del archivo
                       + 5;                  // 5B hash

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
        memcpy(bufferEnvio + posEnvio, tmp, 5);
        posEnvio += 5;
      }

      // escribir nickname emisor
      memcpy(bufferEnvio + posEnvio, nickname.c_str(), tamanoEmisor);
      posEnvio += tamanoEmisor;

      // esscribir 100B tamaño nombre del archivo
      {
        char tmp[101];
        sprintf(tmp, "%0100ld", tamanoNombreArchivo);
        memcpy(bufferEnvio + posEnvio, tmp, 100);
        posEnvio += 100;
      }

      // escribir nombre del archivo
      memcpy(bufferEnvio + posEnvio, nombreArchivo.c_str(), tamanoNombreArchivo);
      posEnvio += tamanoNombreArchivo;

      // escribir 18B tamaño archivo
      {
        char tmp[19];
        sprintf(tmp, "%018ld", tamanoArchivo);
        memcpy(bufferEnvio + posEnvio, tmp, 18);
        posEnvio += 18;
      }

      // escribir contenido del archivo
      memcpy(bufferEnvio + posEnvio, datosArchivo, tamanoArchivo);
      posEnvio += tamanoArchivo;

      // escribir 5B hash
      memcpy(bufferEnvio + posEnvio, hashCampo, 5);
      posEnvio += 5;

      // enviar al destinatario
      if (mapaSockets.find(nombreDestino) != mapaSockets.end())
      {
        int socketDestino = mapaSockets[nombreDestino];
        write(socketDestino, bufferEnvio, totalMensaje_f);
        printf("[Servidor] Reenviando archivo de %s a %s\n", nickname.c_str(), nombreDestino);
      }
      else
      {
        printf("[Servidor] Destinatario %s no encontrado\n", nombreDestino);
      }
      delete[] bufferEnvio;
      delete[] datosArchivo;
    }

    // MENSAJE Q
    else if (buffer[0] == 'Q')
    {
      printf("\nEl cliente %s ha salido del chat\n", nickname.c_str());
      break;
    }

    // MENSAJE J
    else if (buffer[0] == 'J')
    {
      if (!partida.activa && jugadorEnEspera.empty())
      { // 1er jugador
        jugadorEnEspera = nickname;
        enviarM(socketCliente, "wait for player");
      }
      else if (!partida.activa && !jugadorEnEspera.empty())
      { // 2do jugador
        partida.activa = true;
        partida.jugadorO = jugadorEnEspera;
        partida.jugadorX = nickname;
        jugadorEnEspera = "";

        // m “inicio” a ambos
        enviarM(mapaSockets[partida.jugadorO], "inicio");
        enviarM(mapaSockets[partida.jugadorX], "inicio");

        enviarX_aTodos();               // X tablero vacío
        enviarT(partida.jugadorO, 'O'); // primer turno
      }
      else
      { // espectador
        enviarM(socketCliente, "do you want to see?");
      }
    }

    // MENSAJE V
    else if (buffer[0] == 'V')
    {
      if (partida.activa)
        partida.espectadores.push_back(nickname);
      char len[6];
      sprintf(len, "00010"); // 1+9 = 10
      string pkt(len);
      pkt += 'X';
      pkt.append(partida.tablero, 9);
      write(mapaSockets[nickname], pkt.c_str(), 15);
    }

    // MENSAJE P
    else if (buffer[0] == 'P')
    {
      char pos, simb;
      read(socketCliente, &pos, 1);
      read(socketCliente, &simb, 1);

      int idx = pos - '1'; // 0‑8
      if (idx < 0 || idx > 8 || partida.tablero[idx] != '_')
      {
        // E6 Posicion ocupada
        const string err = "00018E600016Posicion ocupada";
        write(socketCliente, err.c_str(), err.size());
        continue;
      }

      partida.tablero[idx] = simb; // actualizar tablero

      // ganador o empate
      if (ganador(simb))
      {
        enviarX_aTodos();
        // O a ambos jugadores y espectadores
        char resO = (simb == 'O') ? 'W' : 'L';
        char resX = (simb == 'X') ? 'W' : 'L';
        string msgO = "00002O";
        msgO += resO;
        string msgX = "00002O";
        msgX += resX;
        write(mapaSockets[partida.jugadorO], msgO.c_str(), 7);
        write(mapaSockets[partida.jugadorX], msgX.c_str(), 7);
        for (auto &esp : partida.espectadores)
          write(mapaSockets[esp], "00002OE", 7);
        partida = Partida(); // reset
      }
      else if (tableroLleno())
      { // empate
        enviarX_aTodos();
        for (auto nick : {partida.jugadorO, partida.jugadorX})
          write(mapaSockets[nick], "00002OE", 7);
        for (auto &esp : partida.espectadores)
          write(mapaSockets[esp], "00002OE", 7);
        partida = Partida();
      }
      else
      { // juego continúa
        enviarX_aTodos();
        partida.turno = (partida.turno == 'O') ? 'X' : 'O';
        const string &prox = (partida.turno == 'O') ? partida.jugadorO
                                                    : partida.jugadorX;
        enviarT(prox, partida.turno);
      }
    }

  } while (strncmp(buffer, "exit", 4) != 0);
  shutdown(socketCliente, SHUT_RDWR);
  close(socketCliente);
  mapaSockets.erase(nickname);
}

//----func auxiliares para tic tac toe--------------

void enviarM(int sock, const std::string &msg)
{
  const std::string remitente = "servidor"; // <- remitente fijo
  // tamaños
  const int lenMsg = static_cast<int>(msg.size());
  const int lenEm = static_cast<int>(remitente.size());
  const int dataLen = 1 + lenMsg + 5 + lenEm;

  // campos de tamaños
  char campoTotal[6], campoLenMsg[6], campoLenEm[6];
  sprintf(campoTotal, "%05d", dataLen);
  sprintf(campoLenMsg, "%05d", lenMsg);
  sprintf(campoLenEm, "%05d", lenEm);

  // construccion mensaje
  std::string pkt(campoTotal, 5); // 5B tamaño total
  pkt += 'm';                     // 1B tipo
  pkt.append(campoLenMsg, 5);     // 5B tam mensaje
  pkt += msg;                     // n  mensaje
  pkt.append(campoLenEm, 5);      // 5B tam remitente
  pkt += remitente;               // k  remitente

  write(sock, pkt.c_str(), pkt.size());
}

void enviarX_aTodos()
{
  char len[6];
  sprintf(len, "00010"); // 1+9 = 10
  string pkt(len);
  pkt += 'X';
  pkt.append(partida.tablero, 9);
  for (auto nick : {partida.jugadorO, partida.jugadorX})
    if (!nick.empty())
    {
      write(mapaSockets[nick], pkt.c_str(), 15);
    }
  for (auto &esp : partida.espectadores)
    write(mapaSockets[esp], pkt.c_str(), 15);
}

void enviarT(const string &nick, char simbolo)
{
  string pkt = "00002T";
  pkt += simbolo;
  write(mapaSockets[nick], pkt.c_str(), 7);
}

bool linea(int a, int b, int c, char s)
{
  return partida.tablero[a] == s && partida.tablero[b] == s && partida.tablero[c] == s;
}
bool ganador(char s)
{
  return linea(0, 1, 2, s) || linea(3, 4, 5, s) || linea(6, 7, 8, s) ||
         linea(0, 3, 6, s) || linea(1, 4, 7, s) || linea(2, 5, 8, s) ||
         linea(0, 4, 8, s) || linea(2, 4, 6, s);
}
bool tableroLleno()
{
  for (char c : partida.tablero)
    if (c == '_')
      return false;
  return true;
}

//----------------------------------------------

int main(void)
{
  struct sockaddr_in direccion;
  int socketServidor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char *buffer = (char *)malloc(10);
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
    buffer = (char *)realloc(buffer, tamano + 1);

    n = read(socketCliente, buffer, 1);
    buffer[n] = '\0';
    if (buffer[0] == 'N')
    {
      n = read(socketCliente, buffer, tamano - 1);
      buffer[n] = '\0';
      mapaSockets[buffer] = socketCliente;
      printf("\nSe ha conectado el Cliente: %s\n", buffer);
      std::thread(leerSocket, socketCliente, std::string(buffer)).detach();
    }
    else
    {
      shutdown(socketCliente, SHUT_RDWR);
      close(socketCliente);
    }
  }
  free(buffer);
  close(socketServidor);
  return 0;
}