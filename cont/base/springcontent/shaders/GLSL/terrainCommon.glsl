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

#ifdef UseTextureRECT
#extension GL_ARB_texture_rectangle : enable
#endif

#ifdef UseBumpMapping
	varying vec3 tsLightDir;
	varying vec3 tsEyeDir;
	//varying vec3 tsHalfDir;

#else
	varying vec3 wsEyeDir;
	varying vec3 normal;
#endif

#ifdef UseShadowMapping
	uniform mat4 shadowMatrix;
	uniform vec4 shadowParams; // A,B, mid[0], mid[1]
	uniform sampler2DShadow shadowMap;

	varying vec4 shadowTexCoord;
#endif

uniform vec3 wsLightDir;
uniform vec3 wsEyePos;
