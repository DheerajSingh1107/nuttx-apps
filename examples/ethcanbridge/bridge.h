// bridge.h: Message routing interface
#ifndef __BRIDGE_H__
#define __BRIDGE_H__
#include <stdint.h>
#include <stddef.h>
void bridge_init(void);
void bridge_forward_from_eth(const uint8_t *data, size_t len);
void bridge_forward_from_can(const uint8_t *data, size_t len);

#endif