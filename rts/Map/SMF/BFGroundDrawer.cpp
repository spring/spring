#include "StdAfx.h"
#include "BFGroundDrawer.h"
#include "BFGroundTextures.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "LogOutput.h"
#include "SmfReadMap.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GroundDecalHandler.h"
#include "ConfigHandler.h"
#include "FastMath.h"
#include "GlobalUnsynced.h"
#include "mmgr.h"

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> *gmlProcessor;
#endif

using std::min;
using std::max;

CBFGroundDrawer::CBFGroundDrawer(CSmfReadMap* rm) :
	bigSquareSize(128),
	numBigTexX(gs->mapx / bigSquareSize),
	numBigTexY(gs->mapy / bigSquareSize),
	maxIdx(((gs->mapx + 1) * (gs->mapy + 1)) - 1),
	heightDataX(gs->mapx + 1)
{
	mapWidth = (gs->mapx << 3);
	bigTexH = (gs->mapy << 3) / numBigTexY;

	map = rm;

	heightData = map->heightmap;

	if (shadowHandler->canUseShadows) {
		groundVP = LoadVertexProgram("ground.vp");
		groundShadowVP = LoadVertexProgram("groundshadow.vp");

		if (shadowHandler->useFPShadows) {
			groundFPShadow = LoadFragmentProgram("groundFPshadow.fp");
		}
	}

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
	multiThreadDrawGround=configHandler->Get("MultiThreadDrawGround", 1);
	multiThreadDrawGroundShadow=configHandler->Get("MultiThreadDrawGroundShadow", 0);
#endif
}

CBFGroundDrawer::~CBFGroundDrawer(void)
{
	delete textures;

	if (shadowHandler->canUseShadows) {
		glSafeDeleteProgram(groundVP);
		glSafeDeleteProgram(groundShadowVP);

		if (shadowHandler->useFPShadows) {
			glSafeDeleteProgram( groundFPShadow);
		}
	}

	configHandler->Set("GroundDetail", viewRadius);

	if (waterPlaneCamInDispList) {
		glDeleteLists(waterPlaneCamInDispList,1);
		glDeleteLists(waterPlaneCamOutDispList,1);
	}

#ifdef USE_GML
	configHandler->Set("MultiThreadDrawGround", multiThreadDrawGround);
	configHandler->Set("MultiThreadDrawGroundShadow", multiThreadDrawGroundShadow);
#endif
}

void CBFGroundDrawer::CreateWaterPlanes(const bool &camOufOfMap) {
	glDisable(GL_TEXTURE_2D);

	const float xsize = (gs->mapx * SQUARE_SIZE) >> 2;
	const float ysize = (gs->mapy * SQUARE_SIZE) >> 2;

	CVertexArray *va = GetVertexArray();
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

		if ((n==1) && !camOufOfMap) {
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
}

inline void CBFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (!drawWaterReflection) {
		const bool camOutOfMap = !camera->pos.IsInBounds();
		glCallList(camOutOfMap ? waterPlaneCamOutDispList : waterPlaneCamInDispList);
	}
}


inline void CBFGroundDrawer::DrawVertexAQ(CVertexArray *ma, int x, int y)
{
	float height = heightData[y * heightDataX + x];
	if (waterDrawn && height < 0) {
		height *= 2;
	}

	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}

inline void CBFGroundDrawer::DrawVertexAQ(CVertexArray *ma, int x, int y, float height)
{
	if (waterDrawn && height < 0) {
		height *= 2;
	}
	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}

