#ifndef SHADOWHANDLER_H
#define SHADOWHANDLER_H
/*pragma once removed*/

#include "Matrix44f.h"
#include "myGL.h"
#include <vector>
#include "3DOParser.h"
#ifndef _WIN32
#include <GL/glx.h>
#endif

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
	void CreateFramebuffer(void);
	void DrawShadowTex(void);
	void CalcMinMaxView(void);

	bool fbo;
	void render_piece_shadow_volume(S3DO &obj, float3 &obj_pos, float3 &light_pos);
	void render_object_shadow(CUnit &unit);
	void render_unit_shadows();
	void render_all_shadows();
	void draw_shadow();
#ifdef _WIN32
	HPBUFFERARB hPBuffer; // Handle to a p-buffer.
	HDC         hDCPBuffer;      // Handle to a device context.
	HGLRC       hRCPBuffer;      // Handle to a GL rendering context.
#else
	Display *g_pDisplay;
	Window g_window;
	GLXContext g_windowContext;
	GLXContext g_pbufferContext;
	GLXPbuffer g_pbuffer;
#endif

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
	GLuint g_frameBuffer;
	GLuint g_depthRenderBuffer;
};

extern CShadowHandler* shadowHandler;

#endif /* SHADOWHANDLER_H */
