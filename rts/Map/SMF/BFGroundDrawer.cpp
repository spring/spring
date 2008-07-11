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
#include "Platform/ConfigHandler.h"
#include "mmgr.h"

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int,CUnit*> gmlProcessor;
#endif

using std::min;
using std::max;

CBFGroundDrawer::CBFGroundDrawer(CSmfReadMap* rm) :
	bigSquareSize(128),
	numBigTexX(gs->mapx / bigSquareSize),
	numBigTexY(gs->mapy / bigSquareSize),
	heightDataX(gs->mapx + 1)
{
	map = rm;

	heightData = map->heightmap;

	if (shadowHandler->canUseShadows) {
		groundVP = LoadVertexProgram("ground.vp");
		groundShadowVP = LoadVertexProgram("groundshadow.vp");

		if (shadowHandler->useFPShadows) {
			groundFPShadow = LoadFragmentProgram("groundFPshadow.fp");
		}
	}

	textures = SAFE_NEW CBFGroundTextures(map);

	viewRadius = configHandler.GetInt("GroundDetail", 40);
	viewRadius += (viewRadius & 1);

	waterDrawn = false;
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

	configHandler.SetInt("GroundDetail", viewRadius);
}



inline void CBFGroundDrawer::DrawVertexA(int x, int y)
{
	float height = heightData[y * heightDataX + x];
	if (waterDrawn && height < 0) {
		height *= 2;
	}

	va->AddVertex0(float3(x * SQUARE_SIZE, height, y * SQUARE_SIZE));
}

inline void CBFGroundDrawer::DrawVertexA(int x, int y, float height)
{
	if (waterDrawn && height < 0) {
		height *= 2;
	}
	va->AddVertex0(float3(x * SQUARE_SIZE, height, y * SQUARE_SIZE));
}

inline void CBFGroundDrawer::EndStrip()
{
	va->EndStrip();
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
	static int mapWidth = (gs->mapx << 3);
	static int bigTexH = (gs->mapy << 3) / numBigTexY;

	const int minx =                   0;
	const int maxx =            mapWidth;
	const int minz = bty * bigTexH;
	const int maxz = minz + bigTexH;
	const float miny = readmap->minheight;
	const float maxy = fabs(cam2->pos.y); // ??

	const float3 mins(minx, miny, minz);
	const float3 maxs(maxx, maxy, maxz);

	return (cam2->InView(mins, maxs));
}



inline void CBFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (!drawWaterReflection) {
		glDisable(GL_TEXTURE_2D);
		glColor3f(mapInfo->water.planeColor.x, mapInfo->water.planeColor.y, mapInfo->water.planeColor.z);
		glBegin(GL_QUADS);

		static const float xsize = (gs->mapx * SQUARE_SIZE) >> 2;
		static const float ysize = (gs->mapy * SQUARE_SIZE) >> 2;
		const bool camOutMap = (camera->pos.x < 0 || camera->pos.z < 0 || camera->pos.x > float3::maxxpos || camera->pos.z > float3::maxzpos);

		for (int y = -4; y < 8; ++y) {
			for (int x = -4; x < 8; ++x) {
				if (x > 3 || x < 0 || y > 3 || y < 0 || camOutMap) {
					glVertex3f( x      * xsize, -200,  y      * ysize);
					glVertex3f((x + 1) * xsize, -200,  y      * ysize);
					glVertex3f((x + 1) * xsize, -200, (y + 1) * ysize);
					glVertex3f( x      * xsize, -200, (y + 1) * ysize);
				}
			}
		}
		glEnd();
	}
}

inline void CBFGroundDrawer::DrawGroundVertexArray()
{
	va->DrawArray0(GL_TRIANGLE_STRIP);
	va = GetVertexArray();
	va->Initialize();
}


inline void CBFGroundDrawer::DrawGroundVertexArrayQ(CVertexArray * &ma)
{
	ma->DrawArray0(GL_TRIANGLE_STRIP);
	ma = GetVertexArray();
	ma->Initialize();
}

int neededLod=0;
int maxIdx=0;

