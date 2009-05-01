#ifndef GROUNDFLASH_H
#define GROUNDFLASH_H

#include "Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"

class CVertexArray;
class CColorMap;

class CGroundFlash : public CExpGenSpawnable
{
public:
	CR_DECLARE(CGroundFlash);

	CGroundFlash(const float3& p GML_FARG_H);
	CGroundFlash();
	virtual ~CGroundFlash(){};
	virtual void Draw(){};
	virtual bool Update(){return false;}; // returns false when it should be deleted
	virtual void Init(const float3& pos, CUnit *owner GML_FARG_H){};

	//bool alwaysVisible;
	//float3 pos;

	static CVertexArray* va;
};

class CStandardGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CStandardGroundFlash);

	CStandardGroundFlash();
	CStandardGroundFlash(const float3& pos, float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl, const float3& color=float3(1.0f,1.0f,0.7f) GML_FARG_H);
	~CStandardGroundFlash();
	void Draw();
	bool Update(); // returns false when it should be deleted

	float3 side1,side2;

	float flashSize;
	float circleSize;
	float circleGrowth;
	float circleAlpha;
	float flashAlpha;
	float flashAge;
	float flashAgeSpeed;
	float circleAlphaDec;
	unsigned char color[3];
	int ttl;
};

//a simple groundflash only creating one quad with specified texture
//also this one sets alwaysVisible=true (because it's used by the seismic effect)
// so don't use it (or fix it) for things that should be affected by LOS.
class CSeismicGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CSeismicGroundFlash);
	~CSeismicGroundFlash();
	CSeismicGroundFlash(const float3& pos, AtlasedTexture texture, int ttl, int fade, float size, float sizeGrowth, float alpha, const float3& col GML_FARG_H);
	void Draw();
	bool Update(); // returns false when it should be deleted

	float3 side1,side2;

	AtlasedTexture texture;
	float sizeGrowth;
	float size;
	float alpha;
	int fade;
	int ttl;
	unsigned char color[4];
};

//simple groundflash using a texture and a colormap, used in the explosiongenerator
class CSimpleGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CSimpleGroundFlash);

	CSimpleGroundFlash();
	~CSimpleGroundFlash();

	void Init(const float3& explosionPos, CUnit *owner GML_FARG_H);
	void Draw();
	bool Update(); // returns false when it should be deleted

	float3 side1,side2;

	float sizeGrowth;
	float size;
	int ttl;
	float age, agerate;

	CColorMap *colorMap;
	GroundFXTexture *texture;

};

#endif /* GROUNDFLASH_H */
