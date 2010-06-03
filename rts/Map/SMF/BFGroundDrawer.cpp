/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "SmfReadMap.h"
#include "BFGroundDrawer.h"
#include "BFGroundTextures.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.hpp"
#include "Rendering/Shaders/Shader.hpp"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "System/ConfigHandler.h"
#include "System/FastMath.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/mmgr.h"

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif

using std::min;
using std::max;

CBFGroundDrawer::CBFGroundDrawer(CSmfReadMap* rm):
	bigSquareSize(128),
	numBigTexX(gs->mapx / bigSquareSize),
	numBigTexY(gs->mapy / bigSquareSize),
	maxIdx(((gs->mapx + 1) * (gs->mapy + 1)) - 1),
	heightDataX(gs->mapx + 1)
{
	mapWidth = (gs->mapx * SQUARE_SIZE);
	mapHeight = (gs->mapy * SQUARE_SIZE);
	bigTexH = mapHeight / numBigTexY;

	map = rm;
	heightData = map->heightmap;

	LoadMapShaders();

	textures = new CBFGroundTextures(map);

	viewRadius = configHandler->Get("GroundDetail", 40);
	viewRadius += (viewRadius & 1); //! we need a multiple of 2

	waterDrawn = false;

	waterPlaneCamInDispList  = 0;
	waterPlaneCamOutDispList = 0;

	if (mapInfo->water.hasWaterPlane) {
		waterPlaneCamInDispList = glGenLists(1);
		glNewList(waterPlaneCamInDispList,GL_COMPILE);
		CreateWaterPlanes(false);
		glEndList();

		waterPlaneCamOutDispList = glGenLists(1);
		glNewList(waterPlaneCamOutDispList,GL_COMPILE);
		CreateWaterPlanes(true);
		glEndList();
	}

#ifdef USE_GML
	multiThreadDrawGround = configHandler->Get("MultiThreadDrawGround", 1);
	multiThreadDrawGroundShadow = configHandler->Get("MultiThreadDrawGroundShadow", 0);
#endif
}

CBFGroundDrawer::~CBFGroundDrawer(void)
{
	delete textures;

	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");

	configHandler->Set("GroundDetail", viewRadius);

	if (waterPlaneCamInDispList) {
		glDeleteLists(waterPlaneCamInDispList, 1);
		glDeleteLists(waterPlaneCamOutDispList, 1);
	}

#ifdef USE_GML
	configHandler->Set("MultiThreadDrawGround", multiThreadDrawGround);
	configHandler->Set("MultiThreadDrawGroundShadow", multiThreadDrawGroundShadow);
#endif
}

bool CBFGroundDrawer::LoadMapShaders() {
	CShaderHandler* sh = shaderHandler;

	smfShaderBaseARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderBaseARB", true);
	smfShaderReflARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderReflARB", true);
	smfShaderRefrARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderRefrARB", true);
	smfShaderCurrARB = smfShaderBaseARB;
	smfShaderGLSL    = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderGLSL", false);

	if (shadowHandler->canUseShadows) {
		if (!map->haveSpecularLighting) {
			// always use FP shadows (shadowHandler->useFPShadows is short
			// for "this graphics card supports ARB_FRAGMENT_PROGRAM", and
			// canUseShadows can not be true while useFPShadows is false)
			smfShaderBaseARB->AttachShaderObject(sh->CreateShaderObject("ARB/ground.vp", "", GL_VERTEX_PROGRAM_ARB));
			smfShaderBaseARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			smfShaderBaseARB->Link();

			smfShaderReflARB->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundreflectinverted.vp", "", GL_VERTEX_PROGRAM_ARB));
			smfShaderReflARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			smfShaderReflARB->Link();

			smfShaderRefrARB->AttachShaderObject(sh->CreateShaderObject("ARB/dwgroundrefract.vp", "", GL_VERTEX_PROGRAM_ARB));
			smfShaderRefrARB->AttachShaderObject(sh->CreateShaderObject("ARB/groundFPshadow.fp", "", GL_FRAGMENT_PROGRAM_ARB));
			smfShaderRefrARB->Link();
		} else {
			std::string defs;
				defs += (map->haveSplatTexture)?
					"#define SMF_DETAIL_TEXTURE_SPLATTING 1\n":
					"#define SMF_DETAIL_TEXTURE_SPLATTING 0\n";
				defs += (map->minheight > 0.0f || mapInfo->map.voidWater)?
					"#define SMF_WATER_ABSORPTION 0\n":
					"#define SMF_WATER_ABSORPTION 1\n";
				defs += (map->GetSkyReflectModTexture() != 0)?
					"#define SMF_SKY_REFLECTIONS 1\n":
					"#define SMF_SKY_REFLECTIONS 0\n";

			smfShaderGLSL->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFVertProg.glsl", defs, GL_VERTEX_SHADER));
			smfShaderGLSL->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFFragProg.glsl", defs, GL_FRAGMENT_SHADER));
			smfShaderGLSL->Link();
			smfShaderGLSL->SetUniformLocation("diffuseTex");          // idx  0
			smfShaderGLSL->SetUniformLocation("normalsTex");          // idx  1
			smfShaderGLSL->SetUniformLocation("shadowTex");           // idx  2
			smfShaderGLSL->SetUniformLocation("detailTex");           // idx  3
			smfShaderGLSL->SetUniformLocation("specularTex");         // idx  4
			smfShaderGLSL->SetUniformLocation("mapSizePO2");          // idx  5
			smfShaderGLSL->SetUniformLocation("mapSize");             // idx  6
			smfShaderGLSL->SetUniformLocation("texSquareX");          // idx  7
			smfShaderGLSL->SetUniformLocation("texSquareZ");          // idx  8
			smfShaderGLSL->SetUniformLocation("lightDir");            // idx  9
			smfShaderGLSL->SetUniformLocation("cameraPos");           // idx 10
			smfShaderGLSL->SetUniformLocation("$UNUSED$");            // idx 11
			smfShaderGLSL->SetUniformLocation("shadowMat");           // idx 12
			smfShaderGLSL->SetUniformLocation("shadowParams");        // idx 13
			smfShaderGLSL->SetUniformLocation("groundAmbientColor");  // idx 14
			smfShaderGLSL->SetUniformLocation("groundDiffuseColor");  // idx 15
			smfShaderGLSL->SetUniformLocation("groundSpecularColor"); // idx 16
			smfShaderGLSL->SetUniformLocation("groundShadowDensity"); // idx 17
			smfShaderGLSL->SetUniformLocation("waterMinColor");       // idx 18
			smfShaderGLSL->SetUniformLocation("waterBaseColor");      // idx 19
			smfShaderGLSL->SetUniformLocation("waterAbsorbColor");    // idx 20
			smfShaderGLSL->SetUniformLocation("splatDetailTex");      // idx 21
			smfShaderGLSL->SetUniformLocation("splatDistrTex");       // idx 22
			smfShaderGLSL->SetUniformLocation("splatTexScales");      // idx 23
			smfShaderGLSL->SetUniformLocation("splatTexMults");       // idx 24
			smfShaderGLSL->SetUniformLocation("skyReflectTex");       // idx 25
			smfShaderGLSL->SetUniformLocation("skyReflectModTex");    // idx 26

			smfShaderGLSL->Enable();
			smfShaderGLSL->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
			smfShaderGLSL->SetUniform1i(1, 5); // normalsTex  (idx 1, texunit 5)
			smfShaderGLSL->SetUniform1i(2, 4); // shadowTex   (idx 2, texunit 4)
			smfShaderGLSL->SetUniform1i(3, 2); // detailTex   (idx 3, texunit 2)
			smfShaderGLSL->SetUniform1i(4, 6); // specularTex (idx 4, texunit 6)
			smfShaderGLSL->SetUniform2f(5, (gs->pwr2mapx * SQUARE_SIZE), (gs->pwr2mapy * SQUARE_SIZE));
			smfShaderGLSL->SetUniform2f(6, (gs->mapx * SQUARE_SIZE), (gs->mapy * SQUARE_SIZE));
			smfShaderGLSL->SetUniform4fv(9, const_cast<float*>(&mapInfo->light.sunDir[0]));
			smfShaderGLSL->SetUniform3fv(14, const_cast<float*>(&mapInfo->light.groundAmbientColor[0]));
			smfShaderGLSL->SetUniform3fv(15, const_cast<float*>(&mapInfo->light.groundSunColor[0]));
			smfShaderGLSL->SetUniform3fv(16, const_cast<float*>(&mapInfo->light.groundSpecularColor[0]));
			smfShaderGLSL->SetUniform1f(17, mapInfo->light.groundShadowDensity);
			smfShaderGLSL->SetUniform3fv(18, const_cast<float*>(&mapInfo->water.minColor[0]));
			smfShaderGLSL->SetUniform3fv(19, const_cast<float*>(&mapInfo->water.baseColor[0]));
			smfShaderGLSL->SetUniform3fv(20, const_cast<float*>(&mapInfo->water.absorb[0]));
			smfShaderGLSL->SetUniform1i(21, 7); // splatDetailTex (idx 21, texunit 7)
			smfShaderGLSL->SetUniform1i(22, 8); // splatDistrTex (idx 22, texunit 8)
			smfShaderGLSL->SetUniform4fv(23, const_cast<float*>(&mapInfo->splats.texScales[0]));
			smfShaderGLSL->SetUniform4fv(24, const_cast<float*>(&mapInfo->splats.texMults[0]));
			smfShaderGLSL->SetUniform1i(25,  9); // skyReflectTex (idx 25, texunit 9)
			smfShaderGLSL->SetUniform1i(26, 10); // skyReflectModTex (idx 26, texunit 10)

			smfShaderGLSL->Disable();
		}

		return true;
	}

	return false;
}