inline void CBFGroundDrawer::EndStripQ(CVertexArray *ma)
{
	ma->EndStripQ();
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


inline void CBFGroundDrawer::DrawGroundVertexArrayQ(CVertexArray * &ma)
{
	ma->DrawArray0(GL_TRIANGLE_STRIP);
	ma = GetVertexArray();
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

				int nloop=(xe-xs)/lod+1;
				ma->EnlargeArrays(52*nloop, 14*nloop+1); //! includes one extra for final endstrip

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

			int yst=max(ystart - lod, minty);
			int yed=min(yend + lod, maxty);
			int nloop=(yed-yst)/lod+1;

			if (nloop > 0)
				ma->EnlargeArrays(8*nloop, 2*nloop);

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
					int nloop=(xe-xs)/lod+2; //! one extra for if statment
					ma->EnlargeArrays(2*nloop, 1);
					int ylhdx=(y + lod) * heightDataX;
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
					int nloop=(xe-xs)/lod+2; //! one extra for if statment
					ma->EnlargeArrays(2*nloop, 1);
					int yhdx=y * heightDataX;
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

void CBFGroundDrawer::Draw(bool drawWaterReflection, bool drawUnitReflection, unsigned int VP)
{
	if (mapInfo->map.voidWater && map->currMaxHeight<0) {
		return;
	}

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	overrideVP = VP;
	waterDrawn = drawWaterReflection;
	int baseViewRadius = max(4, viewRadius);

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
	viewRadius  = (int) (viewRadius * fastmath::sqrt(zoom));
	viewRadius  = max(max(numBigTexY,numBigTexX), viewRadius);
	viewRadius += (viewRadius & 1); //! we need a multiple of 2
	neededLod   = int((gu->viewRange * 0.125f) / viewRadius) << 1;

	UpdateCamRestraints();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	if (!overrideVP)
		glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetupTextureUnits(drawWaterReflection);

	if (mapInfo->map.voidWater && !waterDrawn) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.9f);
	}

#ifdef USE_GML
	if(multiThreadDrawGround) {
		gmlProcessor->Work(NULL,&CBFGroundDrawer::DoDrawGroundRowMT,NULL,this,gmlThreadCount,FALSE,NULL,numBigTexY,50,100,TRUE,NULL);
	}
	else {
#endif
		int camBty = (int)math::floor(cam2->pos.z / (bigSquareSize * SQUARE_SIZE));
		camBty = std::max(0,std::min(numBigTexY-1, camBty ));

		//! try to render in "front to back" (so start with the camera nearest BigGroundLines)
		for (int bty = camBty; bty >= 0; --bty) {
			DoDrawGroundRow(bty);
		}
		for (int bty = camBty+1; bty < numBigTexY; ++bty) {
			DoDrawGroundRow(bty);
		}
#ifdef USE_GML
	}
#endif

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
			ph->DrawGroundFlashes();
			glDepthMask(1);
		}
	}

//	sky->SetCloudShadow(1);
//	if (drawWaterReflection)
//		treeDistance *= 0.5f;

	viewRadius = baseViewRadius;
	overrideVP = NULL;
}


