#ifndef UNICODE
#define UNICODE
#endif

#define _CRT_SECURE_NO_WARNINGS // Suppress warnings for fopen, sprintf, sscanf
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "8080"

void send_response(SOCKET ClientSocket, const char *header, const char *body,
                   int body_len) {
  send(ClientSocket, header, (int)strlen(header), 0);
  if (body && body_len > 0) {
    send(ClientSocket, body, body_len, 0);
  }
}

void serve_binary_file(SOCKET ClientSocket, const char *path,
                       const char *content_type) {
  char full_path[MAX_PATH];
  _snprintf(full_path, sizeof(full_path), "www%s", path);

  FILE *file = fopen(full_path, "rb");
  if (!file) {
    const char *not_found =
        "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send(ClientSocket, not_found, (int)strlen(not_found), 0);
    return;
  }

  fseek(file, 0, SEEK_END);
  int file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(file_size);
  fread(buffer, 1, file_size, file);
  fclose(file);

  char header[512];
  _snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",
            content_type, file_size);
  send_response(ClientSocket, header, buffer, file_size);
  free(buffer);
}

void serve_file(SOCKET ClientSocket, const char *path,
                const char *content_type) {
  serve_binary_file(ClientSocket, path, content_type);
}

int main() {
  WSADATA wsaData;
  int iResult;

  SOCKET ListenSocket = INVALID_SOCKET;
  SOCKET ClientSocket = INVALID_SOCKET;

  struct addrinfo *result = NULL;
  struct addrinfo hints;

  char recvbuf[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    return 1;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the server address and port
  iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  // Create a SOCKET for the server to listen for client connections.
  ListenSocket =
      socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET) {
    printf("socket failed with error: %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // Setup the TCP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  freeaddrinfo(result);

  iResult = listen(ListenSocket, SOMAXCONN);
  if (iResult == SOCKET_ERROR) {
    printf("listen failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  printf("Server started at http://localhost:%s\n", DEFAULT_PORT);

  while (1) {
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
      printf("accept failed with error: %d\n", WSAGetLastError());
      closesocket(ListenSocket);
      WSACleanup();
      return 1;
    }

    iResult = recv(ClientSocket, recvbuf, recvbuflen - 1, 0);
    if (iResult > 0) {
      recvbuf[iResult] = '\0';
      printf("Request:\n%s\n", recvbuf);

      char method[16] = {0}, path[256] = {0};
      if (sscanf(recvbuf, "%15s %255s", method, path) < 2) {
        closesocket(ClientSocket);
        continue;
      }

      if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
          serve_file(ClientSocket, "/", "text/html");
        } else if (strcmp(path, "/style.css") == 0) {
          serve_file(ClientSocket, "/style.css", "text/css");
        } else if (strcmp(path, "/menu") == 0) {
          serve_file(ClientSocket, "/menu.html", "text/html");
        } else if (strcmp(path, "/flights") == 0) {
          serve_file(ClientSocket, "/flights.html", "text/html");
        } else if (strcmp(path, "/book") == 0) {
          serve_file(ClientSocket, "/book.html", "text/html");
        } else if (strcmp(path, "/confirm") == 0) {
          serve_file(ClientSocket, "/confirm.html", "text/html");
        } else if (strcmp(path, "/cancel") == 0) {
          serve_file(ClientSocket, "/cancel.html", "text/html");
        } else if (strcmp(path, "/cancel_confirm") == 0) {
          serve_file(ClientSocket, "/cancel_confirm.html", "text/html");
        } else if (strstr(path, ".jpg") || strstr(path, ".png")) {
          const char *ext = strrchr(path, '.');
          char mime[32];
          sprintf(mime, "image/%s", ext + 1);
          serve_binary_file(ClientSocket, path, mime);
        } else if (strcmp(path, "/logout") == 0) {
          const char *redirect = "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
          send(ClientSocket, redirect, (int)strlen(redirect), 0);
        } else {
          serve_file(ClientSocket, path, "text/plain");
        }
      } else if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/login") == 0) {
          const char *redirect =
              "HTTP/1.1 302 Found\r\nLocation: /menu\r\n\r\n";
          send(ClientSocket, redirect, (int)strlen(redirect), 0);
        } else if (strcmp(path, "/book") == 0) {
          const char *redirect =
              "HTTP/1.1 302 Found\r\nLocation: /confirm\r\n\r\n";
          send(ClientSocket, redirect, (int)strlen(redirect), 0);
        } else if (strcmp(path, "/cancel") == 0) {
          const char *redirect =
              "HTTP/1.1 302 Found\r\nLocation: /cancel_confirm\r\n\r\n";
          send(ClientSocket, redirect, (int)strlen(redirect), 0);
        }
      }
    }
    closesocket(ClientSocket);
  }

  closesocket(ListenSocket);
  WSACleanup();

  return 0;
}