void CBFGroundDrawer::CreateWaterPlanes(const bool &camOufOfMap) {
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);

	const float xsize = (mapWidth) >> 2;
	const float ysize = (mapHeight) >> 2;

	CVertexArray* va = GetVertexArray();
	va->Initialize();

	unsigned char fogColor[4] = {
	  (unsigned char)(255 * mapInfo->atmosphere.fogColor[0]),
	  (unsigned char)(255 * mapInfo->atmosphere.fogColor[1]),
	  (unsigned char)(255 * mapInfo->atmosphere.fogColor[2]),
	  255
	};

	unsigned char planeColor[4] = {
	  (unsigned char)(255 * mapInfo->water.planeColor[0]),
	  (unsigned char)(255 * mapInfo->water.planeColor[1]),
	  (unsigned char)(255 * mapInfo->water.planeColor[2]),
	   255
	};

	const float alphainc = fastmath::PI2 / 32;
	float alpha,r1,r2;
	float3 p(0.0f,std::min(-200.0f, map->minheight - 400.0f),0.0f);
	const float size = std::min(xsize,ysize);
	for (int n = (camOufOfMap) ? 0 : 1; n < 4 ; ++n) {

		if ((n == 1) && !camOufOfMap) {
			//! don't render vertices under the map
			r1 = 2 * size;
		}else{
			r1 = n*n * size;
		}

		if (n==3) {
			//! last stripe: make it thinner (looks better with fog)
			r2 = (n+0.5)*(n+0.5) * size;
		}else{
			r2 = (n+1)*(n+1) * size;
		}
		for (alpha = 0.0f; (alpha - fastmath::PI2) < alphainc ; alpha+=alphainc) {
			p.x = r1 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r1 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertexC(p, planeColor );
			p.x = r2 * fastmath::sin(alpha) + 2 * xsize;
			p.z = r2 * fastmath::cos(alpha) + 2 * ysize;
			va->AddVertexC(p, (n==3) ? fogColor : planeColor);
		}
	}
	va->DrawArrayC(GL_TRIANGLE_STRIP);

	glDepthMask(GL_TRUE);
}

inline void CBFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (!drawWaterReflection) {
		const bool camOutOfMap = !camera->pos.IsInBounds();
		glCallList(camOutOfMap ? waterPlaneCamOutDispList : waterPlaneCamInDispList);
	}
}



inline void CBFGroundDrawer::DrawVertexAQ(CVertexArray* ma, int x, int y)
{
	float height = heightData[y * heightDataX + x];
	if (waterDrawn && height < 0.0f) {
		height *= 2;
	}

	//! don't send the normals as vertex attributes
	//! (DLOD'ed triangles mess with interpolation)
	//! const float3& n = readmap->vertexNormals[(y * heightDataX) + x];
	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}

inline void CBFGroundDrawer::DrawVertexAQ(CVertexArray* ma, int x, int y, float height)
{
	if (waterDrawn && height < 0.0f) {
		height *= 2;
	}

	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}

inline void CBFGroundDrawer::EndStripQ(CVertexArray* ma)
{
	ma->EndStripQ();
}

inline void CBFGroundDrawer::DrawGroundVertexArrayQ(CVertexArray * &ma)
{
	ma->DrawArray0(GL_TRIANGLE_STRIP);
	ma = GetVertexArray();
}



inline bool CBFGroundDrawer::BigTexSquareRowVisible(int bty) {
	const int minx =             0;
	const int maxx =      mapWidth;
	const int minz = bty * bigTexH;
	const int maxz = minz + bigTexH;
	const float miny = readmap->currMinHeight;
	const float maxy = fabs(cam2->pos.y); // ??

	const float3 mins(minx, miny, minz);
	const float3 maxs(maxx, maxy, maxz);

	return (cam2->InView(mins, maxs));
}



