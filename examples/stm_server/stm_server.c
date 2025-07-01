#include <nuttx/config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <poll.h>

#define PORT 2525
#define BUFFER_SIZE 1024
#define POLL_TIMEOUT_MS 5000  // Timeout in milliseconds

static void *stm_server_thread(void *arg)
{
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    char buffer[BUFFER_SIZE];
    int n;

    /* Create a TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        return NULL;
    }

    /* Set up the server address structure and bind */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        close(sockfd);
        return NULL;
    }

    /* Listen for incoming connections */
    if (listen(sockfd, 5) < 0)
    {
        perror("ERROR on listen");
        close(sockfd);
        return NULL;
    }

    printf("[STM Server] Listening on port %d...\n", PORT);
    fflush(stdout);
    clilen = sizeof(cli_addr);

    while (1)
    {
        /* Wait for an incoming connection */
        printf("[STM Server] Waiting for a connection...\n");
        fflush(stdout);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            fflush(stdout);
            continue;
        }

        printf("[STM Server] Connection accepted from %s:%d\n",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        fflush(stdout);

        /* Prepare the poll structure */
        struct pollfd pfd;
        pfd.fd = newsockfd;
        pfd.events = POLLIN;

        /* Poll for incoming data until timeout or client closes connection */
        while (1)
        {
            int poll_ret = poll(&pfd, 1, POLL_TIMEOUT_MS);
            printf("[STM Server] Poll returned: %d\n", poll_ret);
            fflush(stdout);
            if (poll_ret > 0)
            {
                if (pfd.revents & POLLIN)
                {
                    n = recv(newsockfd, buffer, BUFFER_SIZE - 1, 0);
                    if (n > 0)
                    {
                        buffer[n] = '\0';
                        printf("[STM Server] Received (%d bytes): %s\n", n, buffer);
                        fflush(stdout);

                        /* Echo back the received data */
                        if (send(newsockfd, buffer, n, 0) < 0)
                        {
                            perror("ERROR sending data back to client");
                            fflush(stdout);
                            break;
                        }
                    }
                    else if (n == 0)
                    {
                        printf("[STM Server] Client closed the connection\n");
                        fflush(stdout);
                        break;
                    }
                    else
                    {
                        perror("ERROR reading from socket");
                        fflush(stdout);
                        break;
                    }
                }
            }
            else if (poll_ret == 0)
            {
                printf("[STM Server] Poll timeout, no data received within %d ms\n", POLL_TIMEOUT_MS);
                fflush(stdout);
                break;
            }
            else
            {
                perror("ERROR during poll");
                fflush(stdout);
                break;
            }
        }
        close(newsockfd);
    }

    close(sockfd);
    return NULL;
}

int main(int argc, FAR char *argv[])
{
    pthread_t server_thread;
    int ret = pthread_create(&server_thread, NULL, stm_server_thread, NULL);
    if (ret != 0)
    {
        perror("Failed to create TCP server thread");
        return 1;
    }

    printf("[STM Server] TCP server thread started.\n");
    fflush(stdout);
    pthread_join(server_thread, NULL);
    return 0;
}