inline void CBFGroundDrawer::DoDrawGroundRow(int bty, unsigned int overrideVP) {
#define CLAMP(i) std::max(0, std::min((i), maxIdx))
	if (!BigTexSquareRowVisible(bty)) {
		// skip this entire row of squares if we can't see it
		return;
	}

	CVertexArray *ma = GetVertexArray();
	ma->Initialize();

	bool inStrip = false;
	int x,y;
	// only process the necessary big squares in the x direction
	int sx = 0;
	int ex = numBigTexX;
	float x0, x1;
	std::vector<fline>::iterator fli;


	for (fli = left.begin(); fli != left.end(); fli++) {
		x0 = ((fli->base / SQUARE_SIZE + fli->dir *  (bty * bigSquareSize)                 ));
		x1 = ((fli->base / SQUARE_SIZE + fli->dir * ((bty * bigSquareSize) + bigSquareSize)));

		if (x0 > x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 > sx)
			sx = (int) x0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		x0 = ((fli->base / SQUARE_SIZE + fli->dir *  (bty * bigSquareSize)                 )) + bigSquareSize;
		x1 = ((fli->base / SQUARE_SIZE + fli->dir * ((bty * bigSquareSize) + bigSquareSize))) + bigSquareSize;

		if (x0 < x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 < ex)
			ex = (int) x0;
	}


	for (int btx = sx; btx < ex; ++btx) {
		// must be in drawLos mode or shadows must be off
		if (DrawExtraTex() || !shadowHandler->drawShadows) {
			textures->SetTexture(btx, bty);
			SetTexGen(1.0f / 1024, 1.0f / 1024, -btx, -bty);

			if (overrideVP) {
				glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 11, -btx, -bty, 0, 0);
			}
		} else {
			textures->SetTexture(btx, bty);
			glProgramEnvParameter4fARB(GL_VERTEX_PROGRAM_ARB, 11, -btx, -bty, 0, 0);
		}

		for (int lod = 1; lod < neededLod; lod <<= 1) {
			float cx2 = (cam2->pos.x / (SQUARE_SIZE));
			float cy2 = (cam2->pos.z / (SQUARE_SIZE));

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
				oldcamxpart = (cx2 - cx2o) / (lod);
				oldcamypart = (cy2 - cy2o) / (lod);
			}

			cx = (cx / lod) * lod;
			cy = (cy / lod) * lod;
			int ysquaremod = ((cy) % (dlod)) / lod;
			int xsquaremod = ((cx) % (dlod)) / lod;

			float camxpart = (cx2 - ((cx / (dlod)) * dlod)) / (dlod);
			float camypart = (cy2 - ((cy / (dlod)) * dlod)) / (dlod);

			int minty =  bty      * bigSquareSize;
			int maxty = (bty + 1) * bigSquareSize;
			int mintx =  btx      * bigSquareSize;
			int maxtx = (btx + 1) * bigSquareSize;

			int minly = cy + (-viewRadius + 3 - ysquaremod) * lod;
			int maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
			int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod;
			int maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

			int xstart = max(minlx, mintx);
			int xend   = min(maxlx, maxtx);
			int ystart = max(minly, minty);
			int yend   = min(maxly, maxty);

			for (y = ystart; y < yend; y += lod) {
				int xs = xstart;
				int xe = xend;
				int xt0, xt1;
				std::vector<fline>::iterator fli;


				for (fli = left.begin(); fli != left.end(); fli++) {
					xt0 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir *  y       )) / lod * lod - lod;
					xt1 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir * (y + lod))) / lod * lod - lod;

					if (xt0 > xt1) xt0 = xt1;
					if (xt0 > xs) xs = xt0;
				}
				for (fli = right.begin(); fli != right.end(); fli++) {
					xt0 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir *  y       )) / lod * lod + lod;
					xt1 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir * (y + lod))) / lod * lod + lod;

					if (xt0 < xt1) xt0 = xt1;
					if (xt0 < xe) xe = xt0;
				}

				int nloop=(xe-xs)/lod+1;
				ma->EnlargeArrays(52*nloop, 14*nloop+1); // includes one extra for final endstrip

				for (x = xs; x < xe; x += lod) {
					if ((lod == 1) ||
						(x > (cx) + viewRadius * hlod) || (x < (cx) - viewRadius * hlod) ||
						(y > (cy) + viewRadius * hlod) || (y < (cy) - viewRadius * hlod)) {
							// normal terrain
							if (!inStrip) {
								DrawVertexAQ(ma, x, y      );
								DrawVertexAQ(ma, x, y + lod);
								inStrip = true;
							}

							DrawVertexAQ(ma, x + lod, y      );
							DrawVertexAQ(ma, x + lod, y + lod);
					} else {
						// inre begr�sning mot f�eg�nde lod
						if ((x >= (cx) + viewRadius * hlod)) {
							int idx1 = CLAMP((y       ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP((y +  lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP((y + hlod) * heightDataX + x),                              idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1] + heightData[idx2   ]) * 0.5f * (1 - oldcamxpart) + heightData[idx3    ] * (oldcamxpart);
							float h2 = (heightData[idx1] + heightData[idx1LOD]) * 0.5f * (1 - oldcamxpart) + heightData[idx1HLOD] * (oldcamxpart);
							float h3 = (heightData[idx2] + heightData[idx1LOD]) * 0.5f * (1 - oldcamxpart) + heightData[idx3HLOD] * (oldcamxpart);
							float h4 = (heightData[idx2] + heightData[idx2LOD]) * 0.5f * (1 - oldcamxpart) + heightData[idx2HLOD] * (oldcamxpart);

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x,        y           );
							DrawVertexAQ(ma, x,        y + hlod, h1);
							DrawVertexAQ(ma, x + hlod, y,        h2);
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							EndStripQ(ma);
							DrawVertexAQ(ma, x,        y + hlod, h1);
							DrawVertexAQ(ma, x,        y +  lod    );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x + hlod, y +  lod, h4);
							EndStripQ(ma);
							DrawVertexAQ(ma, x + hlod, y +  lod, h4);
							DrawVertexAQ(ma, x +  lod, y +  lod    );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x +  lod, y           );
							DrawVertexAQ(ma, x + hlod, y,        h2);
							EndStripQ(ma);
						}
						if ((x <= (cx) - viewRadius * hlod)) {
							int idx1 = CLAMP((y       ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP((y +  lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP((y + hlod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1LOD] + heightData[idx2LOD]) * 0.5f * (oldcamxpart) + heightData[idx3LOD ] * (1 - oldcamxpart);
							float h2 = (heightData[idx1   ] + heightData[idx1LOD]) * 0.5f * (oldcamxpart) + heightData[idx1HLOD] * (1 - oldcamxpart);
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * 0.5f * (oldcamxpart) + heightData[idx3HLOD] * (1 - oldcamxpart);
							float h4 = (heightData[idx2   ] + heightData[idx2LOD]) * 0.5f * (oldcamxpart) + heightData[idx2HLOD] * (1 - oldcamxpart);

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x +  lod, y + hlod, h1);
							DrawVertexAQ(ma, x +  lod, y           );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x + hlod, y,        h2);
							EndStripQ(ma);
							DrawVertexAQ(ma, x +  lod, y +  lod    );
							DrawVertexAQ(ma, x +  lod, y + hlod, h1);
							DrawVertexAQ(ma, x + hlod, y +  lod, h4);
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							EndStripQ(ma);
							DrawVertexAQ(ma, x + hlod, y,        h2);
							DrawVertexAQ(ma, x,        y           );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x,        y +  lod    );
							DrawVertexAQ(ma, x + hlod, y +  lod, h4);
							EndStripQ(ma);
						}
						if ((y >= (cy) + viewRadius * hlod)) {
							int idx1 = (y       ) * heightDataX + x, idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = (y +  lod) * heightDataX + x, idx2LOD = CLAMP(idx2 + lod);
							int idx3 = (y + hlod) * heightDataX + x, idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1   ] + heightData[idx1LOD]) * 0.5f * (1 - oldcamypart) + heightData[idx1HLOD] * (oldcamypart);
							float h2 = (heightData[idx1   ] + heightData[idx2   ]) * 0.5f * (1 - oldcamypart) + heightData[idx3    ] * (oldcamypart);
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * 0.5f * (1 - oldcamypart) + heightData[idx3HLOD] * (oldcamypart);
							float h4 = (heightData[idx2LOD] + heightData[idx1LOD]) * 0.5f * (1 - oldcamypart) + heightData[idx3LOD ] * (oldcamypart);

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x,        y           );
							DrawVertexAQ(ma, x,        y + hlod, h2);
							DrawVertexAQ(ma, x + hlod, y,        h1);
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x +  lod, y           );
							DrawVertexAQ(ma, x +  lod, y + hlod, h4);
							EndStripQ(ma);
							DrawVertexAQ(ma, x,        y + hlod, h2);
							DrawVertexAQ(ma, x,        y +  lod    );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x +  lod, y +  lod    );
							DrawVertexAQ(ma, x +  lod, y + hlod, h4);
							EndStripQ(ma);
						}
						if ((y <= (cy) - viewRadius * hlod)) {
							int idx1 = CLAMP((y       ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
							int idx2 = CLAMP((y +  lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP((y + hlod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx2   ] + heightData[idx2LOD]) * 0.5f * (oldcamypart) + heightData[idx2HLOD] * (1 - oldcamypart);
							float h2 = (heightData[idx1   ] + heightData[idx2   ]) * 0.5f * (oldcamypart) + heightData[idx3    ] * (1 - oldcamypart);
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * 0.5f * (oldcamypart) + heightData[idx3HLOD] * (1 - oldcamypart);
							float h4 = (heightData[idx2LOD] + heightData[idx1LOD]) * 0.5f * (oldcamypart) + heightData[idx3LOD ] * (1 - oldcamypart);

							if (inStrip) {
								EndStripQ(ma);
								inStrip = false;
							}

							DrawVertexAQ(ma, x,        y + hlod, h2);
							DrawVertexAQ(ma, x,        y +  lod    );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x + hlod, y +  lod, h1);
							DrawVertexAQ(ma, x +  lod, y + hlod, h4);
							DrawVertexAQ(ma, x +  lod, y +  lod    );
							EndStripQ(ma);
							DrawVertexAQ(ma, x +  lod, y + hlod, h4);
							DrawVertexAQ(ma, x +  lod, y           );
							DrawVertexAQ(ma, x + hlod, y + hlod, h3);
							DrawVertexAQ(ma, x,        y           );
							DrawVertexAQ(ma, x,        y + hlod, h2);
							EndStripQ(ma);
						}
					}
				}

				if (inStrip) {
					EndStripQ(ma);
					inStrip = false;
				}
			}

			int yst=max(ystart - lod, minty);
			int yed=min(yend + lod, maxty);
			int nloop=(yed-yst)/lod+1;
			ma->EnlargeArrays(8*nloop, 2*nloop);

			// rita yttre begr�snings yta mot n�ta lod
			if (maxlx < maxtx && maxlx >= mintx) {
				x = maxlx;
				for (y = yst; y < yed; y += lod) {
					DrawVertexAQ(ma, x, y      );
					DrawVertexAQ(ma, x, y + lod);

					if (y % (dlod)) {
						int idx1 = CLAMP((y      ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
						int idx2 = CLAMP((y + lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod);
						int idx3 = CLAMP((y - lod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod);
						float h = ((heightData[idx3LOD] +
							heightData[idx2LOD]) * 0.5f) * (1 - camxpart) +
							heightData[idx1LOD] * (camxpart);

						DrawVertexAQ(ma, x + lod, y,       h);
						DrawVertexAQ(ma, x + lod, y + lod   );
					} else {
						int idx1 = CLAMP((y       ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
						int idx2 = CLAMP((y +  lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod);
						int idx3 = CLAMP((y + dlod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod);
						float h = (heightData[idx1LOD] +
							heightData[idx3LOD]) * 0.5f * (1 - camxpart) +
							heightData[idx2LOD] * (camxpart);

						DrawVertexAQ(ma, x + lod, y);
						DrawVertexAQ(ma, x + lod, y + lod, h);
					}
					EndStripQ(ma);
				}
			}

			if (minlx > mintx && minlx < maxtx) {
				x = minlx - lod;
				for (y = yst; y < yed; y += lod) {
					if (y % (dlod)) {
						int idx1 = CLAMP((y      ) * heightDataX + x);
						int idx2 = CLAMP((y + lod) * heightDataX + x);
						int idx3 = CLAMP((y - lod) * heightDataX + x);
						float h = ((heightData[idx3] +
							heightData[idx2]) * 0.5f) * (camxpart) +
							heightData[idx1] * (1 - camxpart);

						DrawVertexAQ(ma, x, y,       h);
						DrawVertexAQ(ma, x, y + lod   );
					} else {
						int idx1 = CLAMP((y       ) * heightDataX + x);
						int idx2 = CLAMP((y +  lod) * heightDataX + x);
						int idx3 = CLAMP((y + dlod) * heightDataX + x);
						float h = (heightData[idx1] +
							heightData[idx3]) * 0.5f * (camxpart) +
							heightData[idx2] * (1 - camxpart);

						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, y + lod, h);
					}
					DrawVertexAQ(ma, x + lod, y      );
					DrawVertexAQ(ma, x + lod, y + lod);
					EndStripQ(ma);
				}
			}

			if (maxly < maxty && maxly > minty) {
				y = maxly;
				int xs = max(xstart - lod, mintx);
				int xe = min(xend + lod,   maxtx);
				int xt0, xt1;
				std::vector<fline>::iterator fli;


				for (fli = left.begin(); fli != left.end(); fli++) {
					xt0 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir *  y       )) / lod * lod - lod;
					xt1 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir * (y + lod))) / lod * lod - lod;

					if (xt0 > xt1) xt0 = xt1;
					if (xt0 > xs) xs = xt0;
				}
				for (fli = right.begin(); fli != right.end(); fli++) {
					xt0 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir *  y       )) / lod * lod + lod;
					xt1 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir * (y + lod))) / lod * lod + lod;

					if (xt0 < xt1) xt0 = xt1;
					if (xt0 < xe) xe = xt0;
				}


				if (xs < xe) {
					x = xs;
					int nloop=(xe-xs)/lod+2; // one extra for if statment
					ma->EnlargeArrays(2*nloop, 1);
					if (x % (dlod)) {
						int idx2     = CLAMP((y + lod) * heightDataX + x),
							idx2PLOD = CLAMP(idx2 + lod),
							idx2MLOD = CLAMP(idx2 - lod);
						float h = ((heightData[idx2MLOD] +
							heightData[idx2PLOD]) * 0.5f) * (1 - camypart) +
							heightData[idx2    ] * (camypart);

						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, y + lod, h);
					} else {
						DrawVertexAQ(ma, x, y      );
						DrawVertexAQ(ma, x, y + lod);
					}
					for (x = xs; x < xe; x += lod) {
						if (x % (dlod)) {
							DrawVertexAQ(ma, x + lod, y      );
							DrawVertexAQ(ma, x + lod, y + lod);
						} else {
							int idx2      = CLAMP((y + lod) * heightDataX + x),
								idx2PLOD  = CLAMP(idx2 +  lod),
								idx2PLOD2 = CLAMP(idx2 + dlod);
							float h = (heightData[idx2PLOD2] +
								heightData[idx2     ]) * 0.5f * (1 - camypart) +
								heightData[idx2PLOD ] * (camypart);

							DrawVertexAQ(ma, x + lod, y);
							DrawVertexAQ(ma, x + lod, y + lod, h);
						}
					}
					EndStripQ(ma);
				}
			}

			if (minly > minty && minly < maxty) {
				y = minly - lod;
				int xs = max(xstart - lod, mintx);
				int xe = min(xend + lod,   maxtx);
				int xt0, xt1;
				std::vector<fline>::iterator fli;


				for (fli = left.begin(); fli != left.end(); fli++) {
					xt0 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir *  y       )) / lod * lod - lod;
					xt1 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir * (y + lod))) / lod * lod - lod;

					if (xt0 > xt1) xt0 = xt1;
					if (xt0 > xs) xs = xt0;
				}
				for (fli = right.begin(); fli != right.end(); fli++) {
					xt0 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir *  y       )) / lod * lod + lod;
					xt1 = ((int) (fli->base / (SQUARE_SIZE) + fli->dir * (y + lod))) / lod * lod + lod;

					if (xt0 < xt1) xt0 = xt1;
					if (xt0 < xe) xe = xt0;
				}


				if (xs < xe) {
					x = xs;
					int nloop=(xe-xs)/lod+2; // one extra for if statment
					ma->EnlargeArrays(2*nloop, 1);
					if (x % (dlod)) {
						int idx1     = CLAMP((y) * heightDataX + x),
							idx1PLOD = CLAMP(idx1 + lod),
							idx1MLOD = CLAMP(idx1 - lod);
						float h = ((heightData[idx1MLOD] +
							heightData[idx1PLOD]) * 0.5f) * (camypart) +
							heightData[idx1    ] * (1 - camypart);

						DrawVertexAQ(ma, x, y,       h);
						DrawVertexAQ(ma, x, y + lod   );
					} else {
						DrawVertexAQ(ma, x, y      );
						DrawVertexAQ(ma, x, y + lod);
					}

					for (x = xs; x < xe; x+= lod) {
						if (x % (dlod)) {
							DrawVertexAQ(ma, x + lod, y      );
							DrawVertexAQ(ma, x + lod, y + lod);
						} else {
							int idx1      = CLAMP((y) * heightDataX + x),
								idx1PLOD  = CLAMP(idx1 +  lod),
								idx1PLOD2 = CLAMP(idx1 + dlod);
							float h = (heightData[idx1PLOD2] +
								heightData[idx1     ]) * 0.5f * (camypart) +
								heightData[idx1PLOD ] * (1 - camypart);

							DrawVertexAQ(ma, x + lod, y,       h);
							DrawVertexAQ(ma, x + lod, y + lod   );
						}
					}
					EndStripQ(ma);
				}
			}
		}
		DrawGroundVertexArrayQ(ma);
	}
}