inline void CBFGroundDrawer::FindRange(int &xs, int &xe, const std::vector<fline> &left, const std::vector<fline> &right, int y, int lod) {
	int xt0, xt1;
	std::vector<fline>::const_iterator fli;

	for (fli = left.begin(); fli != left.end(); fli++) {
		float xtf = fli->base + fli->dir * y;
		xt0 = (int)xtf;
		xt1 = (int)(xtf + fli->dir * lod);

		if (xt0 > xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod - lod;

		if (xt0 > xs)
			xs = xt0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		float xtf = fli->base + fli->dir * y;
		xt0 = (int)xtf;
		xt1 = (int)(xtf + fli->dir * lod);

		if (xt0 < xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod + lod;

		if (xt0 < xe)
			xe = xt0;
	}
}


#define CLAMP(i) std::max(0, std::min((i), maxIdx))

inline void CBFGroundDrawer::DoDrawGroundRow(int bty) {
	if (!BigTexSquareRowVisible(bty)) {
		//! skip this entire row of squares if we can't see it
		return;
	}

	CVertexArray *ma = GetVertexArray();

	bool inStrip = false;
	float x0, x1;
	int x,y;
	int sx = 0;
	int ex = numBigTexX;
	std::vector<fline>::iterator fli;

	//! only process the necessary big squares in the x direction
	int bigSquareSizeY = bty * bigSquareSize;
	for (fli = left.begin(); fli != left.end(); fli++) {
		x0 = fli->base + fli->dir * bigSquareSizeY;
		x1 = x0 + fli->dir * bigSquareSize;

		if (x0 > x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 > sx)
			sx = (int) x0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		x0 = fli->base + fli->dir * bigSquareSizeY + bigSquareSize;
		x1 = x0 + fli->dir * bigSquareSize;

		if (x0 < x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 < ex)
			ex = (int) x0;
	}

	float cx2 = cam2->pos.x / SQUARE_SIZE;
	float cy2 = cam2->pos.z / SQUARE_SIZE;

	for (int btx = sx; btx < ex; ++btx) {
		ma->Initialize();

		for (int lod = 1; lod < neededLod; lod <<= 1) {
			float oldcamxpart = 0.0f;
			float oldcamypart = 0.0f;

			int hlod = lod >> 1;
			int dlod = lod << 1;

			int cx = (int)cx2;
			int cy = (int)cy2;

			if(lod>1) {
				int cxo = (cx / hlod) * hlod;
				int cyo = (cy / hlod) * hlod;
				float cx2o = (cxo / lod) * lod;
				float cy2o = (cyo / lod) * lod;
				oldcamxpart = (cx2 - cx2o) / lod;
				oldcamypart = (cy2 - cy2o) / lod;
			}

			cx = (cx / lod) * lod;
			cy = (cy / lod) * lod;
			int ysquaremod = (cy % dlod) / lod;
			int xsquaremod = (cx % dlod) / lod;

			float camxpart = (cx2 - ((cx / dlod) * dlod)) / dlod;
			float camypart = (cy2 - ((cy / dlod) * dlod)) / dlod;

			float mcxp=1.0f-camxpart;
			float hcxp=0.5f*camxpart;
			float hmcxp=0.5f*mcxp;

			float mcyp=1.0f-camypart;
			float hcyp=0.5f*camypart;
			float hmcyp=0.5f*mcyp;

			float mocxp=1.0f-oldcamxpart;
			float hocxp=0.5f*oldcamxpart;
			float hmocxp=0.5f*mocxp;

			float mocyp=1.0f-oldcamypart;
			float hocyp=0.5f*oldcamypart;
			float hmocyp=0.5f*mocyp;

			int minty = bty * bigSquareSize;
			int maxty = minty + bigSquareSize;
			int mintx = btx * bigSquareSize;
			int maxtx = mintx + bigSquareSize;

			int minly = cy + (-viewRadius + 3 - ysquaremod) * lod;
			int maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
			int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod;
			int maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

			int xstart = max(minlx, mintx);
			int xend   = min(maxlx, maxtx);
			int ystart = max(minly, minty);
			int yend   = min(maxly, maxty);

			int vrhlod = viewRadius * hlod;

			for (y = ystart; y < yend; y += lod) {
				int xs = xstart;
				int xe = xend;
				FindRange(/*inout*/ xs, /*inout*/ xe, left, right, y, lod);

				// If FindRange modifies (xs, xe) to a (less then) empty range,
				// continue to the next row.
				// If we'd continue, nloop (below) would become negative and we'd
				// allocate a vertex array with negative size.  (mantis #1415)
				if (xe < xs) continue;

				int ylod = y + lod;
				int yhlod = y + hlod;
				int nloop = (xe - xs) / lod + 1;

				ma->EnlargeArrays((52 * nloop), 14 * nloop + 1);

				int yhdx = y * heightDataX;
				int ylhdx = yhdx + lod * heightDataX;
				int yhhdx = yhdx + hlod * heightDataX;

				for (x = xs; x < xe; x += lod) {
					int xlod = x + lod;
					int xhlod = x + hlod;
					//! info: all triangle quads start in the top left corner
					if ((lod == 1) ||
						(x > cx + vrhlod) || (x < cx - vrhlod) ||
						(y > cy + vrhlod) || (y < cy - vrhlod)) {
						//! normal terrain (all vertices in one LOD)
						if (!inStrip) {
							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, ylod);
							inStrip = true;
						}

						DrawVertexAQ(ma, xlod, y);
						DrawVertexAQ(ma, xlod, ylod);
					} else {
						//! border between 2 different LODs
						if ((x >= cx + vrhlod)) {
							//! lower LOD to the right
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x),                              idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1] + heightData[idx2   ]) * hmocxp + heightData[idx3    ] * oldcamxpart;
							float h2 = (heightData[idx1] + heightData[idx1LOD]) * hmocxp + heightData[idx1HLOD] * oldcamxpart;
							float h3 = (heightData[idx2] + heightData[idx1LOD]) * hmocxp + heightData[idx3HLOD] * oldcamxpart;
							float h4 = (heightData[idx2] + heightData[idx2LOD]) * hmocxp + heightData[idx2HLOD] * oldcamxpart;

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, yhlod, h1);
							DrawVertexAQ(ma, xhlod, y, h2);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							EndStripQ(ma);
							DrawVertexAQ(ma, x, yhlod, h1);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							EndStripQ(ma);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							DrawVertexAQ(ma, xlod, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xhlod, y, h2);
							EndStripQ(ma);
						}
						else if ((x <= cx - vrhlod)) {
							//! lower LOD to the left
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1LOD] + heightData[idx2LOD]) * hocxp + heightData[idx3LOD ] * mocxp;
							float h2 = (heightData[idx1   ] + heightData[idx1LOD]) * hocxp + heightData[idx1HLOD] * mocxp;
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * hocxp + heightData[idx3HLOD] * mocxp;
							float h4 = (heightData[idx2   ] + heightData[idx2LOD]) * hocxp + heightData[idx2HLOD] * mocxp;

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, xlod, yhlod, h1);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xhlod, y, h2);
							EndStripQ(ma);
							DrawVertexAQ(ma, xlod, ylod);
							DrawVertexAQ(ma, xlod, yhlod, h1);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							EndStripQ(ma);
							DrawVertexAQ(ma, xhlod, y, h2);
							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							EndStripQ(ma);
						}

						if ((y >= cy + vrhlod)) {
							//! lower LOD above
							int idx1 = yhdx + x,  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = ylhdx + x, idx2LOD = CLAMP(idx2 + lod);
							int idx3 = yhhdx + x, idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1   ] + heightData[idx1LOD]) * hmocyp + heightData[idx1HLOD] * oldcamypart;
							float h2 = (heightData[idx1   ] + heightData[idx2   ]) * hmocyp + heightData[idx3    ] * oldcamypart;
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * hmocyp + heightData[idx3HLOD] * oldcamypart;
							float h4 = (heightData[idx2LOD] + heightData[idx1LOD]) * hmocyp + heightData[idx3LOD ] * oldcamypart;

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, yhlod, h2);
							DrawVertexAQ(ma, xhlod, y, h1);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							EndStripQ(ma);
							DrawVertexAQ(ma, x, yhlod, h2);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xlod, ylod);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							EndStripQ(ma);
						}
						else if ((y <= cy - vrhlod)) {
							//! lower LOD beneath
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx2   ] + heightData[idx2LOD]) * hocyp + heightData[idx2HLOD] * mocyp;
							float h2 = (heightData[idx1   ] + heightData[idx2   ]) * hocyp + heightData[idx3    ] * mocyp;
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * hocyp + heightData[idx3HLOD] * mocyp;
							float h4 = (heightData[idx2LOD] + heightData[idx1LOD]) * hocyp + heightData[idx3LOD ] * mocyp;

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x, yhlod, h2);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xhlod, ylod, h1);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							DrawVertexAQ(ma, xlod, ylod);
							EndStripQ(ma);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, yhlod, h2);
							EndStripQ(ma);
						}
					}
				}

				if (inStrip) {
					EndStripQ(ma);
					inStrip = false;
				}
			} //for (y = ystart; y < yend; y += lod)

			int yst = max(ystart - lod, minty);
			int yed = min(yend + lod, maxty);
			int nloop = (yed - yst) / lod + 1;

			if (nloop > 0)
				ma->EnlargeArrays((8 * nloop), 2 * nloop);

			//! rita yttre begr?snings yta mot n?ta lod
			if (maxlx < maxtx && maxlx >= mintx) {
				x = maxlx;
				int xlod = x + lod;
				for (y = yst; y < yed; y += lod) {
					DrawVertexAQ(ma, x, y);
					DrawVertexAQ(ma, x, y + lod);

					if (y % dlod) {
						int idx1 = CLAMP((y      ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
						int idx2 = CLAMP((y + lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod);
						int idx3 = CLAMP((y - lod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod);
						float h = (heightData[idx3LOD] + heightData[idx2LOD]) * hmcxp +	heightData[idx1LOD] * camxpart;
						DrawVertexAQ(ma, xlod, y, h);
						DrawVertexAQ(ma, xlod, y + lod);
					} else {
						int idx1 = CLAMP((y       ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
						int idx2 = CLAMP((y +  lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod);
						int idx3 = CLAMP((y + dlod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod);
						float h = (heightData[idx1LOD] + heightData[idx3LOD]) * hmcxp + heightData[idx2LOD] * camxpart;
						DrawVertexAQ(ma, xlod, y);
						DrawVertexAQ(ma, xlod, y + lod, h);
					}
					EndStripQ(ma);
				}
			}

			if (minlx > mintx && minlx < maxtx) {
				x = minlx - lod;
				int xlod = x + lod;
				for (y = yst; y < yed; y += lod) {
					if (y % dlod) {
						int idx1 = CLAMP((y      ) * heightDataX + x);
						int idx2 = CLAMP((y + lod) * heightDataX + x);
						int idx3 = CLAMP((y - lod) * heightDataX + x);
						float h = (heightData[idx3] + heightData[idx2]) * hcxp + heightData[idx1] * mcxp;
						DrawVertexAQ(ma, x, y, h);
						DrawVertexAQ(ma, x, y + lod);
					} else {
						int idx1 = CLAMP((y       ) * heightDataX + x);
						int idx2 = CLAMP((y +  lod) * heightDataX + x);
						int idx3 = CLAMP((y + dlod) * heightDataX + x);
						float h = (heightData[idx1] + heightData[idx3]) * hcxp + heightData[idx2] * mcxp;
						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, y + lod, h);
					}
					DrawVertexAQ(ma, xlod, y);
					DrawVertexAQ(ma, xlod, y + lod);
					EndStripQ(ma);
				}
			}

			if (maxly < maxty && maxly > minty) {
				y = maxly;
				int xs = max(xstart - lod, mintx);
				int xe = min(xend + lod,   maxtx);
				FindRange(xs, xe, left, right, y, lod);

				if (xs < xe) {
					x = xs;
					int ylod = y + lod;
					int nloop = (xe - xs) / lod + 2; //! one extra for if statment
					int ylhdx = (y + lod) * heightDataX;

					ma->EnlargeArrays((2 * nloop), 1);

					if (x % dlod) {
						int idx2 = CLAMP(ylhdx + x), idx2PLOD = CLAMP(idx2 + lod), idx2MLOD = CLAMP(idx2 - lod);
						float h = (heightData[idx2MLOD] + heightData[idx2PLOD]) * hmcyp + heightData[idx2] * camypart;
						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, ylod, h);
					} else {
						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, ylod);
					}
					for (x = xs; x < xe; x += lod) {
						if (x % dlod) {
							DrawVertexAQ(ma, x + lod, y);
							DrawVertexAQ(ma, x + lod, ylod);
						} else {
							int idx2 = CLAMP(ylhdx + x), idx2PLOD  = CLAMP(idx2 +  lod), idx2PLOD2 = CLAMP(idx2 + dlod);
							float h = (heightData[idx2PLOD2] + heightData[idx2]) * hmcyp + heightData[idx2PLOD] * camypart;
							DrawVertexAQ(ma, x + lod, y);
							DrawVertexAQ(ma, x + lod, ylod, h);
						}
					}
					EndStripQ(ma);
				}
			}

			if (minly > minty && minly < maxty) {
				y = minly - lod;
				int xs = max(xstart - lod, mintx);
				int xe = min(xend + lod,   maxtx);
				FindRange(xs, xe, left, right, y, lod);

				if (xs < xe) {
					x = xs;
					int ylod = y + lod;
					int yhdx = y * heightDataX;
					int nloop = (xe - xs) / lod + 2; //! one extra for if statment

					ma->EnlargeArrays((2 * nloop), 1);

					if (x % dlod) {
						int idx1 = CLAMP(yhdx + x), idx1PLOD = CLAMP(idx1 + lod), idx1MLOD = CLAMP(idx1 - lod);
						float h = (heightData[idx1MLOD] + heightData[idx1PLOD]) * hcyp + heightData[idx1] * mcyp;
						DrawVertexAQ(ma, x, y, h);
						DrawVertexAQ(ma, x, ylod);
					} else {
						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, ylod);
					}

					for (x = xs; x < xe; x+= lod) {
						if (x % dlod) {
							DrawVertexAQ(ma, x + lod, y);
							DrawVertexAQ(ma, x + lod, ylod);
						} else {
							int idx1 = CLAMP(yhdx + x), idx1PLOD  = CLAMP(idx1 +  lod), idx1PLOD2 = CLAMP(idx1 + dlod);
							float h = (heightData[idx1PLOD2] + heightData[idx1]) * hcyp + heightData[idx1PLOD] * mcyp;
							DrawVertexAQ(ma, x + lod, y, h);
							DrawVertexAQ(ma, x + lod, ylod);
						}
					}
					EndStripQ(ma);
				}
			}

		} //for (int lod = 1; lod < neededLod; lod <<= 1)

		SetupBigSquare(btx,bty);
		DrawGroundVertexArrayQ(ma);
	}
}

void CBFGroundDrawer::Draw(bool drawWaterReflection, bool drawUnitReflection)
{
	if (mapInfo->map.voidWater && map->currMaxHeight < 0.0f) {
		return;
	}

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}


	waterDrawn = drawWaterReflection;


	const int baseViewRadius = max(4, viewRadius);

	if (drawUnitReflection) {
		viewRadius = ((int)(viewRadius * LODScaleUnitReflection)) & 0xfffffe;
	}
	if (drawWaterReflection) {
		viewRadius = ((int)(viewRadius * LODScaleReflection)) & 0xfffffe;
	}
	//if (drawWaterRefraction) {
	//	viewRadius = ((int)(viewRadius * LODScaleRefraction)) & 0xfffffe;
	//}

	float zoom  = 45.0f / camera->GetFov();
	viewRadius  = (int) (viewRadius * fastmath::apxsqrt(zoom));
	viewRadius  = max(max(numBigTexY,numBigTexX), viewRadius);
	viewRadius += (viewRadius & 1); //! we need a multiple of 2
	neededLod   = int((globalRendering->viewRange * 0.125f) / viewRadius) << 1;

	UpdateCamRestraints();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D); //FIXME needed?
	glCullFace(GL_BACK);

	if (smfShaderCurrARB == smfShaderBaseARB)
		glEnable(GL_CULL_FACE);

	SetupTextureUnits(drawWaterReflection);

	if (mapInfo->map.voidWater && !waterDrawn) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.9f);
	}

