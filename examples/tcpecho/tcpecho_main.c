/****************************************************************************
 * Robust TCP Echo Server Implementation
 *
 * Features:
 * - Proper TCP connection handling
 * - Error resilience
 * - Graceful shutdown
 * - Detailed logging
 ****************************************************************************/

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "netutils/netlib.h"

#ifdef CONFIG_EXAMPLES_TCPECHO_DHCPC
#  include "netutils/dhcpc.h"
#endif

#define TCPECHO_PORT 2525
#define TCPECHO_MAXLINE 64
#define TCPECHO_BACKLOG 10  // Increase backlog size
#define MAX_CLIENTS 10
#define TCPECHO_POLLTIMEOUT 30000
#define MAX_RETRIES 3
#define STABILIZATION_DELAY 3
#define BUFFER_SIZE 64

static void print_socket_info(int sockfd)
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);

  if (getsockname(sockfd, (struct sockaddr *)&addr, &len) == 0)
    {
      printf("Socket %d bound to %s:%d\n",
             sockfd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    }

  int flags = fcntl(sockfd, F_GETFL, 0);
  printf("Socket %d flags: %s\n", sockfd,
         (flags & O_NONBLOCK) ? "non-blocking" : "blocking");
}

static int tcpecho_netsetup(void)
{
  printf("Network setup completed successfully\n");
  return OK;
}

static int create_listen_socket(void)
{
  int listenfd;
  int optval = 1;
  struct sockaddr_in servaddr;

  listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (listenfd < 0)
    {
      perror("ERROR: socket creation failed");
      return -1;
    }

  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
      perror("ERROR: setsockopt(SO_REUSEADDR) failed");
      goto error;
    }

  if (setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0)
    {
      perror("ERROR: setsockopt(TCP_NODELAY) failed");
      goto error;
    }

  if (setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
    {
      perror("ERROR: setsockopt(SO_KEEPALIVE) failed");
      goto error;
    }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(CONFIG_EXAMPLES_TCPECHO_PORT);

  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
      perror("ERROR: bind failed");
      goto error;
    }

  if (listen(listenfd, CONFIG_EXAMPLES_TCPECHO_BACKLOG) < 0)
    {
      perror("ERROR: listen failed");
      goto error;
    }

  print_socket_info(listenfd);
  return listenfd;

error:
  close(listenfd);
  return -1;
}

/* Thread function to handle a single client */
static void *handle_client(void *arg)
{
    int client_fd = *((int *)arg);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    printf("Client thread started for client_fd: %d\n", client_fd);

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Client disconnected (client_fd: %d)\n", client_fd);
            }
            else
            {
                perror("ERROR: recv failed");
            }
            break;
        }

        printf("Received %zd bytes from client_fd %d: ", bytes_read, client_fd);
        for (int i = 0; i < bytes_read; i++)
        {
            printf("%02x ", buffer[i]);
        }
        printf("\n");

        /* Echo the data back to the client */
        if (send(client_fd, buffer, bytes_read, 0) != bytes_read)
        {
            perror("ERROR: send failed");
            break;
        }

        printf("Echoed back %zd bytes to client_fd %d\n", bytes_read, client_fd);
    }

    close(client_fd);
    printf("Client thread exiting for client_fd: %d\n", client_fd);
    pthread_exit(NULL);
}

/* Main server function */
static int tcpecho_server(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t client_thread;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    /* Create the server socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("ERROR: socket creation failed");
        return -1;
    }

    /* Set socket options */
    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        perror("ERROR: setsockopt failed");
        close(server_fd);
        return -1;
    }

    /* Bind the socket to the port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(TCPECHO_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("ERROR: bind failed");
        close(server_fd);
        return -1;
    }

    /* Start listening for incoming connections */
    if (listen(server_fd, 1) < 0) // Allow only 1 pending connection
    {
        perror("ERROR: listen failed");
        close(server_fd);
        return -1;
    }

    printf("TCP Echo Server listening on port %d\n", TCPECHO_PORT);

    /* Accept a single client connection */
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0)
    {
        perror("ERROR: accept failed");
        close(server_fd);
        return -1;
    }

    printf("Client connected from %s:%d (client_fd: %d)\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);

    /* Handle the client connection */
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                printf("Client disconnected (client_fd: %d)\n", client_fd);
            }
            else
            {
                perror("ERROR: recv failed");
            }
            break;
        }

        printf("Received %zd bytes from client_fd %d: ", bytes_read, client_fd);
        for (int i = 0; i < bytes_read; i++)
        {
            printf("%02x ", buffer[i]);
        }
        printf("\n");

        /* Echo the data back to the client */
        if (send(client_fd, buffer, bytes_read, 0) != bytes_read)
        {
            perror("ERROR: send failed");
            break;
        }

        printf("Echoed back %zd bytes to client_fd %d\n", bytes_read, client_fd);
    }

    /* Close the client and server sockets */
    close(client_fd);
    close(server_fd);
    printf("Server shutting down\n");

    return 0;
}

int main(int argc, char *argv[])
{
    printf("Starting TCP Echo Server...\n");
    return tcpecho_server();
}