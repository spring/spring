#ifdef NO_NET
#include <sys/types.h>
#include <sys/socket.h>
typedef  sockaddr sockaddr_in;
typedef void * SOCKET;
#else
#warning unix: this file should not be included
#endif