void CBFGroundDrawer::Draw(bool drawWaterReflection, bool drawUnitReflection, unsigned int overrideVP)
{
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	waterDrawn = drawWaterReflection;
	int baseViewRadius = max(4, viewRadius);

	if (drawUnitReflection) {
		viewRadius = (viewRadius >> 1) & 0xfffffe;
	}

	float zoom = 45.0f / camera->GetFov();
	viewRadius = (int) (viewRadius * sqrt(zoom));
	viewRadius += (viewRadius & 1);

	va = GetVertexArray();
	va->Initialize();
	textures->DrawUpdate();

	int x, y;
	neededLod = int((gu->viewRange * 0.125f) / viewRadius) << 1;
	maxIdx = ((gs->mapx + 1) * (gs->mapy + 1)) - 1;
	#define CLAMP(i) std::max(0, std::min((i), maxIdx))

	UpdateCamRestraints();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	if (!overrideVP)
		glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetupTextureUnits(drawWaterReflection, overrideVP);
	bool inStrip = false;

	if (mapInfo->map.voidWater && !waterDrawn) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.9f);
	}

	float camxpart = 0.0f, oldcamxpart;
	float camypart = 0.0f, oldcamypart;

#if GML_ENABLE_DRAWGROUND
	mt_overrideVP=overrideVP;
	gmlProcessor.Work(NULL,&CBFGroundDrawer::DoDrawGroundRowMT,NULL,this,gmlThreadCount,FALSE,NULL,numBigTexY,50,100,TRUE,NULL);
