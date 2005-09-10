#ifndef SHADOWHANDLER_H
#define SHADOWHANDLER_H
/*pragma once removed*/

#include "FBO.h"
#include "Matrix44f.h"
#include "myGL.h"
#include <vector>

class CShadowHandler
{
public:
	CShadowHandler(void);
	~CShadowHandler(void);
	void CreateShadows(void);

	int shadowMapSize;
	unsigned int shadowTexture;
	unsigned int tempTexture;

	bool drawShadows;
	bool inShadowPass;
	bool showShadowMap;
	bool firstDraw;
	bool useFPShadows;

	float3 centerPos;
	float3 cross1;
	float3 cross2;

	float x1,x2,y1,y2;

	CMatrix44f shadowMatrix;
	void DrawShadowTex(void);
	void CalcMinMaxView(void);

protected:
	void GetFrustumSide(float3& side,bool upside);
	struct fline {
		float base;
		float dir;
		int left;
		float minz;
		float maxz;
	};
	std::vector<fline> left;
	FBO *fb;
};

extern CShadowHandler* shadowHandler;

#endif /* SHADOWHANDLER_H */