inline void CBFGroundDrawer::DoDrawGroundShadowLOD(int nlod) {
	CVertexArray *ma = GetVertexArray();
	ma->Initialize();

	bool inStrip=false;
	int x,y;
	int lod=1<<nlod;

	float cx2 = camera->pos.x / SQUARE_SIZE;
	float cy2 = camera->pos.z / SQUARE_SIZE;

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

		int nloop=(xe-xs)/lod+1;
		ma->EnlargeArrays(52*nloop, 14*nloop+1); //! includes one extra for final endstrip
		int ydx=y*heightDataX;
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

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
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
		ma->EnlargeArrays(8*nloop, 2*nloop);

	//!rita yttre begr?snings yta mot n?ta lod
	if(maxlx<maxtx && maxlx>=mintx){
		x=maxlx;
		int xlod = x + lod;
		for(y=yst;y<yed;y+=lod){
			DrawVertexAQ(ma, x,y);
			DrawVertexAQ(ma, x,y+lod);
			int yhdx=y*heightDataX+x;
			if(y%dlod){
				float h=(heightData[yhdx-lhdx+lod]+heightData[yhdx+lhdx+lod]) * hmcxp + heightData[yhdx+lod] * camxpart;
				DrawVertexAQ(ma, xlod,y,h);
				DrawVertexAQ(ma, xlod,y+lod);
			} else {
				float h=(heightData[yhdx+lod]+heightData[yhdx+dhdx+lod]) * hmcxp + heightData[yhdx+lhdx+lod] * camxpart;
				DrawVertexAQ(ma, xlod,y);
				DrawVertexAQ(ma, xlod,y+lod,h);
			}
			EndStripQ(ma);
		}
	}
	if(minlx>mintx && minlx<maxtx){
		x=minlx-lod;
		int xlod = x + lod;
		for(y=yst;y<yed;y+=lod){
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
		if(xs<xe){
			x=xs;
			int ylod = y + lod;
			int nloop=(xe-xs)/lod+2; //! two extra for if statment
			ma->EnlargeArrays(2*nloop, 1);
			int ydx=y*heightDataX;
			if(x%dlod){
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
			int nloop=(xe-xs)/lod+2; //! two extra for if statment
			ma->EnlargeArrays(2*nloop, 1);
			int ydx=y*heightDataX;
			if(x%dlod){
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
	if (mapInfo->map.voidWater && map->currMaxHeight<0) {
		return;
	}

//	glEnable(GL_CULL_FACE);
	const int NUM_LODS = 4;

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, groundShadowVP);
	glEnable(GL_VERTEX_PROGRAM_ARB);

#ifdef USE_GML
	if(multiThreadDrawGroundShadow) {
		gmlProcessor->Work(NULL,&CBFGroundDrawer::DoDrawGroundShadowLODMT,NULL,this,gmlThreadCount,FALSE,NULL,NUM_LODS+1,50,100,TRUE,NULL);
	}
	else {
#endif
		for (int nlod = 0; nlod < NUM_LODS+1; ++nlod) {
			DoDrawGroundShadowLOD(nlod);
		}
#ifdef USE_GML
	}
#endif

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_VERTEX_PROGRAM_ARB);
}


inline void CBFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	//! must be in drawLos mode or shadows must be off
	if (DrawExtraTex() || !shadowHandler->drawShadows) {
		textures->SetTexture(bigSquareX, bigSquareY);
		SetTexGen(1.0f / 1024, 1.0f / 1024, -bigSquareX, -bigSquareY);

		if (overrideVP) {
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 11, -bigSquareX, -bigSquareY, 0, 0);
		}
	} else {
		textures->SetTexture(bigSquareX, bigSquareY);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 11, -bigSquareX, -bigSquareY, 0, 0);
	}
}


