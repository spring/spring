/* compile using: gcc -shared -m32 no-msync.c -o no-msync.so */
#include <stdio.h>
#include <stdlib.h>
int msync(void *addr, size_t len, int flags) {}
static void _no_msync_init(void) __attribute__ ((constructor));
static void _no_msync_init(void) {
	putenv("LD_PRELOAD=");
}