#ifdef USE_GML
	if(multiThreadDrawGround) {
		gmlProcessor->Work(NULL,&CBFGroundDrawer::DoDrawGroundRowMT,NULL,this,gmlThreadCount,FALSE,NULL,numBigTexY,50,100,TRUE,NULL);
	}
	else
#endif
	{
		int camBty = (int)math::floor(cam2->pos.z / (bigSquareSize * SQUARE_SIZE));
		camBty = std::max(0,std::min(numBigTexY-1, camBty ));

		//! try to render in "front to back" (so start with the camera nearest BigGroundLines)
		for (int bty = camBty; bty >= 0; --bty) {
			DoDrawGroundRow(bty);
		}
		for (int bty = camBty+1; bty < numBigTexY; ++bty) {
			DoDrawGroundRow(bty);
		}
	}
	ResetTextureUnits(drawWaterReflection);
	glDisable(GL_CULL_FACE);


	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (mapInfo->map.voidWater && !waterDrawn) {
		glDisable(GL_ALPHA_TEST);
	}

	if (!(drawWaterReflection || drawUnitReflection)) {
		if (mapInfo->water.hasWaterPlane) {
			DrawWaterPlane(drawWaterReflection);
		}

		if (groundDecals) {
			groundDecals->Draw();
			projectileDrawer->DrawGroundFlashes();
			glDepthMask(1);
		}
	}

