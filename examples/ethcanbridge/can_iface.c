/* SPDX-License-Identifier: Apache-2.0 */

#include <nuttx/config.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <pthread.h>
#include <nuttx/can/can.h>

#include "can_iface.h"
#include "bridge.h"

#ifdef CONFIG_CAN_EXTID
#define PRI_CAN_ID PRIu32
#else
#define PRI_CAN_ID PRIu16
#endif

#if defined(CONFIG_EXAMPLES_ETHCANBRIDGE_READ)
#  define CAN_OFLAGS O_RDONLY
#elif defined(CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE)
#  define CAN_OFLAGS O_WRONLY
#elif defined(CONFIG_EXAMPLES_ETHCANBRIDGE_READWRITE)
#  define CAN_OFLAGS O_RDWR
#  define CONFIG_EXAMPLES_ETHCANBRIDGE_READ 1
#  define CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE 1
#endif

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE
#  ifdef CONFIG_CAN_EXTID
#    define MAX_ID CAN_MAX_EXTMSGID
#  else
#    define MAX_ID CAN_MAX_STDMSGID
#  endif
#endif

uint8_t run[8] = {0x00, 0x00, 0xcc, 0x7f, 0xff, 0x87, 0x17, 0xff};
static int fd;

void can_init(void)
{
  fd = open(CONFIG_EXAMPLES_ETHCANBRIDGE_DEVPATH, CAN_OFLAGS);
  if (fd < 0)
    {
      printf("ERROR: open %s failed: %d\n", CONFIG_EXAMPLES_ETHCANBRIDGE_DEVPATH, errno);
      return;
    }

  struct canioc_bittiming_s bt;
  int ret = ioctl(fd, CANIOC_GET_BITTIMING, (unsigned long)((uintptr_t)&bt));
  if (ret < 0)
    {
      printf("Bit timing not available: %d\n", errno);
    }
  else
    {
      printf("Bit timing:\n");
      printf("   Baud: %lu\n", (unsigned long)bt.bt_baud);
      printf("  TSEG1: %u\n", bt.bt_tseg1);
      printf("  TSEG2: %u\n", bt.bt_tseg2);
      printf("    SJW: %u\n", bt.bt_sjw);
    }
}

void can_send(const uint8_t *data, size_t len)
{
  size_t msgsize;
  struct can_msg_s txmsg = {0};

  int msgdlc = can_bytes2dlc(len);

  txmsg.cm_hdr.ch_id    = 0x1;
  txmsg.cm_hdr.ch_rtr   = false;
  txmsg.cm_hdr.ch_dlc   = msgdlc;

#ifdef CONFIG_CAN_ERRORS
      txmsg.cm_hdr.ch_error  = 0;
#endif
#ifdef CONFIG_CAN_EXTID
      txmsg.cm_hdr.ch_extid  = extended;
#endif
#ifdef CONFIG_CAN_FD
      txmsg.cm_hdr.ch_edl    = true;
      txmsg.cm_hdr.ch_brs    = brs;
      txmsg.cm_hdr.ch_esi    = false;
#endif
      txmsg.cm_hdr.ch_tcf    = 0;


  memcpy(txmsg.cm_data, data, len);
  msgsize = CAN_MSGLEN(can_dlc2bytes(msgdlc));
  ssize_t nbytes = write(fd, &txmsg, msgsize);
  if (nbytes != msgsize)
    {
      printf("ERROR: write failed: %ld\n", (long)nbytes);
    }
}

static void *can_rx_thread(void *arg)
{
  struct can_msg_s rxmsg = {0};

  size_t msgsize;
  msgsize = sizeof(struct can_msg_s);



  while (1)
    {
      ssize_t nbytes = read(fd, &rxmsg, msgsize);
      if (nbytes < CAN_MSGLEN(0) || nbytes > msgsize)
        {
          printf("ERROR: read failed\n");
          break;
        }

#ifdef CONFIG_CAN_ERRORS
      if (rxmsg.cm_hdr.ch_error != 0)
        {
          printf("CAN error received\n");
          continue;
        }
#endif
    // can_send(run, sizeof(run)); // Echo the run data for testing

      printf("Received ID: %4" PRI_CAN_ID " DLC: %u\n",
             rxmsg.cm_hdr.ch_id, rxmsg.cm_hdr.ch_dlc);
     // bridge_forward_from_can(rxmsg.cm_data, can_dlc2bytes(rxmsg.cm_hdr.ch_dlc));
    }
    printf("CAN RX thread exiting\n");
    close(fd);
  return NULL;
}

void can_rx_thread_start(void)
{
  pthread_t tid;
  pthread_create(&tid, NULL, can_rx_thread, NULL);
}
