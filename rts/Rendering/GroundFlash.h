#ifndef GROUNDFLASH_H
#define GROUNDFLASH_H

#include "Rendering/Textures/TextureAtlas.h"

class CVertexArray;

class CGroundFlash
{
public:
	CGroundFlash(const float3& p) : pos(p) {};
	virtual ~CGroundFlash(){};
	virtual void Draw() = 0;
	virtual bool Update()= 0; // returns false when it should be deleted

	float3 pos;

	static CVertexArray* va;
};

class CStandardGroundFlash : public CGroundFlash
{
public:
	CStandardGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl, float3 color=float3(1.0f,1.0f,0.7f));
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
class CSimpleGroundFlash : public CGroundFlash
{
public:
	~CSimpleGroundFlash();
	CSimpleGroundFlash(float3 pos, AtlasedTexture texture, int ttl, int fade, float size, float sizeGrowth, float alpha, float3 col);
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

#endif /* GROUNDFLASH_H */
