/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TERRAIN_BASE_H
#define TERRAIN_BASE_H

#include "Rendering/GL/myGL.h"

#include "Map/SM3/Frustum.h"
#include "Matrix44f.h"

#include "LogOutput.h"

#define TERRAIN_USE_IL

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define d_puts(_C) logOutput.Print(std::string(_C));
#define d_trace logOutput.Print


namespace terrain {

	struct ILoadCallback
	{
		void PrintMsg (const char *fmt, ...);

		virtual void Write (const char *msg) = 0;
	};

};

#endif