//	sky->SetCloudShadow(1);

	viewRadius = baseViewRadius;
}


inline void CBFGroundDrawer::DoDrawGroundShadowLOD(int nlod) {
	CVertexArray *ma = GetVertexArray();
	ma->Initialize();

	bool inStrip = false;
	int x,y;
	int lod = 1 << nlod;

	float cx2 = camera->pos.x / SQUARE_SIZE;
	float cy2 = camera->pos.z / SQUARE_SIZE;

	float oldcamxpart = 0.0f;
	float oldcamypart = 0.0f;

	int hlod = lod >> 1;
	int dlod = lod << 1;

	int cx = (int)cx2;
	int cy = (int)cy2;
	if (lod > 1) {
		int cxo = (cx / hlod) * hlod;
		int cyo = (cy / hlod) * hlod;
		float cx2o = (cxo / lod) * lod;
		float cy2o = (cyo / lod) * lod;
		oldcamxpart = (cx2 - cx2o) / lod;
		oldcamypart = (cy2 - cy2o) / lod;
	}

	cx = (cx / lod) * lod;
	cy = (cy / lod) * lod;
	int ysquaremod = (cy % dlod) / lod;
	int xsquaremod = (cx % dlod) / lod;

	float camxpart = (cx2 - (cx / dlod) * dlod) / dlod;
	float camypart = (cy2 - (cy / dlod) * dlod) / dlod;

	int minty = 0;
	int maxty = gs->mapy;
	int mintx = 0;
	int maxtx = gs->mapx;

	int minly = cy + (-viewRadius + 3 - ysquaremod) * lod;
	int maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
	int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod;
	int maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

	int xstart = max(minlx, mintx);
	int xend   = min(maxlx, maxtx);
	int ystart = max(minly, minty);
	int yend   = min(maxly, maxty);

	int lhdx=lod*heightDataX;
	int hhdx=hlod*heightDataX;
	int dhdx=dlod*heightDataX;

	float mcxp=1.0f-camxpart;
	float hcxp=0.5f*camxpart;
	float hmcxp=0.5f*mcxp;

	float mcyp=1.0f-camypart;
	float hcyp=0.5f*camypart;
	float hmcyp=0.5f*mcyp;

	float mocxp=1.0f-oldcamxpart;
	float hocxp=0.5f*oldcamxpart;
	float hmocxp=0.5f*mocxp;

	float mocyp=1.0f-oldcamypart;
	float hocyp=0.5f*oldcamypart;
	float hmocyp=0.5f*mocyp;

	int vrhlod = viewRadius * hlod;

	for (y = ystart; y < yend; y += lod) {
		int xs = xstart;
		int xe = xend;

		if (xe < xs) continue;

		int ylod = y + lod;
		int yhlod = y + hlod;
		int ydx = y * heightDataX;
		int nloop = (xe - xs) / lod + 1;

		//! EnlargeArrays(nVertices, nStrips [, stripSize])
		//! includes one extra for final endstrip
		ma->EnlargeArrays((52 * nloop), 14 * nloop + 1);

		for (x = xs; x < xe; x += lod) {
			int xlod = x + lod;
			int xhlod = x + hlod;
			if ((lod == 1) ||
				(x > cx + vrhlod) || (x < cx - vrhlod) ||
				(y > cy + vrhlod) || (y < cy - vrhlod)) {
					if (!inStrip) {
						DrawVertexAQ(ma, x, y      );
						DrawVertexAQ(ma, x, ylod);
						inStrip = true;
					}
					DrawVertexAQ(ma, xlod, y      );
					DrawVertexAQ(ma, xlod, ylod);
			}
			else {  //! inre begr?sning mot f?eg?nde lod
				int yhdx=ydx+x;
				int ylhdx=yhdx+lhdx;
				int yhhdx=yhdx+hhdx;

				if(x>=cx+vrhlod){
					float h1=(heightData[yhdx ] + heightData[ylhdx    ]) * hmocxp + heightData[yhhdx     ] * oldcamxpart;
					float h2=(heightData[yhdx ] + heightData[yhdx+lod ]) * hmocxp + heightData[yhdx+hlod ] * oldcamxpart;
					float h3=(heightData[ylhdx] + heightData[yhdx+lod ]) * hmocxp + heightData[yhhdx+hlod] * oldcamxpart;
					float h4=(heightData[ylhdx] + heightData[ylhdx+lod]) * hmocxp + heightData[ylhdx+hlod] * oldcamxpart;

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,yhlod,h1);
					DrawVertexAQ(ma, xhlod,y,h2);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					EndStripQ(ma);
					DrawVertexAQ(ma, x,yhlod,h1);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					EndStripQ(ma);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					DrawVertexAQ(ma, xlod,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xhlod,y,h2);
					EndStripQ(ma);
				}
				if(x<=cx-vrhlod){
					float h1=(heightData[yhdx+lod] + heightData[ylhdx+lod]) * hocxp + heightData[yhhdx+lod ] * mocxp;
					float h2=(heightData[yhdx    ] + heightData[yhdx+lod ]) * hocxp + heightData[yhdx+hlod ] * mocxp;
					float h3=(heightData[ylhdx   ] + heightData[yhdx+lod ]) * hocxp + heightData[yhhdx+hlod] * mocxp;
					float h4=(heightData[ylhdx   ] + heightData[ylhdx+lod]) * hocxp + heightData[ylhdx+hlod] * mocxp;

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, xlod,yhlod,h1);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xhlod,y,h2);
					EndStripQ(ma);
					DrawVertexAQ(ma, xlod,ylod);
					DrawVertexAQ(ma, xlod,yhlod,h1);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					EndStripQ(ma);
					DrawVertexAQ(ma, xhlod,y,h2);
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					EndStripQ(ma);
				}
				if(y>=cy+vrhlod){
					float h1=(heightData[yhdx     ] + heightData[yhdx+lod]) * hmocyp + heightData[yhdx+hlod ] * oldcamypart;
					float h2=(heightData[yhdx     ] + heightData[ylhdx   ]) * hmocyp + heightData[yhhdx     ] * oldcamypart;
					float h3=(heightData[ylhdx    ] + heightData[yhdx+lod]) * hmocyp + heightData[yhhdx+hlod] * oldcamypart;
					float h4=(heightData[ylhdx+lod] + heightData[yhdx+lod]) * hmocyp + heightData[yhhdx+lod ] * oldcamypart;

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,yhlod,h2);
					DrawVertexAQ(ma, xhlod,y,h1);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					EndStripQ(ma);
					DrawVertexAQ(ma, x,yhlod,h2);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xlod,ylod);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					EndStripQ(ma);
				}
				if(y<=cy-vrhlod){
					float h1=(heightData[ylhdx    ] + heightData[ylhdx+lod]) * hocyp + heightData[ylhdx+hlod] * mocyp;
					float h2=(heightData[yhdx     ] + heightData[ylhdx    ]) * hocyp + heightData[yhhdx     ] * mocyp;
					float h3=(heightData[ylhdx    ] + heightData[yhdx+lod ]) * hocyp + heightData[yhhdx+hlod] * mocyp;
					float h4=(heightData[ylhdx+lod] + heightData[yhdx+lod ]) * hocyp + heightData[yhhdx+lod ] * mocyp;

					if (inStrip) {
						EndStripQ(ma);
						inStrip = false;
					}
					DrawVertexAQ(ma, x,yhlod,h2);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xhlod,ylod,h1);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					DrawVertexAQ(ma, xlod,ylod);
					EndStripQ(ma);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,yhlod,h2);
					EndStripQ(ma);
				}
			}
		}
		if(inStrip){
			EndStripQ(ma);
			inStrip=false;
		}
	}

	int yst=max(ystart - lod, minty);
	int yed=min(yend + lod, maxty);
	int nloop=(yed-yst)/lod+1;

	if (nloop > 0)
		ma->EnlargeArrays((8 * nloop), 2 * nloop);

	//!rita yttre begr?snings yta mot n?ta lod
	if (maxlx < maxtx && maxlx >= mintx) {
		x = maxlx;
		const int xlod = x + lod;

		for (y = yst; y < yed; y += lod) {
			DrawVertexAQ(ma, x, y      );
			DrawVertexAQ(ma, x, y + lod);
			const int yhdx = y * heightDataX + x;

			if (y % dlod) {
				float h = (heightData[yhdx - lhdx + lod] + heightData[yhdx + lhdx + lod]) * hmcxp + heightData[yhdx+lod] * camxpart;
				DrawVertexAQ(ma, xlod, y, h);
				DrawVertexAQ(ma, xlod, y + lod);
			} else {
				float h=(heightData[yhdx+lod]+heightData[yhdx+dhdx+lod]) * hmcxp + heightData[yhdx+lhdx+lod] * camxpart;
				DrawVertexAQ(ma, xlod,y);
				DrawVertexAQ(ma, xlod,y+lod,h);
			}
			EndStripQ(ma);
		}
	}

	if (minlx > mintx && minlx < maxtx) {
		x = minlx-lod;
		const int xlod = x + lod;

		for(y = yst; y < yed; y += lod) {
			int yhdx=y*heightDataX+x;
			if(y%dlod){
				float h=(heightData[yhdx-lhdx]+heightData[yhdx+lhdx]) * hcxp + heightData[yhdx] * mcxp;
				DrawVertexAQ(ma, x,y,h);
				DrawVertexAQ(ma, x,y+lod);
			} else {
				float h=(heightData[yhdx]+heightData[yhdx+dhdx]) * hcxp + heightData[yhdx+lhdx] * mcxp;
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,y+lod,h);
			}
			DrawVertexAQ(ma, xlod,y);
			DrawVertexAQ(ma, xlod,y+lod);
			EndStripQ(ma);
		}
	}
	if(maxly<maxty && maxly>minty){
		y=maxly;
		int xs=max(xstart-lod,mintx);
		int xe=min(xend+lod,maxtx);
		if (xs < xe) {
			x = xs;
			int ylod = y + lod;
			int ydx = y * heightDataX;
			int nloop = (xe - xs) / lod + 2; //! two extra for if statment

			ma->EnlargeArrays((2 * nloop), 1);

			if (x % dlod) {
				int ylhdx=ydx+x+lhdx;
				float h=(heightData[ylhdx-lod]+heightData[ylhdx+lod]) * hmcyp + heightData[ylhdx] * camypart;
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,ylod,h);
			} else {
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,ylod);
			}
			for(x=xs;x<xe;x+=lod){
				if(x%dlod){
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+lod,ylod);
				} else {
					DrawVertexAQ(ma, x+lod,y);
					int ylhdx=ydx+x+lhdx;
					float h=(heightData[ylhdx+dlod]+heightData[ylhdx]) * hmcyp + heightData[ylhdx+lod] * camypart;
					DrawVertexAQ(ma, x+lod,ylod,h);
				}
			}
			EndStripQ(ma);
		}
	}
	if(minly>minty && minly<maxty){
		y=minly-lod;
		int xs=max(xstart-lod,mintx);
		int xe=min(xend+lod,maxtx);
		if(xs<xe){
			x=xs;
			int ylod = y + lod;
			int ydx = y * heightDataX;
			int nloop = (xe - xs) / lod + 2; //! two extra for if statment

			ma->EnlargeArrays((2 * nloop), 1);

			if (x % dlod) {
				int yhdx=ydx+x;
				float h=(heightData[yhdx-lod]+heightData[yhdx+lod]) * hcyp + heightData[yhdx] * mcyp;
				DrawVertexAQ(ma, x,y,h);
				DrawVertexAQ(ma, x,ylod);
			} else {
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,ylod);
			}
			for(x=xs;x<xe;x+=lod){
				if(x%dlod){
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+lod,ylod);
				} else {
					int yhdx=ydx+x;
					float h=(heightData[yhdx+dlod]+heightData[yhdx]) * hcyp + heightData[yhdx+lod] * mcyp;
					DrawVertexAQ(ma, x+lod,y,h);
					DrawVertexAQ(ma, x+lod,ylod);
				}
			}
			EndStripQ(ma);
		}
	}
	DrawGroundVertexArrayQ(ma);
}


