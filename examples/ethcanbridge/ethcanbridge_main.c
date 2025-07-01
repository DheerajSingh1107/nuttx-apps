
// main.c: Initialization and thread setup
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "can_iface.h"
#include "eth_iface.h"
#include "bridge.h"

int main(int argc, char *argv[])
{
    printf("[INIT] Starting Ethernet–CAN bridge...\n");

    bridge_init();
    eth_server_start();
    can_rx_thread_start();

    pthread_exit(NULL);
    return 0;
}