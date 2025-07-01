// can_iface.h
#ifndef __CAN_IFACE_H__
#define __CAN_IFACE_H__

#include <stdint.h>
#include <stddef.h>

void can_init(void);
void can_send(const uint8_t *data, size_t len);
void can_rx_thread_start(void);

#endif