void CBFGroundDrawer::DrawShadowPass(void)
{
	if (mapInfo->map.voidWater && map->currMaxHeight < 0.0f) {
		return;
	}

	const int NUM_LODS = 4;

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	// glEnable(GL_CULL_FACE);
	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	po->Enable();

#ifdef USE_GML
	if (multiThreadDrawGroundShadow) {
		gmlProcessor->Work(NULL, &CBFGroundDrawer::DoDrawGroundShadowLODMT, NULL, this, gmlThreadCount, FALSE, NULL, NUM_LODS + 1, 50, 100, TRUE, NULL);
	}
	else
#endif
	{
		for (int nlod = 0; nlod < NUM_LODS + 1; ++nlod) {
			DoDrawGroundShadowLOD(nlod);
		}
	}

	po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
}


inline void CBFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	textures->SetTexture(bigSquareX, bigSquareY);

	//! must be in drawLos mode or shadows must be off
	if (DrawExtraTex() || !shadowHandler->drawShadows) {
		SetTexGen(1.0f / 1024, 1.0f / 1024, -bigSquareX, -bigSquareY);
	}

	if (map->haveSpecularLighting) {
		smfShaderGLSL->SetUniform1i(7, bigSquareX);
		smfShaderGLSL->SetUniform1i(8, bigSquareY);
	} else {
		smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
		smfShaderCurrARB->SetUniform4f(11, -bigSquareX, -bigSquareY, 0, 0);
	}
}






