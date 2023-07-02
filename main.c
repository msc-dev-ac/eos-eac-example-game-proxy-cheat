#include <dlfcn.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "eos.h"

// Since loading will be done via /etc/ld.so.preload, this boolean indicates the
// instance is loaded into the correct target process.
int enable = 0;

pthread_t server_thread;
int proxy_client_socket = -1;
int server_socket = -1;

// Type definitions for hooked functions to call original if needed.
typedef ssize_t (*read_t)(int fd, void *buf, size_t count);

// Nullify the anti-cheat functionality on the bad client.
EOS_HAntiCheatClient
EOS_Platform_GetAntiCheatClientInterface(EOS_HPlatform Handle) {
  printf("[PROXY CHEAT] %s\n", "BLOCKED get anti-cheat interface!");
  return (void *)1;
}
EOS_EResult
EOS_AntiCheatClient_ReceiveMessageFromServer(EOS_HAntiCheatClient Handle,
                                             const void *Options) {
  printf("[PROXY CHEAT] %s\n", "BLOCKED recv message!");
  return 0;
}

EOS_NotificationId EOS_AntiCheatClient_AddNotifyMessageToServer(
    EOS_HAntiCheatClient Handle, const void *Options, void *ClientData,
    EOS_AntiCheatClient_OnMessageToServerCallback NotificationFn) {
  printf("[PROXY CHEAT] %s\n", "BLOCKED add send message callback!");
  return 1;
}

EOS_NotificationId EOS_AntiCheatClient_AddNotifyClientIntegrityViolated(
    EOS_HAntiCheatClient Handle, const void *Options, void *ClientData,
    EOS_AntiCheatClient_OnClientIntegrityViolatedCallback NotificationFn) {
  printf("[PROXY CHEAT] %s\n", "BLOCKED add integrity callback!");
  return 1;
}

EOS_EResult EOS_AntiCheatClient_BeginSession(EOS_HAntiCheatClient Handle,
                                             const void *Options) {
  printf("[PROXY CHEAT] %s\n", "BLOCKED begin session!");
  return 0;
}

// Hook read to forward messages from server to good client
ssize_t read(int fd, void *buf, size_t count) {
  // perform actual read first to get message 
  read_t o_read = (read_t)dlsym(RTLD_NEXT, "read");
  ssize_t ret = (*o_read)(fd, buf, count);
  if(0 == enable) {
    return ret;
  }
  // only handle EAC messages
  if (0 == strncmp(buf, "MSG", 3)) {
    // save the server socket for future use
    server_socket = fd;
    if (-1 == proxy_client_socket) {
      printf("[PROXY CHEAT] %s\n",
             "SKIPPED FORWARD without good client socket");
    } else {
      // reconstruct packet
      void *packet = malloc(sizeof(ssize_t) + count);
      if (NULL == packet) {
        printf("[PROXY CHEAT] %s\n", "Failed to malloc packet to send.");
        return ret;
      }
      char *ptr = (char *)packet;
      memcpy(ptr, &count, sizeof(ssize_t));
      ptr += sizeof(ssize_t);
      memcpy(ptr, buf, count);

      // send to good client
      ssize_t len =
          send(proxy_client_socket, packet, sizeof(ssize_t) + count, 0);
      if (len != sizeof(ssize_t) + count) {
        printf("[PROXY CHEAT] %s\n", "Sent incomplete packet to good client");
        free(packet);
        return ret;
      }

      free(packet);
    }
  }
  return ret;
}

// Setup receiving socket for good client to connect to and then forward all EAC messages to the server.
void *proxy_server_run(void *arg) {

  // Don't use the hooked function!
  read_t o_read = (read_t)dlsym(RTLD_NEXT, "read");

  struct sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_port = htons(4321);
  bind_addr.sin_addr.s_addr = INADDR_ANY;

  int bind_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (0 > bind_socket) {
    printf("[PROXY CHEAT] %s\n", "Failed to create socket");
    return NULL;
  }

  if (0 > bind(bind_socket, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) {
    printf("[PROXY CHEAT] %s\n", "Failed to bind socket");
    return NULL;
  }

  if (0 != (listen(bind_socket, 5))) {
    printf("[PROXY CHEAT] %s\n", "Failed to listen on socket");
    return NULL;
  }

  struct sockaddr_in client_addr;
  uint32_t addr_len = sizeof(struct sockaddr_in);
  proxy_client_socket =
      accept(bind_socket, (struct sockaddr *)&client_addr, &addr_len);
  if (-1 == proxy_client_socket) {
    printf("[PROXY CHEAT] %s\n", "Failed to accept connection on socket");
    return NULL;
  }

  printf("[PROXY CHEAT] %s\n", "Connection established with good client");

  // Forward received EAC messages to the server
  while (1) {
    // Receive and unpack message
    ssize_t data_size = 0;
    ssize_t len_read =
        (*o_read)(proxy_client_socket, &data_size, sizeof(data_size));
    if (0 == len_read) {
      return NULL;
    }

    if (sizeof(data_size) != len_read) {
      printf("[PROXY CHEAT] %s\n",
             "Received incomplete size packet from good client");
      return NULL;
    }
    void *data = malloc(data_size);
    if (NULL == data) {
      printf("[PROXY CHEAT] %s\n",
             "Failed to malloc data received from good cient");
      return NULL;
    }

    ssize_t data_len = (*o_read)(proxy_client_socket, data, data_size);
    if (data_len != data_size) {
      printf("[PROXY CHEAT] %s\n",
             "Received incomplete data packet from good client");
      free(data);
      return NULL;
    }

    // Filter out non-EAC messages
    if (0 != strncmp(data, "MSG", 3)) {
      printf("[PROXY CHEAT] %s\n", "SKIPPING non-EAC message");
      continue;
    }

    // Repack and forward message to server
    void *packet = malloc(sizeof(ssize_t) + data_size);
    if (NULL == packet) {
      printf("[PROXY CHEAT] %s\n", "Failed to malloc packet to send.");
      return NULL;
    }

    char *ptr = (char *)packet;
    memcpy(ptr, &data_size, sizeof(ssize_t));
    ptr += sizeof(ssize_t);
    memcpy(ptr, data, data_size);

    ssize_t len_sent =
        send(server_socket, packet, sizeof(ssize_t) + data_size, 0);
    if (len_sent != sizeof(ssize_t) + data_size) {
      printf("[PROXY CHEAT] %s\n", "Sent incomplete packet to server socket");
      free(packet);
      return NULL;
    }

    free(packet);
  }

  return NULL;
}

__attribute__((constructor)) void init() {
  char filename[1024];
  FILE *f = fopen("/proc/self/comm", "r");
  fread(filename, sizeof(filename), 1, f);
  fclose(f);

  // Check we are loaded into the target program
  if (0 == strncmp(filename, "MyEOSGame", 9)) {
    enable = 1;
    server_thread = pthread_create(&server_thread, NULL, proxy_server_run, NULL);
  }
}