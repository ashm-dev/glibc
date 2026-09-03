/* Process tst-winsize-main.c in the default mode, with <sys/ioctl.h>
   included first to check that both headers can be combined in this
   order; tst-tcwinsize.c covers the other order.  */
#include <sys/ioctl.h>

#include "tst-winsize-main.c"
