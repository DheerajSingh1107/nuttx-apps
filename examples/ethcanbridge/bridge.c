// bridge.c: Packet parsing & routing
#include "bridge.h"
#include "can_iface.h"
#include "eth_iface.h"

void bridge_init(void)
{
    // Init any shared buffers or semaphores here
    can_init(); // Initialize CAN interface
}

void bridge_forward_from_eth(const uint8_t *data, size_t len)
{
    // TODO: Parse and send via CAN
    can_send(data, len);
}

void bridge_forward_from_can(const uint8_t *data, size_t len)
{
    // TODO: Parse and send via Ethernet
    eth_send(data, len);
}