#else
	for (int bty = 0; bty < numBigTexY; ++bty) {
		DoDrawGroundRow(bty,overrideVP);
	}
#endif

	ResetTextureUnits(drawWaterReflection, overrideVP);
	glDisable(GL_CULL_FACE);

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (mapInfo->map.voidWater && !waterDrawn) {
		glDisable(GL_ALPHA_TEST);
	}

	if (mapInfo->hasWaterPlane) {
		DrawWaterPlane(drawWaterReflection);
	}

	if (groundDecals && !(drawWaterReflection || drawUnitReflection)) {
		groundDecals->Draw();
	}

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

//	sky->SetCloudShadow(1);
//	if (drawWaterReflection)
//		treeDistance *= 0.5f;

	ph->DrawGroundFlashes();


	if (treeDrawer->drawTrees) {
		if (DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			glBindTexture(GL_TEXTURE_2D, infoTex);
			SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		treeDrawer->Draw(drawWaterReflection || drawUnitReflection);

		if (DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);
	viewRadius = baseViewRadius;
}


inline void CBFGroundDrawer::DoDrawGroundShadowLOD(int nlod) {
	CVertexArray *ma = GetVertexArray();
	ma->Initialize();

	bool inStrip=false;
	int x,y;
	int lod=1<<nlod;

	float cx2 = (camera->pos.x / (SQUARE_SIZE));
	float cy2 = (camera->pos.z / (SQUARE_SIZE));

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
		oldcamxpart = (cx2 - cx2o) / (lod);
		oldcamypart = (cy2 - cy2o) / (lod);
	}

	cx = (cx / lod) * lod;
	cy = (cy / lod) * lod;
	int ysquaremod = ((cy) % (dlod)) / lod;
	int xsquaremod = ((cx) % (dlod)) / lod;

	float camxpart = (cx2 - ((cx / (dlod)) * dlod)) / (dlod);
	float camypart = (cy2 - ((cy / (dlod)) * dlod)) / (dlod);

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

	for (y = ystart; y < yend; y += lod) {
		int xs = xstart;
		int xe = xend;

		int nloop=(xe-xs)/lod+1;
		ma->EnlargeArrays(52*nloop, 14*nloop+1); // includes one extra for final endstrip
		for (x = xs; x < xe; x += lod) {
			if ((lod == 1) ||
				(x > (cx) + viewRadius * hlod) || (x < (cx) - viewRadius * hlod) ||
				(y > (cy) + viewRadius * hlod) || (y < (cy) - viewRadius * hlod)) {
					if (!inStrip) {
						DrawVertexAQ(ma, x, y      );
						DrawVertexAQ(ma, x, y + lod);
						inStrip = true;
					}
					DrawVertexAQ(ma, x + lod, y      );
					DrawVertexAQ(ma, x + lod, y + lod);
			} else {  //inre begr�sning mot f�eg�nde lod
				if((x>=(cx)+viewRadius*hlod)){
					float h1=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x]*(oldcamxpart);
					float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(oldcamxpart);
					float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamxpart);
					float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(1-oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(oldcamxpart);

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,y+hlod,h1);
					DrawVertexAQ(ma, x+hlod,y,h2);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					EndStripQ(ma);
					DrawVertexAQ(ma, x,y+hlod,h1);
					DrawVertexAQ(ma, x,y+lod);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x+hlod,y+lod,h4);
					EndStripQ(ma);
					DrawVertexAQ(ma, x+hlod,y+lod,h4);
					DrawVertexAQ(ma, x+lod,y+lod);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+hlod,y,h2);
					EndStripQ(ma);
				}
				if((x<=(cx)-viewRadius*hlod)){
					float h1=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamxpart);
					float h2=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y)*heightDataX+x+hlod]*(1-oldcamxpart);
					float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamxpart);
					float h4=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamxpart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamxpart);

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, x+lod,y+hlod,h1);
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x+hlod,y,h2);
					EndStripQ(ma);
					DrawVertexAQ(ma, x+lod,y+lod);
					DrawVertexAQ(ma, x+lod,y+hlod,h1);
					DrawVertexAQ(ma, x+hlod,y+lod,h4);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					EndStripQ(ma);
					DrawVertexAQ(ma, x+hlod,y,h2);
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x,y+lod);
					DrawVertexAQ(ma, x+hlod,y+lod,h4);
					EndStripQ(ma);
				}
				if((y>=(cy)+viewRadius*hlod)){
					float h1=(heightData[(y)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y)*heightDataX+x+hlod]*(oldcamypart);
					float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(oldcamypart);
					float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(oldcamypart);
					float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5f*(1-oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(oldcamypart);

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,y+hlod,h2);
					DrawVertexAQ(ma, x+hlod,y,h1);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+lod,y+hlod,h4);
					EndStripQ(ma);
					DrawVertexAQ(ma, x,y+hlod,h2);
					DrawVertexAQ(ma, x,y+lod);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x+lod,y+lod);
					DrawVertexAQ(ma, x+lod,y+hlod,h4);
					EndStripQ(ma);
				}
				if((y<=(cy)-viewRadius*hlod)){
					float h1=(heightData[(y+lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+lod)*heightDataX+x+hlod]*(1-oldcamypart);
					float h2=(heightData[(y)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x]*(1-oldcamypart);
					float h3=(heightData[(y+lod)*heightDataX+x]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+hlod]*(1-oldcamypart);
					float h4=(heightData[(y+lod)*heightDataX+x+lod]+heightData[(y)*heightDataX+x+lod])*0.5f*(oldcamypart)+heightData[(y+hlod)*heightDataX+x+lod]*(1-oldcamypart);

					if(inStrip){
						EndStripQ(ma);
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y+hlod,h2);
					DrawVertexAQ(ma, x,y+lod);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x+hlod,y+lod,h1);
					DrawVertexAQ(ma, x+lod,y+hlod,h4);
					DrawVertexAQ(ma, x+lod,y+lod);
					EndStripQ(ma);
					DrawVertexAQ(ma, x+lod,y+hlod,h4);
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+hlod,y+hlod,h3);
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,y+hlod,h2);
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
	ma->EnlargeArrays(8*nloop, 2*nloop);
	//rita yttre begr�snings yta mot n�ta lod
	if(maxlx<maxtx && maxlx>=mintx){
		x=maxlx;
		for(y=yst;y<yed;y+=lod){
			DrawVertexAQ(ma, x,y);
			DrawVertexAQ(ma, x,y+lod);
			if(y%(lod*2)){
				float h=((heightData[(y-lod)*heightDataX+x+lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f)*(1-camxpart)+heightData[(y)*heightDataX+x+lod]*(camxpart);
				DrawVertexAQ(ma, x+lod,y,h);
				DrawVertexAQ(ma, x+lod,y+lod);
			} else {
				DrawVertexAQ(ma, x+lod,y);
				float h=(heightData[(y)*heightDataX+x+lod]+heightData[(y+lod*2)*heightDataX+x+lod])*0.5f*(1-camxpart)+heightData[(y+lod)*heightDataX+x+lod]*(camxpart);
				DrawVertexAQ(ma, x+lod,y+lod,h);
			}
			EndStripQ(ma);
		}
	}
	if(minlx>mintx && minlx<maxtx){
		x=minlx-lod;
		for(y=yst;y<yed;y+=lod){
			if(y%(lod*2)){
				float h=((heightData[(y-lod)*heightDataX+x]+heightData[(y+lod)*heightDataX+x])*0.5f)*(camxpart)+heightData[(y)*heightDataX+x]*(1-camxpart);
				DrawVertexAQ(ma, x,y,h);
				DrawVertexAQ(ma, x,y+lod);
			} else {
				DrawVertexAQ(ma, x,y);
				float h=(heightData[(y)*heightDataX+x]+heightData[(y+lod*2)*heightDataX+x])*0.5f*(camxpart)+heightData[(y+lod)*heightDataX+x]*(1-camxpart);
				DrawVertexAQ(ma, x,y+lod,h);
			}
			DrawVertexAQ(ma, x+lod,y);
			DrawVertexAQ(ma, x+lod,y+lod);
			EndStripQ(ma);
		}
	}
	if(maxly<maxty && maxly>minty){
		y=maxly;
		int xs=max(xstart-lod,mintx);
		int xe=min(xend+lod,maxtx);
		if(xs<xe){
			x=xs;
			int nloop=(xe-xs)/lod+2; // one extra for if statment
			ma->EnlargeArrays(2*nloop, 1);
			if(x%(lod*2)){
				DrawVertexAQ(ma, x,y);
				float h=((heightData[(y+lod)*heightDataX+x-lod]+heightData[(y+lod)*heightDataX+x+lod])*0.5f)*(1-camypart)+heightData[(y+lod)*heightDataX+x]*(camypart);
				DrawVertexAQ(ma, x,y+lod,h);
			} else {
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,y+lod);
			}
			for(x=xs;x<xe;x+=lod){
				if(x%(lod*2)){
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+lod,y+lod);
				} else {
					DrawVertexAQ(ma, x+lod,y);
					float h=(heightData[(y+lod)*heightDataX+x+2*lod]+heightData[(y+lod)*heightDataX+x])*0.5f*(1-camypart)+heightData[(y+lod)*heightDataX+x+lod]*(camypart);
					DrawVertexAQ(ma, x+lod,y+lod,h);
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
			int nloop=(xe-xs)/lod+2; // one extra for if statment
			ma->EnlargeArrays(2*nloop, 1);
			if(x%(lod*2)){
				float h=((heightData[(y)*heightDataX+x-lod]+heightData[(y)*heightDataX+x+lod])*0.5f)*(camypart)+heightData[(y)*heightDataX+x]*(1-camypart);
				DrawVertexAQ(ma, x,y,h);
				DrawVertexAQ(ma, x,y+lod);
			} else {
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,y+lod);
			}
			for(x=xs;x<xe;x+=lod){
				if(x%(lod*2)){
					DrawVertexAQ(ma, x+lod,y);
					DrawVertexAQ(ma, x+lod,y+lod);
				} else {
					float h=(heightData[(y)*heightDataX+x+2*lod]+heightData[(y)*heightDataX+x])*0.5f*(camypart)+heightData[(y)*heightDataX+x+lod]*(1-camypart);
					DrawVertexAQ(ma, x+lod,y,h);
					DrawVertexAQ(ma, x+lod,y+lod);
				}
			}
			EndStripQ(ma);
		}
	}

	DrawGroundVertexArrayQ(ma);
}


