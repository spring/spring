/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/
#ifndef TERRAIN_BASE_H
#define TERRAIN_BASE_H

#include "Rendering/GL/myGL.h"

#include "../Frustum.h"
#include "Matrix44f.h"

#include "LogOutput.h"

#include <IL/il.h>
#define TERRAIN_USE_IL

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define d_puts(_C) logOutput.Print(0, std::string(_C));
#define d_trace logOutput.Print


namespace terrain {

	struct ILoadCallback
	{
		void PrintMsg (const char *fmt, ...);

		virtual void Write (const char *msg) = 0;
	};

};

#endif
