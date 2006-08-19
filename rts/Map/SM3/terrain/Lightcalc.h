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
namespace terrain
{
	// The lightmap contains a shading texture to apply the lights on the terrain,
	// and a shadow texture for unit shadowing

	class Lightmap : public BaseTexture
	{
	public:
		Lightmap (Heightmap *hm, int level, int shadowLevelDif, LightingInfo *li);
		~Lightmap ();

		uint GetShadowTexture() { return shadowTex; }
		uint GetShadingTexture() { return shadingTex; }

	protected:
		void CalculateShadows (uchar *dst, int dstw, float lightX,float lightY, float lightH, float *centerhm, int hmw, int hmscale, bool directional);

		uint shadowTex, shadingTex;
	};

	class Shadowmap : public BaseTexture
	{
	public:
		Shadowmap (Heightmap *hm, int level, LightingInfo *li);
	};
};
