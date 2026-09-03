/* Check tcgetwinsize and tcsetwinsize on a pseudo-terminal (bug 32074).
   Copyright (C) 2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* <termios.h> comes first so that this test also covers including
   <sys/ioctl.h> after it; tst-winsize-ioctl.c covers the other order.  */
#include <termios.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <string.h>
#include <support/check.h>
#include <support/tty.h>
#include <support/xunistd.h>

static int
do_test (void)
{
  struct winsize initial = { .ws_row = 24, .ws_col = 80 };
  int outer, inner;
  support_openpty (&outer, &inner, NULL, NULL, &initial);

  struct winsize ws;
  memset (&ws, 0xff, sizeof ws);
  TEST_COMPARE (tcgetwinsize (inner, &ws), 0);
  TEST_COMPARE (ws.ws_row, 24);
  TEST_COMPARE (ws.ws_col, 80);

  struct winsize updated
    = { .ws_row = 50, .ws_col = 132, .ws_xpixel = 640, .ws_ypixel = 480 };
  TEST_COMPARE (tcsetwinsize (inner, &updated), 0);

  /* The new size must be visible through the ioctl interface and
     through the other end of the pseudo-terminal.  */
  memset (&ws, 0, sizeof ws);
  TEST_COMPARE (ioctl (inner, TIOCGWINSZ, &ws), 0);
  TEST_COMPARE (ws.ws_row, 50);
  TEST_COMPARE (ws.ws_col, 132);
  TEST_COMPARE (ws.ws_xpixel, 640);
  TEST_COMPARE (ws.ws_ypixel, 480);

  memset (&ws, 0, sizeof ws);
  TEST_COMPARE (tcgetwinsize (outer, &ws), 0);
  TEST_COMPARE (ws.ws_row, 50);
  TEST_COMPARE (ws.ws_col, 132);

  /* A descriptor which is not a terminal.  */
  int fds[2];
  xpipe (fds);
  errno = 0;
  TEST_COMPARE (tcgetwinsize (fds[0], &ws), -1);
  TEST_COMPARE (errno, ENOTTY);
  errno = 0;
  TEST_COMPARE (tcsetwinsize (fds[0], &updated), -1);
  TEST_COMPARE (errno, ENOTTY);

  /* An invalid descriptor.  */
  errno = 0;
  TEST_COMPARE (tcgetwinsize (-1, &ws), -1);
  TEST_COMPARE (errno, EBADF);
  errno = 0;
  TEST_COMPARE (tcsetwinsize (-1, &updated), -1);
  TEST_COMPARE (errno, EBADF);

  xclose (fds[0]);
  xclose (fds[1]);
  xclose (inner);
  xclose (outer);
  return 0;
}

#include <support/test-driver.c>
