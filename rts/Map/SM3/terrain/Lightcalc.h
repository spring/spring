/*
Static lightmapping class
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
		void CalculateShadows (uchar *dst, int dstw, int lightX,int lightY, float lightH, float *centerhm, int hmw, int hmscale);

		uint shadowTex, shadingTex;
	};
};
