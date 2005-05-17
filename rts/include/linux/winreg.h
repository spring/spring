//FIXME to be removed when a way to store options for linux will be developed
#warning linux doesnt use winreg.h. add a way to store options for linux
#ifdef NO_WINREG
typedef void* HKEY;
#define HKEY_CURRENT_USER 0
#endif
