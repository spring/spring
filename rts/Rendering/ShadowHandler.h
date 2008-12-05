#ifndef SHADOWHANDLER_H
#define SHADOWHANDLER_H

#include "Matrix44f.h"
#include "GL/myGL.h"
#include <vector>
#include "Rendering/GL/FBO.h"

class CShadowHandler
{
public:
	CShadowHandler(void);
	~CShadowHandler(void);
	void CreateShadows(void);

	int shadowMapSize;
	GLuint shadowTexture;

	static bool canUseShadows;
	static bool useFPShadows;

	bool drawShadows;
	bool inShadowPass;
	bool showShadowMap;

	float3 centerPos;
	float3 cross1;
	float3 cross2;

	float x1,x2,y1,y2;
	float xmid,ymid;
	float p17, p18;

	CMatrix44f shadowMatrix;
	void DrawShadowTex(void);
	void CalcMinMaxView(void);

	void GetShadowMapSizeFactors(float &param17, float &param18);

protected:
	void GetFrustumSide(float3& side,bool upside);
	bool InitDepthTarget();
	void DrawShadowPasses();

protected:
	struct fline {
		float base;
		float dir;
		int left;
		float minz;
		float maxz;
	};
	std::vector<fline> left;
	FBO fb;

	bool firstDraw;
	static bool firstInstance;
};

extern CShadowHandler* shadowHandler;

#endif /* SHADOWHANDLER_H */