void CBFGroundDrawer::SetupTextureUnits(bool drawReflection)
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	if (DrawExtraTex()) {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, infoTex);
		glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1,1,1,1); //fixes a nvidia bug with gltexgen

		glActiveTexture(GL_TEXTURE3);
		if (map->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
			glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			static const GLfloat planeX[] = {0.02f, 0.0f, 0.0f, 0.0f};
			static const GLfloat planeZ[] = {0.0f, 0.0f, 0.02f, 0.0f};

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glDisable(GL_TEXTURE_2D);
		}

		/*
		//! this is just used by DynamicWater to distort the underwater rendering, but it's hard to maintain a vertex shader when working with opengl combiners,
		//! so it's better to limit this to full-shader-driven systems (-> e.g. when shadows are enabled)
		if (overrideVP != 0) {
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, overrideVP);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, 1.0f / 1024, 1.0f / 1024, 0, 1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 0.02f, 0.02f, 0, 1);

			if (drawReflection) {
				glAlphaFunc(GL_GREATER, 0.9f);
				glEnable(GL_ALPHA_TEST);
			}
		}*/
	}

	else if (shadowHandler->drawShadows) {
		const float3 ambientColor = mapInfo->light.groundAmbientColor * (210.0f / 255.0f);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		glActiveTexture(GL_TEXTURE2);

		if (map->detailTex) {
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if (drawReflection) {
			//FIXME Why doing this?
			glAlphaFunc(GL_GREATER, 0.8f);
			glEnable(GL_ALPHA_TEST);
		}

		if (!map->haveSpecularLighting) {
			smfShaderCurrARB->Enable();
			smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			smfShaderCurrARB->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
			smfShaderCurrARB->SetUniform4f(12, 1.0f / 1024, 1.0f / 1024, 0, 1);
			smfShaderCurrARB->SetUniform4f(13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
			smfShaderCurrARB->SetUniform4f(14, 0.02f, 0.02f, 0, 1);
			smfShaderCurrARB->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
			smfShaderCurrARB->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1);
			smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, mapInfo->light.groundShadowDensity);

			glMatrixMode(GL_MATRIX0_ARB);
			glLoadMatrixf(shadowHandler->shadowMatrix.m);
			glMatrixMode(GL_MODELVIEW);

			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
			glActiveTexture(GL_TEXTURE0);
		} else {
			smfShaderGLSL->Enable();
			smfShaderGLSL->SetUniform3fv(10, &camera->pos[0]);
			smfShaderGLSL->SetUniformMatrix4fv(12, false, &shadowHandler->shadowMatrix.m[0]);
			smfShaderGLSL->SetUniform4fv(13, const_cast<float*>(&(shadowHandler->GetShadowParams().x)));

			glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, map->GetNormalsTexture());
			glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, map->GetSpecularTexture());
			glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, map->GetSplatDetailTexture());
			glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, map->GetSplatDistrTexture());
			glActiveTexture(GL_TEXTURE9);
				glEnable(GL_TEXTURE_CUBE_MAP_ARB);
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSkyReflectionTextureID());
			glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, map->GetSkyReflectModTexture());

			// setup for shadow2DProj
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
			glActiveTexture(GL_TEXTURE0);
		}
	}

	else {
		if (map->detailTex) {
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
			glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			static const GLfloat planeX[] = {0.02f, 0.0f, 0.00f, 0.0f};
			static const GLfloat planeZ[] = {0.00f, 0.0f, 0.02f, 0.0f};

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_2D);
		}

		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		//! bind the detail texture a 2nd time to increase the details (-> GL_ADD_SIGNED_ARB is limited -0.5 to +0.5)
		//! (also do this after the shading texture cause of color clamping issues)
		if (map->detailTex) {
			glActiveTexture(GL_TEXTURE3);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
			glMultiTexCoord4f(GL_TEXTURE3_ARB, 1,1,1,1); //fixes a nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			static const GLfloat planeX[] = {0.02f, 0.0f, 0.0f, 0.0f};
			static const GLfloat planeZ[] = {0.0f, 0.0f, 0.02f, 0.0f};

			glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_S, GL_OBJECT_PLANE, planeX);
			glEnable(GL_TEXTURE_GEN_S);

			glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
			glTexGenfv(GL_T, GL_OBJECT_PLANE, planeZ);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glActiveTexture(GL_TEXTURE3);
			glDisable(GL_TEXTURE_2D);
		}

		/*
		//! this is just used by DynamicWater to distort the underwater rendering, but it's hard to maintain a vertex shader when working with opengl combiners,
		//! so it's better to limit this to full-shader-driven systems  (-> e.g. when shadows are enabled)
		if (overrideVP != 0) {
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, overrideVP != 0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0f / 1024, 1.0f / 1024, 0, 1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x * 0.02f),-floor(camera->pos.z * 0.02f), 0, 0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02f, 0.02f, 0,1);

			if (drawReflection) {
				glAlphaFunc(GL_GREATER, 0.9f);
				glEnable(GL_ALPHA_TEST);
			}
		}*/
	}

	glActiveTexture(GL_TEXTURE0);
}


