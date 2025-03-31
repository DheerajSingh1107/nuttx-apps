/****************************************************************************
 * apps/examples/leds/leds_main.c
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

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <fcntl.h>
#include <mqueue.h>

#include <nuttx/leds/userled.h>

#include "myapp_main.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_led_daemon_started;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sigterm_action
 ****************************************************************************/

static void sigterm_action(int signo, siginfo_t *siginfo, void *arg)
{
  if (signo == SIGTERM)
    {
      printf("SIGTERM received\n");

      g_led_daemon_started = false;
      printf("led_daemon: Terminated.\n");
    }
  else
    {
      printf("\nsigterm_action: Received signo=%d siginfo=%p arg=%p\n",
             signo, siginfo, arg);
    }
}

/****************************************************************************
 * Name: led_daemon
 ****************************************************************************/

int led_daemon(int argc, char *argv[])
{
  userled_set_t supported;
  userled_set_t ledset;
  userled_set_t ledsetSample;
  bool incrementing;
  int ret;
  int fd;
  pid_t mypid;
  struct sigaction act;

     mqd_t mq;
    int button_state;

    /* Open existing message queue */
  mq = mq_open(MQ_NAME, O_RDONLY);
  if (mq == (mqd_t)-1)
  {
      perror("mq_open failed in led_daemon");
      return NULL;
  }
  /* SIGTERM handler */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = sigterm_action;
  act.sa_flags     = SA_SIGINFO;

  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGTERM);

  ret = sigaction(SIGTERM, &act, NULL);
  if (ret != 0)
    {
      fprintf(stderr, "Failed to install SIGTERM handler, errno=%d\n",
              errno);
      return (EXIT_FAILURE + 1);
    }

  /* Indicate that we are running */

  mypid = getpid();

  g_led_daemon_started = true;
  printf("\nled_daemon (pid# %d): Running\n", mypid);

  /* Open the LED driver */

  printf("led_daemon: Opening %s\n", CONFIG_EXAMPLES_LEDS_DEVPATH);
  fd = open(CONFIG_EXAMPLES_LEDS_DEVPATH, O_WRONLY);
  if (fd < 0)
    {
      int errcode = errno;
      printf("led_daemon: ERROR: Failed to open %s: %d\n",
             CONFIG_EXAMPLES_LEDS_DEVPATH, errcode);
      goto errout;
    }

  /* Get the set of LEDs supported */

  ret = ioctl(fd, ULEDIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      int errcode = errno;
      printf("led_daemon: ERROR: ioctl(ULEDIOC_SUPPORTED) failed: %d\n",
             errcode);
      goto errout_with_fd;
    }

  /* Excluded any LEDs that not supported AND not in the set of LEDs the
   * user asked us to use.
   */

  printf("led_daemon: Supported LEDs 0x%02x\n", (unsigned int)supported);
  supported &= CONFIG_EXAMPLES_LEDS_BUTTONS_LEDSET;

  /* Now loop forever, changing the LED set */

  ledset       = 0;
  incrementing = true;

  while (g_led_daemon_started == true)
    {
      userled_set_t newset;
      userled_set_t tmp;

      if (incrementing)
        {
          tmp = ledset;
          do
            {
              tmp++;
              newset = tmp & supported;
            }
          while (newset == ledset);

          /* REVISIT: There are flaws in this logic.  It would not work
           * correctly if there were spaces in the supported mask.
           */

          if (newset == 0)
            {
              incrementing = false;
              continue;
            }
        }
      else
        {
          /* REVISIT: There are flaws in this logic.  It would not work
           * correctly if there were spaces in the supported mask.
           */

          if (ledset == 0)
            {
              incrementing = true;
              continue;
            }

          tmp = ledset;
          do
            {
              tmp--;
              newset = tmp & supported;
            }
          while (newset == ledset);
        }

      ledset = newset;
      printf("led_daemon: LED set 0x%02x\n", (unsigned int)ledset);
      if (mq_receive(mq, (char *)&button_state, MQ_MSG_SIZE, NULL) > 0)
        {
          printf("Received button state: %d\n", button_state);
          if (button_state == 1 )
            {
             ledsetSample++;
            }
          if (ledsetSample >= 0x7 )
            {
              ledsetSample = 0;
            }
          ret = ioctl(fd, ULEDIOC_SETALL, ledsetSample);
          if (ret < 0)
            {
              int errcode = errno;
              printf("led_daemon: ERROR: ioctl(ULEDIOC_SUPPORTED) failed: %d\n",
                    errcode);
              goto errout_with_fd;
            }
            //control_led(button_state); // Implement this function to toggle LED
        }

      usleep(500 * 1000L);
    }

  /* treats signal termination of the task
   * task terminated by a SIGTERM
   */

  exit(EXIT_SUCCESS);

errout_with_fd:
  close(fd);

errout:
  g_led_daemon_started = false;
  printf("led_daemon: Terminating\n");

  return EXIT_FAILURE;
}
