#ifndef TERRAIN_BASE_H
#define TERRAIN_BASE_H


#include <GL/glew.h>

#include "../Frustum.h"
#include "Matrix44f.h"

#include "Game/UI/InfoConsole.h"

#ifndef __APPLE__
#include <IL/il.h>
#define TERRAIN_USE_IL
#endif

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define d_puts(_C) info->AddLine(0, std::string(_C));
#define d_trace info->AddLine


namespace terrain {

	struct ILoadCallback
	{
		void PrintMsg (const char *fmt, ...);

		virtual void Write (const char *msg) = 0;
	};

};

#endif