void CBFGroundDrawer::ResetTextureUnits(bool drawReflection)
{
	if (DrawExtraTex() || !shadowHandler->drawShadows) {
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(GL_TEXTURE2);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(GL_TEXTURE0);

		if (drawReflection) {
			glDisable(GL_ALPHA_TEST);
		}
	} else {
		glActiveTexture(GL_TEXTURE4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE9);
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
		glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);

		if (drawReflection) {
			glDisable(GL_ALPHA_TEST);
		}

		if (!map->haveSpecularLighting) {
			smfShaderCurrARB->Disable();
		} else {
			smfShaderGLSL->Disable();
		}
	}
}






void CBFGroundDrawer::AddFrustumRestraint(const float3& side)
{
	fline temp;

	// get vector for collision between frustum and horizontal plane
	float3 b = UpVector.cross(side);

	if (fabs(b.z) < 0.0001f)
		b.z = 0.0001f;

	{
		temp.dir = b.x / b.z;      // set direction to that
		float3 c = b.cross(side);  // get vector from camera to collision line
		float3 colpoint;           // a point on the collision line

		if (side.y > 0)
			colpoint = cam2->pos - c * ((cam2->pos.y - (readmap->currMinHeight - 100)) / c.y);
		else
			colpoint = cam2->pos - c * ((cam2->pos.y - (readmap->currMaxHeight +  30)) / c.y);

		// get intersection between colpoint and z axis
		temp.base = (colpoint.x - colpoint.z * temp.dir) / SQUARE_SIZE;

		if (b.z > 0) {
			left.push_back(temp);
		} else {
			right.push_back(temp);
		}
	}
}

void CBFGroundDrawer::UpdateCamRestraints(void)
{
	left.clear();
	right.clear();

	// add restraints for camera sides
	AddFrustumRestraint(cam2->bottom);
	AddFrustumRestraint(cam2->top);
	AddFrustumRestraint(cam2->rightside);
	AddFrustumRestraint(cam2->leftside);

	// add restraint for maximum view distance
	fline temp;
	float3 side = cam2->forward;
	float3 camHorizontal = cam2->forward;
	camHorizontal.y = 0.0f;
	if (camHorizontal.x != 0.0f || camHorizontal.z != 0.0f)
		camHorizontal.ANormalize();
	// get vector for collision between frustum and horizontal plane
	float3 b = UpVector.cross(camHorizontal);

	if (fabs(b.z) > 0.0001f) {
		temp.dir = b.x / b.z;               // set direction to that
		float3 c = b.cross(camHorizontal);  // get vector from camera to collision line
		float3 colpoint;                    // a point on the collision line

		if (side.y > 0.0f)
			colpoint = cam2->pos + camHorizontal * globalRendering->viewRange * 1.05f - c * (cam2->pos.y / c.y);
		else
			colpoint = cam2->pos + camHorizontal * globalRendering->viewRange * 1.05f - c * ((cam2->pos.y - 255 / 3.5f) / c.y);

		// get intersection between colpoint and z axis
		temp.base = (colpoint.x - colpoint.z * temp.dir) / SQUARE_SIZE;

		if (b.z > 0.0f) {
			left.push_back(temp);
		} else {
			right.push_back(temp);
		}
	}

}

void CBFGroundDrawer::Update()
{
	if (mapInfo->map.voidWater && map->currMaxHeight < 0.0f) {
		return;
	}

	textures->DrawUpdate();
}

void CBFGroundDrawer::IncreaseDetail()
{
	viewRadius += 2;
	LogObject() << "ViewRadius is now " << viewRadius << "\n";
}

void CBFGroundDrawer::DecreaseDetail()
{
	viewRadius -= 2;
	LogObject() << "ViewRadius is now " << viewRadius << "\n";
}