void CBFGroundDrawer::SetupTextureUnits(bool drawReflection)
{
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	if (DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		if (map->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
			glMultiTexCoord1f(GL_TEXTURE2_ARB,0); //fixes a nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			//SetTexGen(0.02f,0.02f, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f));
			GLfloat plan[]={0.02f,0.5f,0,0};
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
			glTexGenfv(GL_S,GL_EYE_PLANE,plan);
			glEnable(GL_TEXTURE_GEN_S);
			GLfloat plan2[]={0,0.5f,0.02f,0};
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
			glTexGenfv(GL_T,GL_EYE_PLANE,plan2);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glDisable (GL_TEXTURE_2D);
		}

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, infoTex);
		glMultiTexCoord1f(GL_TEXTURE1_ARB,0); //fixes a nvidia bug with gltexgen
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		if (overrideVP) {
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
		}
	}
	else if (shadowHandler->drawShadows) {
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, groundFPShadow);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		float3 ac = mapInfo->light.groundAmbientColor * (210.0f / 255.0f);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 10, ac.x, ac.y, ac.z, 1);
		glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 11, 0, 0, 0, mapInfo->light.groundShadowDensity);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glMultiTexCoord1f(GL_TEXTURE1_ARB,0); //fixes a nvidia bug with gltexgen
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		glActiveTextureARB(GL_TEXTURE2_ARB);

		if (map->detailTex) {
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glActiveTextureARB(GL_TEXTURE4_ARB);
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);

		glActiveTextureARB(GL_TEXTURE0_ARB);

		if (drawReflection) {
			glAlphaFunc(GL_GREATER, 0.8f);
			glEnable(GL_ALPHA_TEST);
		}
		if (overrideVP) {
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, overrideVP);
		} else {
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, groundVP);
		}

		glEnable(GL_VERTEX_PROGRAM_ARB);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 12, 1.0f / 1024, 1.0f / 1024, 0, 1);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
		glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 14, 0.02f, 0.02f, 0, 1);

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->shadowMatrix.m);
		glMatrixMode(GL_MODELVIEW);
	} else {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, map->GetShadingTexture());
		glMultiTexCoord1f(GL_TEXTURE1_ARB,0); //fixes a nvidia bug with gltexgen
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE2_ARB);

		if (map->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
			glMultiTexCoord1f(GL_TEXTURE2_ARB,0); //fixes a nvidia bug with gltexgen
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			//SetTexGen(0.02f, 0.02f, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f));
			GLfloat plan[]={0.02f,0,0,0};
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
			glTexGenfv(GL_S,GL_OBJECT_PLANE,plan);
			glEnable(GL_TEXTURE_GEN_S);
			GLfloat plan2[]={0,0,0.02f,0};
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
			glTexGenfv(GL_T,GL_OBJECT_PLANE,plan2);
			glEnable(GL_TEXTURE_GEN_T);
		} else {
			glDisable (GL_TEXTURE_2D);
		}

		if (overrideVP) {
			glEnable(GL_VERTEX_PROGRAM_ARB);
			glBindProgramARB(GL_VERTEX_PROGRAM_ARB, overrideVP);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,12, 1.0f / 1024, 1.0f / 1024, 0, 1);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,13, -floor(camera->pos.x * 0.02f),-floor(camera->pos.z * 0.02f), 0, 0);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB,14, 0.02f, 0.02f, 0,1);

			if (drawReflection) {
				glAlphaFunc(GL_GREATER, 0.9f);
				glEnable(GL_ALPHA_TEST);
			}
		}
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
}


void CBFGroundDrawer::ResetTextureUnits(bool drawReflection)
{
	if (DrawExtraTex() || !shadowHandler->drawShadows) {
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		if (overrideVP) {
			glDisable(GL_VERTEX_PROGRAM_ARB);

			if (drawReflection) {
				glDisable(GL_ALPHA_TEST);
			}
		}
	} else {
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);

		glActiveTextureARB(GL_TEXTURE4_ARB);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		if (drawReflection) {
			glDisable(GL_ALPHA_TEST);
		}
	}
}



void CBFGroundDrawer::AddFrustumRestraint(const float3& side)
{
	fline temp;
	static const float3 up(0, 1, 0);

	// get vector for collision between frustum and horizontal plane
	float3 b = up.cross(side);

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
	static const float3 up(0, 1, 0);
	float3 side = cam2->forward;
	float3 camHorizontal = cam2->forward;
	camHorizontal.y = 0;
	camHorizontal.ANormalize();
	// get vector for collision between frustum and horizontal plane
	float3 b = up.cross(camHorizontal);

	if (fabs(b.z) > 0.0001f) {
		temp.dir = b.x / b.z;               // set direction to that
		float3 c = b.cross(camHorizontal);  // get vector from camera to collision line
		float3 colpoint;                    // a point on the collision line

		if (side.y > 0)
			colpoint = cam2->pos + camHorizontal * gu->viewRange * 1.05f - c * (cam2->pos.y / c.y);
		else
			colpoint = cam2->pos + camHorizontal * gu->viewRange * 1.05f - c * ((cam2->pos.y - 255 / 3.5f) / c.y);

		// get intersection between colpoint and z axis
		temp.base = (colpoint.x - colpoint.z * temp.dir) / SQUARE_SIZE;

		if (b.z > 0) {
			left.push_back(temp);
		} else {
			right.push_back(temp);
		}
	}

}

void CBFGroundDrawer::Update()
{
	if (mapInfo->map.voidWater && map->currMaxHeight<0) {
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
