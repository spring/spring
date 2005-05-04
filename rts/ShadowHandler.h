#pragma once

#include "matrix44f.h"
#include "mygl.h"
#include <vector>

class CShadowHandler
{
public:
	CShadowHandler(void);
	~CShadowHandler(void);
	void CreateShadows(void);

	int shadowMapSize;
	unsigned int shadowTexture;

	bool drawShadows;
	bool inShadowPass;
	bool showShadowMap;
	bool firstDraw;
	bool useFPShadows;
	bool copyDepthTexture;

	float3 centerPos;
	float3 cross1;
	float3 cross2;

	float x1,x2,y1,y2;

	CMatrix44f shadowMatrix;

	HPBUFFERARB hPBuffer; // Handle to a p-buffer.
	HDC         hDCPBuffer;      // Handle to a device context.
	HGLRC       hRCPBuffer;      // Handle to a GL rendering context.
	void CreatePBuffer(void);
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
};

extern CShadowHandler* shadowHandler;
