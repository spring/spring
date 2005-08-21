#ifndef GROUNDFLASH_H
#define GROUNDFLASH_H
/*pragma once removed*/



class CVertexArray;

class CGroundFlash
{
public:
	CGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl);
	~CGroundFlash(void);
	void Draw(void);
	void Update(void);

	float3 pos;
	float3 normal;
	float3 side1,side2;

	float flashSize;
	float circleSize;
	float circleGrowth;
	float circleAlpha;
	float flashAlpha;
	float flashAge;
	float flashAgeSpeed;
	float circleAlphaDec;
	int ttl;

	static unsigned int texture;
	static CVertexArray* va;
};


#endif /* GROUNDFLASH_H */
