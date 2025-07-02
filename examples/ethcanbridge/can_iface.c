/****************************************************************************
 * apps/examples/ethcanbridge/can_iface.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/


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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE
#  if defined(CONFIG_CAN_EXTID) && defined(CONFIG_CAN_FD)
#    define OPT_STR ":n:a:b:ehs"
#  elif defined(CONFIG_CAN_EXTID)
#    define OPT_STR ":n:a:b:hs"
#  elif defined(CONFIG_CAN_FD)
#    define OPT_STR ":n:a:b:eh"
#  else
#    define OPT_STR ":n:a:b:h"
#  endif
#else
#  define OPT_STR ":n:h"
#endif

uint8_t run[8] = {0x00, 0x00, 0xcc, 0x7f, 0xff, 0x87, 0x17, 0xff};

static   int fd;


void can_send(const uint8_t *data, size_t len)
{
    // Write to /dev/can0
}

static void *can_rx_thread(void *arg)
{
    // Blocking read from /dev/can0, then call bridge_forward_from_can
  struct canioc_bittiming_s bt;

  #ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE
  int msgdlc;
  struct can_msg_s txmsg =
  {
    0
  };

#ifdef CONFIG_CAN_EXTID
  bool extended = true;
  uint32_t msgid;
#else
  uint16_t msgid;
#endif
#ifdef CONFIG_CAN_FD
  bool brs = true;
#endif
  long minid    = 1;
  long maxid    = MAX_ID;
  uint8_t msgdata;
#endif
  int msgbytes;
  int i;

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_READ
  struct can_msg_s rxmsg =
  {
    0
  };

#endif


  size_t msgsize;
  ssize_t nbytes;
  bool badarg   = false;
  bool help     = false;
  long nmsgs    = CONFIG_EXAMPLES_ETHCANBRIDGE_NMSGS;
  long msgno;
  int option;

  int errval    = 0;
  int ret;
  fd = open(CONFIG_EXAMPLES_ETHCANBRIDGE_DEVPATH, CAN_OFLAGS);
  if (fd < 0)
    {
        printf("ERROR: open %s failed: %d\n",
               CONFIG_EXAMPLES_ETHCANBRIDGE_DEVPATH, errno);
    }
    /* Show bit timing information if provided by the driver.  Not all CAN
     * drivers will support this IOCTL.
     */
  ret = ioctl(fd, CANIOC_GET_BITTIMING, (unsigned long)((uintptr_t)&bt));
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

      /* Now loop the appropriate number of times, performing one loopback test
   * on each pass.
   */

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE
  msgbytes = 8;
  msgid    = 0x1;
  msgdata  = 0;
#endif
  for (msgno = 0; !nmsgs || msgno < nmsgs; msgno++)
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE

      /* Construct the next TX message */

      msgdlc                 = can_bytes2dlc(msgbytes);
      txmsg.cm_hdr.ch_id     = msgid;
      txmsg.cm_hdr.ch_rtr    = false;
      txmsg.cm_hdr.ch_dlc    = msgdlc;
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

      for (i = 0; i < msgbytes; i++)
        {
          txmsg.cm_data[i] = run[i];
        }

      /* Send the TX message */

      msgsize = CAN_MSGLEN(can_dlc2bytes(msgdlc));
      nbytes = write(fd, &txmsg, msgsize);
      if (nbytes != msgsize)
        {
          printf("ERROR: write(%ld) returned %ld\n",
                 (long)msgsize, (long)nbytes);
          errval = 3;
          goto errout_with_dev;
        }

      //printf("  ID: %4" PRI_CAN_ID " DLC: %d\n", msgid, msgdlc);
#endif

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_READ

      /* Read the RX message */

      msgsize = sizeof(struct can_msg_s);
      nbytes = read(fd, &rxmsg, msgsize);
      if (nbytes < CAN_MSGLEN(0) || nbytes > msgsize)
        {
          printf("ERROR: read(%ld) returned %ld\n",
                (long)msgsize, (long)nbytes);
          errval = 4;
          goto errout_with_dev;
        }

      printf("  ID: %4" PRI_CAN_ID " DLC: %u\n",
            rxmsg.cm_hdr.ch_id, rxmsg.cm_hdr.ch_dlc);

      msgbytes = can_dlc2bytes(rxmsg.cm_hdr.ch_dlc);

#ifdef CONFIG_CAN_ERRORS
      /* Check for error reports */

      if (rxmsg.cm_hdr.ch_error != 0)
        {
          //printf("ERROR: CAN error report: [0x%04" PRI_CAN_ID "]\n",
                 rxmsg.cm_hdr.ch_id);
          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_TXTIMEOUT) != 0)
            {
              //printf("  TX timeout\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_LOSTARB) != 0)
            {
              //printf("  Lost arbitration: %02x\n", rxmsg.cm_data[0]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_CONTROLLER) != 0)
            {
              //printf("  Controller error: %02x\n", rxmsg.cm_data[1]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_PROTOCOL) != 0)
            {
              //printf("  Protocol error: %02x %02x\n", rxmsg.cm_data[2],
                     rxmsg.cm_data[3]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_TRANSCEIVER) != 0)
            {
              //printf("  Transceiver error: %02x\n", rxmsg.cm_data[4]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_NOACK) != 0)
            {
              //printf("  No ACK received on transmission\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_BUSOFF) != 0)
            {
              //printf("  Bus off\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_BUSERROR) != 0)
            {
              //printf("  Bus error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_RESTARTED) != 0)
            {
              //printf("  Controller restarted\n");
            }
        }
      else
#endif
        {
#if defined(CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE) && defined(CONFIG_CAN_LOOPBACK)

          /* Verify that the received messages are the same */

          if (memcmp(&txmsg.cm_hdr, &rxmsg.cm_hdr,
                     sizeof(struct can_hdr_s)) != 0)
            {
              printf("ERROR: Sent header does not match received header:\n");
              lib_dumpbuffer("Sent header",
                             (FAR const uint8_t *)&txmsg.cm_hdr,
                             sizeof(struct can_hdr_s));
              lib_dumpbuffer("Received header",
                             (FAR const uint8_t *)&rxmsg.cm_hdr,
                             sizeof(struct can_hdr_s));
              errval = 4;
              goto errout_with_dev;
            }

          if (memcmp(txmsg.cm_data, rxmsg.cm_data, msgbytes) != 0)
            {
              printf("ERROR: Data does not match. DLC=%d\n", msgdlc);
              for (i = 0; i < msgbytes; i++)
                {
                printf("  %d: TX 0x%02x RX 0x%02x\n",
                        i, txmsg.cm_data[i], rxmsg.cm_data[i]);
                  errval = 5;
                  goto errout_with_dev;
                }
            }

          /* Report success */

          printf("  ID: %4" PRI_CAN_ID " DLC: %d -- OK\n", msgid, msgdlc);

#else

          /* Print the data received */

          //printf("Data received:\n");
          for (i = 0; i < msgbytes; i++)
            {
              //printf("  %d: 0x%02x\n", i, rxmsg.cm_data[i]);
            }
#endif
        }
#endif

#ifdef CONFIG_EXAMPLES_ETHCANBRIDGE_WRITE

#endif
    }

errout_with_dev:
  close(fd);

  //printf("Terminating!\n");
  fflush(stdout);
  return errval;
}

void can_init(void)
{
    // Open /dev/can0 and configure bit timing, filters etc.
}
void can_rx_thread_start(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, can_rx_thread, NULL);
}