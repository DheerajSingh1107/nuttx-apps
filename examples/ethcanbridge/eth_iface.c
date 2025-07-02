// eth_iface.c: TCP server
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "eth_iface.h"
#include "bridge.h"

#define PORT 2525

static int client_fd = -1;

static void *eth_server_thread(void *arg)
{
    int server_fd;
    struct sockaddr_in addr = {0};
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 1);

    printf("[ETH] Listening on port %d...\n", PORT);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    uint8_t buf[64];
    while (1)
    {
        ssize_t len = read(client_fd, buf, sizeof(buf));

        if (len > 0)
          {
            //printf("[ETH] data on port %d len : %d...\n", PORT,len);
            if (send(client_fd, buf, len, 0) != len)
              {
                printf("ERROR: send failed");
                break;
              }
          }
    }

    close(client_fd);
    close(server_fd);
    return NULL;
}

void eth_server_start(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, eth_server_thread, NULL);
}

void eth_send(const uint8_t *data, size_t len)
{
    if (client_fd >= 0)
        write(client_fd, data, len);
}