void CBFGroundDrawer::DrawShadowPass(void)
{
	va = GetVertexArray();
	va->Initialize();

//	glEnable(GL_CULL_FACE);
	bool inStrip = false;
	const int NUM_LODS = 4;

	glPolygonOffset(1, 1);
	glEnable(GL_POLYGON_OFFSET_FILL);

	int x,y;
	float camxpart = 0.0f, oldcamxpart;
	float camypart = 0.0f, oldcamypart;

	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, groundShadowVP);
	glEnable(GL_VERTEX_PROGRAM_ARB);

#if GML_ENABLE_DRAWGROUNDSHADOW
	gmlProcessor.Work(NULL,&CBFGroundDrawer::DoDrawGroundShadowLODMT,NULL,this,gmlThreadCount,FALSE,NULL,NUM_LODS+1,50,100,TRUE,NULL);
#else
	for (int nlod = 0; nlod < NUM_LODS+1; ++nlod) {
    DoDrawGroundShadowLOD(nlod);
	}
#endif

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_VERTEX_PROGRAM_ARB);
}


void CBFGroundDrawer::SetupTextureUnits(bool drawReflection, unsigned int overrideVP)
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
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			SetTexGen(0.02f,0.02f, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f));
		} else {
			glDisable (GL_TEXTURE_2D);
		}

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, infoTex);
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
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glActiveTextureARB(GL_TEXTURE2_ARB);

		if (map->detailTex) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, map->detailTex);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			SetTexGen(0.02f, 0.02f, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f));
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


