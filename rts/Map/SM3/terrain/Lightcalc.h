/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LIGHT_CALC_H_
#define _LIGHT_CALC_H_

#include "Textures.h"
#include "Rendering/GL/myGL.h"
#include "TerrainBase.h" // typedef uchar
struct LightingInfo;

namespace terrain
{
	/**
	 * The lightmap contains a shading texture to apply the lights on the
	 * terrain, and a shadow texture for unit shadowing.
	 */
	class Lightmap : public BaseTexture
	{
	public:
		Lightmap(Heightmap* hm, int level, int shadowLevelDif, LightingInfo* li);
		~Lightmap();

		uint GetShadowTexture() const { return shadowTex; } 
		uint GetShadingTexture() const { return shadingTex; }

	protected:
		void CalculateShadows(uchar* dst, int dstw, float lightX, float lightY, float lightH, float* centerhm, int hmw, int hmscale, bool directional);

		GLuint shadowTex;
		GLuint shadingTex;
	};

	class Shadowmap : public BaseTexture
	{
	public:
		Shadowmap (Heightmap* hm, int level, LightingInfo* li);
	};
};

#endif // _LIGHT_CALC_H_

