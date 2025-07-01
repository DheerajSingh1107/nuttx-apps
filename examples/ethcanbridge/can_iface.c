// can_iface.c: CAN interface implementation
#include <stdio.h>
#include <pthread.h>
#include "can_iface.h"
#include "bridge.h"

void can_init(void)
{
    // Open /dev/can0 and configure bit timing, filters etc.
}

void can_send(const uint8_t *data, size_t len)
{
    // Write to /dev/can0
}

static void *can_rx_thread(void *arg)
{
    // Blocking read from /dev/can0, then call bridge_forward_from_can
    return NULL;
}

void can_rx_thread_start(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, can_rx_thread, NULL);
}