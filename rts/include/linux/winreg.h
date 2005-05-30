//FIXME to be removed when a way to store options for linux will be developed
#ifndef _WINREG_H_
#define _WINREG_H_
#ifdef NO_WINREG
typedef void* HKEY;
#define HKEY_CURRENT_USER 0
#else
#warning linux doesnt use winreg.h. add a way to store options for linux
#endif
#endif //_WINREG_H_
