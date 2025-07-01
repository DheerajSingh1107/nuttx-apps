// eth_iface.h
#ifndef __ETH_IFACE_H__
#define __ETH_IFACE_H__

#include <stdint.h>
#include <stddef.h>

void eth_server_start(void);
void eth_send(const uint8_t *data, size_t len);

#endif