void CBFGroundDrawer::ResetTextureUnits(bool drawReflection, unsigned int overrideVP)
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
	float3 up(0, 1, 0);

	// get vector for collision between frustum and horizontal plane
	float3 b = up.cross(side);

	if (fabs(b.z) < 0.0001f)
		b.z = 0.0001f;

	{
		temp.dir = b.x / b.z;				// set direction to that
		float3 c = b.cross(side);			// get vector from camera to collision line
		float3 colpoint;					// a point on the collision line

		if (side.y > 0)
			colpoint = cam2->pos - c * ((cam2->pos.y - (readmap->minheight - 100)) / c.y);
		else
			colpoint = cam2->pos - c * ((cam2->pos.y - (readmap->maxheight +  30)) / c.y);

		// get intersection between colpoint and z axis
		temp.base = colpoint.x - colpoint.z * temp.dir;

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
	float3 up(0, 1, 0);
	float3 side = cam2->forward;
	float3 camHorizontal = cam2->forward;
	camHorizontal.y = 0;
	camHorizontal.Normalize();
	// get vector for collision between frustum and horizontal plane
	float3 b = up.cross(camHorizontal);

	if (fabs(b.z) > 0.0001f) {
		temp.dir = b.x / b.z;					// set direction to that
		float3 c = b.cross(camHorizontal);		// get vector from camera to collision line
		float3 colpoint;						// a point on the collision line

		if (side.y > 0)
			colpoint = cam2->pos + camHorizontal * gu->viewRange * 1.05f - c * (cam2->pos.y / c.y);
		else
			colpoint = cam2->pos + camHorizontal * gu->viewRange * 1.05f - c * ((cam2->pos.y - 255 / 3.5f) / c.y);

		// get intersection between colpoint and z axis
		temp.base = colpoint.x - colpoint.z * temp.dir;

		if (b.z > 0) {
			left.push_back(temp);
		} else {
			right.push_back(temp);
		}
	}

}


void CBFGroundDrawer::IncreaseDetail()
{
	viewRadius += 2;
	logOutput << "ViewRadius is now " << viewRadius << "\n";
}

void CBFGroundDrawer::DecreaseDetail()
{
	viewRadius -= 2;
	logOutput << "ViewRadius is now " << viewRadius << "\n";
}
