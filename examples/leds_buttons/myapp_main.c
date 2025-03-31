/****************************************************************************
 * apps/examples/buttons/buttons_main.c
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
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>

#include "myapp_main.h"
int main(int argc, FAR char *argv[])
{
  int ret;

  printf("buttons_main: Starting the button_daemon\n");

  ret = task_create("button_daemon", CONFIG_EXAMPLES_LEDS_BUTTONS_PRIORITY,
                    CONFIG_EXAMPLES_LEDS_BUTTONS_STACKSIZE, button_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("buttons_main: ERROR: Failed to start button_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  ret = task_create("led_daemon", CONFIG_EXAMPLES_LEDS_BUTTONS_PRIORITY,
                    CONFIG_EXAMPLES_LEDS_BUTTONS_STACKSIZE, led_daemon,
                    NULL);
  if (ret < 0)
    {
      int errcode = errno;
      printf("leds_main: ERROR: Failed to start led_daemon: %d\n",
             errcode);
      return EXIT_FAILURE;
    }

  printf("buttons_main: button_daemon started\n");
  return EXIT_SUCCESS;
}
