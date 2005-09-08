/**
This file is here only for fast porting. 
It has to be removed so that no #include <windows.h>
appears in the common code

@author gnibu

*/

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <fenv.h>
#include <iostream>
#include <boost/cstdint.hpp>
#include <SDL/SDL.h>

struct paletteentry_s {
	Uint8 peRed;
	Uint8 peGreen;
	Uint8 peBlue;
	Uint8 peFlags;
};
typedef struct paletteentry_s PALETTEENTRY;

#endif //__WINDOWS_H__
