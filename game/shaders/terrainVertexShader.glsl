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

vec4 Ambient;
vec4 Diffuse;
vec4 Specular;

#ifdef UseShadowMapping

void CalcShadowTexCoord()
{
	vec2 temp, at;

	temp.x = dot(gl_Vertex, shadowMatrix[0]);
	temp.y = dot(gl_Vertex, shadowMatrix[1]);
	
	at = abs(temp);
	at += vec4(shadowParams.x,shadowParams.x,0.0,0.0);
	at.x = inversesqrt(at.x);
	at.y = inversesqrt(at.y);
	at += vec4(shadowParams.y,shadowParams.y,0.0,0.0);

	shadowTexCoord.xy = temp * at + vec2(shadowParams.z,shadowParams.w);
	shadowTexCoord.z = dot(gl_Vertex, shadowMatrix[2]);
}

#endif

#ifdef UseBumpMapping
	attribute mat3 TangentSpaceMatrix;
#endif


void main (void)
{
    // Eye-coordinate position of vertex, needed in various calculations
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = gl_ProjectionMatrix * ecPosition;
    
	CalculateTexCoords();
#ifdef UseShadowMapping
	CalcShadowTexCoord();	
#endif

#ifdef UseBumpMapping
	tsLightDir = TangentSpaceMatrix * (-wsLightDir);
	vec3 eyeDir = normalize(gl_Vertex.xyz - wsEyePos);
	tsEyeDir = TangentSpaceMatrix * eyeDir;
#endif
}

