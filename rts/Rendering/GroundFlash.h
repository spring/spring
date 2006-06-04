#ifndef GROUNDFLASH_H
#define GROUNDFLASH_H


class CVertexArray;

class CGroundFlash
{
public:
	CGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl, float3 color=float3(1.0f,1.0f,0.7f));
	~CGroundFlash();
	void Draw();
	bool Update(); // returns false when it should be deleted

	float3 pos;
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

	static unsigned int texture;
	static CVertexArray* va;
};


#endif /* GROUNDFLASH_H */
