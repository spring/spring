/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyMeshDrawer.h"
#if 0
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "System/Config/ConfigHandler.h"
#include "System/FastMath.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"

#define CLAMP(i) Clamp((i), 0, smfReadMap->maxHeightMapIdx)

static inline float GetVisibleVertexHeight(int idx) {
	return readMap->GetCornerHeightMapUnsynced()[idx];
}


CLegacyMeshDrawer::CLegacyMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd)
	: smfReadMap(rm)
	, smfGroundDrawer(gd)
{
}



void CLegacyMeshDrawer::DrawVertexAQ(CVertexArray* ma, int x, int y)
{
	// don't send the normals as vertex attributes
	// (DLOD'ed triangles mess with interpolation)
	// const float3& n = readMap->vertexNormals[(y * smfReadMap->heightMapSizeX) + x];

	DrawVertexAQ(ma, x, y, GetVisibleVertexHeight(y * smfReadMap->heightMapSizeX + x));
}

void CLegacyMeshDrawer::DrawVertexAQ(CVertexArray* ma, int x, int y, float height)
{
	//if (waterDrawn && height < 0.0f) {
	//	height *= 2.0f;
	//}

	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}


void CLegacyMeshDrawer::DrawGroundVertexArrayQ(CVertexArray*& ma)
{
	ma->DrawArray0(GL_TRIANGLE_STRIP);
	ma = GetVertexArray();
}



bool CLegacyMeshDrawer::BigTexSquareRowVisible(const CCamera* cam, int bty) const
{
	const int minz =  bty * smfReadMap->bigTexSize;
	const int maxz = minz + smfReadMap->bigTexSize;
	const float miny = readMap->GetCurrMinHeight();
	const float maxy = math::fabs(cam->GetPos().y);

	const float3 mins(                   0, miny, minz);
	const float3 maxs(smfReadMap->mapSizeX, maxy, maxz);

	return (cam->InView(mins, maxs));
}



void CLegacyMeshDrawer::FindRange(const CCamera* cam, int& xs, int& xe, int y, int lod)
{
	int xt0, xt1;

	const CCamera::FrustumLine* negLines = cam->GetNegFrustumLines();
	const CCamera::FrustumLine* posLines = cam->GetPosFrustumLines();

	for (int idx = 0, cnt = negLines[4].sign; idx < cnt; idx++) {
		const CCamera::FrustumLine& fl = negLines[idx];
		const float xtf = fl.base + fl.dir * y;

		xt0 = (int)xtf;
		xt1 = (int)(xtf + fl.dir * lod);

		if (xt0 > xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod - lod;

		if (xt0 > xs)
			xs = xt0;
	}
	for (int idx = 0, cnt = posLines[4].sign; idx < cnt; idx++) {
		const CCamera::FrustumLine& fl = posLines[idx];
		const float xtf = fl.base + fl.dir * y;

		xt0 = (int)xtf;
		xt1 = (int)(xtf + fl.dir * lod);

		if (xt0 < xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod + lod;

		if (xt0 < xe)
			xe = xt0;
	}
}



void CLegacyMeshDrawer::DoDrawGroundRow(const CCamera* cam, int bty)
{
	if (!BigTexSquareRowVisible(cam, bty)) {
		//! skip this entire row of squares if we can't see it
		return;
	}

	CVertexArray* ma = GetVertexArray();

	bool inStrip = false;
	float x0, x1;
	int x,y;
	int sx = 0;
	int ex = smfReadMap->numBigTexX;

	//! only process the necessary big squares in the x direction
	const int bigSquareSizeY = bty * smfReadMap->bigSquareSize;

	const CCamera::FrustumLine* negLines = cam->GetNegFrustumLines();
	const CCamera::FrustumLine* posLines = cam->GetPosFrustumLines();

	for (int idx = 0, cnt = negLines[4].sign; idx < cnt; idx++) {
		const CCamera::FrustumLine& fl = negLines[idx];

		x0 = fl.base + fl.dir * bigSquareSizeY;
		x1 = x0 + fl.dir * smfReadMap->bigSquareSize;

		if (x0 > x1)
			x0 = x1;

		x0 /= smfReadMap->bigSquareSize;

		if (x0 > sx)
			sx = (int) x0;
	}
	for (int idx = 0, cnt = posLines[4].sign; idx < cnt; idx++) {
		const CCamera::FrustumLine& fl = posLines[idx];

		x0 = fl.base + fl.dir * bigSquareSizeY + smfReadMap->bigSquareSize;
		x1 = x0 + fl.dir * smfReadMap->bigSquareSize;

		if (x0 < x1)
			x0 = x1;

		x0 /= smfReadMap->bigSquareSize;

		if (x0 < ex)
			ex = (int) x0;
	}

	if (sx > ex)
		return;

	const float cx2 = cam->GetPos().x / SQUARE_SIZE;
	const float cy2 = cam->GetPos().z / SQUARE_SIZE;

	for (int btx = sx; btx < ex; ++btx) {
		ma->Initialize();

		for (int lod = 1; lod < neededLod; lod <<= 1) {
			float oldcamxpart = 0.0f;
			float oldcamypart = 0.0f;

			const int hlod = lod >> 1;
			const int dlod = lod << 1;

			int cx = cx2;
			int cy = cy2;

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

			const int ysquaremod = (cy % dlod) / lod;
			const int xsquaremod = (cx % dlod) / lod;

			const float camxpart = (cx2 - ((cx / dlod) * dlod)) / dlod;
			const float camypart = (cy2 - ((cy / dlod) * dlod)) / dlod;

			const float mcxp  = 1.0f - camxpart, mcyp  = 1.0f - camypart;
			const float hcxp  = 0.5f * camxpart, hcyp  = 0.5f * camypart;
			const float hmcxp = 0.5f * mcxp,     hmcyp = 0.5f * mcyp;

			const float mocxp  = 1.0f - oldcamxpart, mocyp  = 1.0f - oldcamypart;
			const float hocxp  = 0.5f * oldcamxpart, hocyp  = 0.5f * oldcamypart;
			const float hmocxp = 0.5f * mocxp,       hmocyp = 0.5f * mocyp;

			const int minty = bty * smfReadMap->bigSquareSize, maxty = minty + smfReadMap->bigSquareSize;
			const int mintx = btx * smfReadMap->bigSquareSize, maxtx = mintx + smfReadMap->bigSquareSize;

			const int minly = cy + (-viewRadius + 3 - ysquaremod) * lod;
			const int maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
			const int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod;
			const int maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

			const int xstart = std::max(minlx, mintx), xend = std::min(maxlx, maxtx);
			const int ystart = std::max(minly, minty), yend = std::min(maxly, maxty);

			const int vrhlod = viewRadius * hlod;

			for (y = ystart; y < yend; y += lod) {
				int xs = xstart;
				int xe = xend;

				FindRange(cam, /*inout*/ xs, /*inout*/ xe, y, lod);

				// If FindRange modifies (xs, xe) to a (less then) empty range,
				// continue to the next row.
				// If we'd continue, nloop (below) would become negative and we'd
				// allocate a vertex array with negative size.  (mantis #1415)
				if (xe < xs) continue;

				int ylod = y + lod;
				int yhlod = y + hlod;
				int nloop = (xe - xs) / lod + 1;

				ma->EnlargeArrays(52 * nloop);

				int yhdx = y * smfReadMap->heightMapSizeX;
				int ylhdx = yhdx + lod * smfReadMap->heightMapSizeX;
				int yhhdx = yhdx + hlod * smfReadMap->heightMapSizeX;

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
							float h1 = (GetVisibleVertexHeight(idx1) + GetVisibleVertexHeight(idx2)) * hmocxp + GetVisibleVertexHeight(idx3) * oldcamxpart;
							float h2 = (GetVisibleVertexHeight(idx1) + GetVisibleVertexHeight(idx1LOD)) * hmocxp + GetVisibleVertexHeight(idx1HLOD) * oldcamxpart;
							float h3 = (GetVisibleVertexHeight(idx2) + GetVisibleVertexHeight(idx1LOD)) * hmocxp + GetVisibleVertexHeight(idx3HLOD) * oldcamxpart;
							float h4 = (GetVisibleVertexHeight(idx2) + GetVisibleVertexHeight(idx2LOD)) * hmocxp + GetVisibleVertexHeight(idx2HLOD) * oldcamxpart;

							if (inStrip) {
								ma->EndStrip();
								inStrip = false;
							}

							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, yhlod, h1);
							DrawVertexAQ(ma, xhlod, y, h2);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							ma->EndStrip();
							DrawVertexAQ(ma, x, yhlod, h1);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							ma->EndStrip();
							DrawVertexAQ(ma, xhlod, ylod, h4);
							DrawVertexAQ(ma, xlod, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xhlod, y, h2);
							ma->EndStrip();
						}
						else if ((x <= cx - vrhlod)) {
							//! lower LOD to the left
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (GetVisibleVertexHeight(idx1LOD) + GetVisibleVertexHeight(idx2LOD)) * hocxp + GetVisibleVertexHeight(idx3LOD ) * mocxp;
							float h2 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx1LOD)) * hocxp + GetVisibleVertexHeight(idx1HLOD) * mocxp;
							float h3 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx1LOD)) * hocxp + GetVisibleVertexHeight(idx3HLOD) * mocxp;
							float h4 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx2LOD)) * hocxp + GetVisibleVertexHeight(idx2HLOD) * mocxp;

							if (inStrip) {
								ma->EndStrip();
								inStrip = false;
							}

							DrawVertexAQ(ma, xlod, yhlod, h1);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xhlod, y, h2);
							ma->EndStrip();
							DrawVertexAQ(ma, xlod, ylod);
							DrawVertexAQ(ma, xlod, yhlod, h1);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							ma->EndStrip();
							DrawVertexAQ(ma, xhlod, y, h2);
							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, ylod, h4);
							ma->EndStrip();
						}

						if ((y >= cy + vrhlod)) {
							//! lower LOD above
							int idx1 = yhdx + x,  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = ylhdx + x, idx2LOD = CLAMP(idx2 + lod);
							int idx3 = yhhdx + x, idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx1LOD)) * hmocyp + GetVisibleVertexHeight(idx1HLOD) * oldcamypart;
							float h2 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx2   )) * hmocyp + GetVisibleVertexHeight(idx3    ) * oldcamypart;
							float h3 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx1LOD)) * hmocyp + GetVisibleVertexHeight(idx3HLOD) * oldcamypart;
							float h4 = (GetVisibleVertexHeight(idx2LOD) + GetVisibleVertexHeight(idx1LOD)) * hmocyp + GetVisibleVertexHeight(idx3LOD ) * oldcamypart;

							if (inStrip) {
								ma->EndStrip();
								inStrip = false;
							}

							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, yhlod, h2);
							DrawVertexAQ(ma, xhlod, y, h1);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							ma->EndStrip();
							DrawVertexAQ(ma, x, yhlod, h2);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xlod, ylod);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							ma->EndStrip();
						}
						else if ((y <= cy - vrhlod)) {
							//! lower LOD beneath
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx2LOD)) * hocyp + GetVisibleVertexHeight(idx2HLOD) * mocyp;
							float h2 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx2   )) * hocyp + GetVisibleVertexHeight(idx3    ) * mocyp;
							float h3 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx1LOD)) * hocyp + GetVisibleVertexHeight(idx3HLOD) * mocyp;
							float h4 = (GetVisibleVertexHeight(idx2LOD) + GetVisibleVertexHeight(idx1LOD)) * hocyp + GetVisibleVertexHeight(idx3LOD ) * mocyp;

							if (inStrip) {
								ma->EndStrip();
								inStrip = false;
							}

							DrawVertexAQ(ma, x, yhlod, h2);
							DrawVertexAQ(ma, x, ylod);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, xhlod, ylod, h1);
							DrawVertexAQ(ma, xlod, yhlod, h4);
							DrawVertexAQ(ma, xlod, ylod);
							ma->EndStrip();
							DrawVertexAQ(ma, xlod, yhlod, h4);
							DrawVertexAQ(ma, xlod, y);
							DrawVertexAQ(ma, xhlod, yhlod, h3);
							DrawVertexAQ(ma, x, y);
							DrawVertexAQ(ma, x, yhlod, h2);
							ma->EndStrip();
						}
					}
				}

				if (inStrip) {
					ma->EndStrip();
					inStrip = false;
				}
			} //for (y = ystart; y < yend; y += lod)

			const int yst = std::max(ystart - lod, minty);
			const int yed = std::min(yend + lod, maxty);
			int nloop = (yed - yst) / lod + 1;

			if (nloop > 0)
				ma->EnlargeArrays(8 * nloop);

			//! rita yttre begr?snings yta mot n?ta lod
			if (maxlx < maxtx && maxlx >= mintx) {
				x = maxlx;
				int xlod = x + lod;
				for (y = yst; y < yed; y += lod) {
					DrawVertexAQ(ma, x, y);
					DrawVertexAQ(ma, x, y + lod);

					if (y % dlod) {
						const int idx1 = CLAMP((y      ) * smfReadMap->heightMapSizeX + x), idx1LOD = CLAMP(idx1 + lod);
						const int idx2 = CLAMP((y + lod) * smfReadMap->heightMapSizeX + x), idx2LOD = CLAMP(idx2 + lod);
						const int idx3 = CLAMP((y - lod) * smfReadMap->heightMapSizeX + x), idx3LOD = CLAMP(idx3 + lod);
						const float h = (GetVisibleVertexHeight(idx3LOD) + GetVisibleVertexHeight(idx2LOD)) * hmcxp +	GetVisibleVertexHeight(idx1LOD) * camxpart;
						DrawVertexAQ(ma, xlod, y, h);
						DrawVertexAQ(ma, xlod, y + lod);
					} else {
						const int idx1 = CLAMP((y       ) * smfReadMap->heightMapSizeX + x), idx1LOD = CLAMP(idx1 + lod);
						const int idx2 = CLAMP((y +  lod) * smfReadMap->heightMapSizeX + x), idx2LOD = CLAMP(idx2 + lod);
						const int idx3 = CLAMP((y + dlod) * smfReadMap->heightMapSizeX + x), idx3LOD = CLAMP(idx3 + lod);
						const float h = (GetVisibleVertexHeight(idx1LOD) + GetVisibleVertexHeight(idx3LOD)) * hmcxp + GetVisibleVertexHeight(idx2LOD) * camxpart;
						DrawVertexAQ(ma, xlod, y);
						DrawVertexAQ(ma, xlod, y + lod, h);
					}
					ma->EndStrip();
				}
			}

			if (minlx > mintx && minlx < maxtx) {
				x = minlx - lod;
				int xlod = x + lod;
				for (y = yst; y < yed; y += lod) {
					if (y % dlod) {
						int idx1 = CLAMP((y      ) * smfReadMap->heightMapSizeX + x);
						int idx2 = CLAMP((y + lod) * smfReadMap->heightMapSizeX + x);
						int idx3 = CLAMP((y - lod) * smfReadMap->heightMapSizeX + x);
						float h = (GetVisibleVertexHeight(idx3) + GetVisibleVertexHeight(idx2)) * hcxp + GetVisibleVertexHeight(idx1) * mcxp;
						DrawVertexAQ(ma, x, y, h);
						DrawVertexAQ(ma, x, y + lod);
					} else {
						int idx1 = CLAMP((y       ) * smfReadMap->heightMapSizeX + x);
						int idx2 = CLAMP((y +  lod) * smfReadMap->heightMapSizeX + x);
						int idx3 = CLAMP((y + dlod) * smfReadMap->heightMapSizeX + x);
						float h = (GetVisibleVertexHeight(idx1) + GetVisibleVertexHeight(idx3)) * hcxp + GetVisibleVertexHeight(idx2) * mcxp;
						DrawVertexAQ(ma, x, y);
						DrawVertexAQ(ma, x, y + lod, h);
					}
					DrawVertexAQ(ma, xlod, y);
					DrawVertexAQ(ma, xlod, y + lod);
					ma->EndStrip();
				}
			}

			if (maxly < maxty && maxly > minty) {
				y = maxly;
				int xs = std::max(xstart - lod, mintx);
				int xe = std::min(xend + lod,   maxtx);
				FindRange(cam, xs, xe, y, lod);

				if (xs < xe) {
					x = xs;
					int ylod = y + lod;
					int nloop = (xe - xs) / lod + 2; //! one extra for if statment
					int ylhdx = (y + lod) * smfReadMap->heightMapSizeX;

					ma->EnlargeArrays(2 * nloop);

					if (x % dlod) {
						int idx2 = CLAMP(ylhdx + x), idx2PLOD = CLAMP(idx2 + lod), idx2MLOD = CLAMP(idx2 - lod);
						float h = (GetVisibleVertexHeight(idx2MLOD) + GetVisibleVertexHeight(idx2PLOD)) * hmcyp + GetVisibleVertexHeight(idx2) * camypart;
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
							float h = (GetVisibleVertexHeight(idx2PLOD2) + GetVisibleVertexHeight(idx2)) * hmcyp + GetVisibleVertexHeight(idx2PLOD) * camypart;
							DrawVertexAQ(ma, x + lod, y);
							DrawVertexAQ(ma, x + lod, ylod, h);
						}
					}
					ma->EndStrip();
				}
			}

			if (minly > minty && minly < maxty) {
				y = minly - lod;
				int xs = std::max(xstart - lod, mintx);
				int xe = std::min(xend + lod,   maxtx);
				FindRange(cam, xs, xe, y, lod);

				if (xs < xe) {
					x = xs;
					int ylod = y + lod;
					int yhdx = y * smfReadMap->heightMapSizeX;
					int nloop = (xe - xs) / lod + 2; //! one extra for if statment

					ma->EnlargeArrays(2 * nloop);

					if (x % dlod) {
						int idx1 = CLAMP(yhdx + x), idx1PLOD = CLAMP(idx1 + lod), idx1MLOD = CLAMP(idx1 - lod);
						float h = (GetVisibleVertexHeight(idx1MLOD) + GetVisibleVertexHeight(idx1PLOD)) * hcyp + GetVisibleVertexHeight(idx1) * mcyp;
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
							float h = (GetVisibleVertexHeight(idx1PLOD2) + GetVisibleVertexHeight(idx1)) * hcyp + GetVisibleVertexHeight(idx1PLOD) * mcyp;
							DrawVertexAQ(ma, x + lod, y, h);
							DrawVertexAQ(ma, x + lod, ylod);
						}
					}
					ma->EndStrip();
				}
			}

		} //for (int lod = 1; lod < neededLod; lod <<= 1)

		smfGroundDrawer->SetupBigSquare(btx, bty);
		DrawGroundVertexArrayQ(ma);
	}
}


void CLegacyMeshDrawer::UpdateLODParams(const DrawPass::e& drawPass)
{
	// Get current ground detail (draw pass dependent)
	viewRadius  = smfGroundDrawer->GetGroundDetail(drawPass);

	// Take FOV into account
	viewRadius  = int(viewRadius * fastmath::apxsqrt(45.0f / camera->GetVFOV()));

	// Clamp it to mapsize dependent minimum, else we get holes in the terrain
	viewRadius  = std::max(std::max(smfReadMap->numBigTexY, smfReadMap->numBigTexX) + 1, viewRadius);

	// we need a multiple of 2
	viewRadius += (viewRadius & 1);

	// Compute count of LODs needed/visible
	neededLod   = std::max(1, int((camera->GetFarPlaneDist() * 0.125f) / viewRadius) << 1);
	neededLod   = std::min(neededLod, std::min(mapDims.mapx, mapDims.mapy));
}


void CLegacyMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	if (drawPass == DrawPass::Shadow) {
		DrawShadowMesh();
		return;
	}

	UpdateLODParams(drawPass);

	CCamera* cam = CCameraHandler::GetActiveCamera();
	cam->CalcFrustumLines(readMap->GetCurrMinHeight() - 100.0f, readMap->GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);

	const int camBigTexY = Clamp(int(cam->GetPos().z / (smfReadMap->bigSquareSize * SQUARE_SIZE)), 0, smfReadMap->numBigTexY - 1);

	// try to render "front to back" (so start with the bigtex rows closest to camera)
	for (int bty = camBigTexY; bty >= 0; --bty) {
		DoDrawGroundRow(cam, bty);
	}
	for (int bty = camBigTexY + 1; bty < smfReadMap->numBigTexY; ++bty) {
		DoDrawGroundRow(cam, bty);
	}
}


void CLegacyMeshDrawer::DoDrawGroundShadowLOD(int nlod) {
	CVertexArray* ma = GetVertexArray();
	ma->Initialize();

	bool inStrip = false;
	int x,y;
	int lod = 1 << nlod;

	float cx2 = camera->GetPos().x / SQUARE_SIZE;
	float cy2 = camera->GetPos().z / SQUARE_SIZE;

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
	const int ysquaremod = (cy % dlod) / lod;
	const int xsquaremod = (cx % dlod) / lod;

	const float camxpart = (cx2 - (cx / dlod) * dlod) / dlod;
	const float camypart = (cy2 - (cy / dlod) * dlod) / dlod;

	const int minty = 0, maxty = mapDims.mapy;
	const int mintx = 0, maxtx = mapDims.mapx;

	const int minly = cy + (-viewRadius + 3 - ysquaremod) * lod, maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
	const int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod, maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

	const int xstart = std::max(minlx, mintx), xend   = std::min(maxlx, maxtx);
	const int ystart = std::max(minly, minty), yend   = std::min(maxly, maxty);

	const int lhdx = lod * smfReadMap->heightMapSizeX;
	const int hhdx = hlod * smfReadMap->heightMapSizeX;
	const int dhdx = dlod * smfReadMap->heightMapSizeX;

	const float mcxp  = 1.0f - camxpart, mcyp  = 1.0f - camypart;
	const float hcxp  = 0.5f * camxpart, hcyp  = 0.5f * camypart;
	const float hmcxp = 0.5f * mcxp,     hmcyp = 0.5f * mcyp;

	const float mocxp  = 1.0f - oldcamxpart, mocyp  = 1.0f - oldcamypart;
	const float hocxp  = 0.5f * oldcamxpart, hocyp  = 0.5f * oldcamypart;
	const float hmocxp = 0.5f * mocxp,       hmocyp = 0.5f * mocyp;

	const int vrhlod = viewRadius * hlod;

	for (y = ystart; y < yend; y += lod) {
		int xs = xstart;
		int xe = xend;

		if (xe < xs) continue;

		int ylod = y + lod;
		int yhlod = y + hlod;
		int ydx = y * smfReadMap->heightMapSizeX;
		int nloop = (xe - xs) / lod + 1;

		ma->EnlargeArrays(52 * nloop);

		for (x = xs; x < xe; x += lod) {
			int xlod = x + lod;
			int xhlod = x + hlod;
			if ((lod == 1) ||
				(x > cx + vrhlod) || (x < cx - vrhlod) ||
				(y > cy + vrhlod) || (y < cy - vrhlod)) {
					if (!inStrip) {
						DrawVertexAQ(ma, x, y   );
						DrawVertexAQ(ma, x, ylod);
						inStrip = true;
					}
					DrawVertexAQ(ma, xlod, y   );
					DrawVertexAQ(ma, xlod, ylod);
			}
			else {  //! inre begr?sning mot f?eg?nde lod
				int yhdx=ydx+x;
				int ylhdx=yhdx+lhdx;
				int yhhdx=yhdx+hhdx;

				if ( x>= cx + vrhlod) {
					const float h1 = (GetVisibleVertexHeight(yhdx ) + GetVisibleVertexHeight(ylhdx    )) * hmocxp + GetVisibleVertexHeight(yhhdx     ) * oldcamxpart;
					const float h2 = (GetVisibleVertexHeight(yhdx ) + GetVisibleVertexHeight(yhdx+lod )) * hmocxp + GetVisibleVertexHeight(yhdx+hlod ) * oldcamxpart;
					const float h3 = (GetVisibleVertexHeight(ylhdx) + GetVisibleVertexHeight(yhdx+lod )) * hmocxp + GetVisibleVertexHeight(yhhdx+hlod) * oldcamxpart;
					const float h4 = (GetVisibleVertexHeight(ylhdx) + GetVisibleVertexHeight(ylhdx+lod)) * hmocxp + GetVisibleVertexHeight(ylhdx+hlod) * oldcamxpart;

					if(inStrip){
						ma->EndStrip();
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,yhlod,h1);
					DrawVertexAQ(ma, xhlod,y,h2);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					ma->EndStrip();
					DrawVertexAQ(ma, x,yhlod,h1);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					ma->EndStrip();
					DrawVertexAQ(ma, xhlod,ylod,h4);
					DrawVertexAQ(ma, xlod,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xhlod,y,h2);
					ma->EndStrip();
				}
				if (x <= cx - vrhlod) {
					const float h1 = (GetVisibleVertexHeight(yhdx+lod) + GetVisibleVertexHeight(ylhdx+lod)) * hocxp + GetVisibleVertexHeight(yhhdx+lod ) * mocxp;
					const float h2 = (GetVisibleVertexHeight(yhdx    ) + GetVisibleVertexHeight(yhdx+lod )) * hocxp + GetVisibleVertexHeight(yhdx+hlod ) * mocxp;
					const float h3 = (GetVisibleVertexHeight(ylhdx   ) + GetVisibleVertexHeight(yhdx+lod )) * hocxp + GetVisibleVertexHeight(yhhdx+hlod) * mocxp;
					const float h4 = (GetVisibleVertexHeight(ylhdx   ) + GetVisibleVertexHeight(ylhdx+lod)) * hocxp + GetVisibleVertexHeight(ylhdx+hlod) * mocxp;

					if(inStrip){
						ma->EndStrip();
						inStrip=false;
					}
					DrawVertexAQ(ma, xlod,yhlod,h1);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xhlod,y,h2);
					ma->EndStrip();
					DrawVertexAQ(ma, xlod,ylod);
					DrawVertexAQ(ma, xlod,yhlod,h1);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					ma->EndStrip();
					DrawVertexAQ(ma, xhlod,y,h2);
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,ylod,h4);
					ma->EndStrip();
				}
				if (y >= cy + vrhlod) {
					const float h1 = (GetVisibleVertexHeight(yhdx     ) + GetVisibleVertexHeight(yhdx+lod)) * hmocyp + GetVisibleVertexHeight(yhdx+hlod ) * oldcamypart;
					const float h2 = (GetVisibleVertexHeight(yhdx     ) + GetVisibleVertexHeight(ylhdx   )) * hmocyp + GetVisibleVertexHeight(yhhdx     ) * oldcamypart;
					const float h3 = (GetVisibleVertexHeight(ylhdx    ) + GetVisibleVertexHeight(yhdx+lod)) * hmocyp + GetVisibleVertexHeight(yhhdx+hlod) * oldcamypart;
					const float h4 = (GetVisibleVertexHeight(ylhdx+lod) + GetVisibleVertexHeight(yhdx+lod)) * hmocyp + GetVisibleVertexHeight(yhhdx+lod ) * oldcamypart;

					if(inStrip){
						ma->EndStrip();
						inStrip=false;
					}
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,yhlod,h2);
					DrawVertexAQ(ma, xhlod,y,h1);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					ma->EndStrip();
					DrawVertexAQ(ma, x,yhlod,h2);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xlod,ylod);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					ma->EndStrip();
				}
				if (y <= cy - vrhlod) {
					const float h1 = (GetVisibleVertexHeight(ylhdx    ) + GetVisibleVertexHeight(ylhdx+lod)) * hocyp + GetVisibleVertexHeight(ylhdx+hlod) * mocyp;
					const float h2 = (GetVisibleVertexHeight(yhdx     ) + GetVisibleVertexHeight(ylhdx    )) * hocyp + GetVisibleVertexHeight(yhhdx     ) * mocyp;
					const float h3 = (GetVisibleVertexHeight(ylhdx    ) + GetVisibleVertexHeight(yhdx+lod )) * hocyp + GetVisibleVertexHeight(yhhdx+hlod) * mocyp;
					const float h4 = (GetVisibleVertexHeight(ylhdx+lod) + GetVisibleVertexHeight(yhdx+lod )) * hocyp + GetVisibleVertexHeight(yhhdx+lod ) * mocyp;

					if (inStrip) {
						ma->EndStrip();
						inStrip = false;
					}
					DrawVertexAQ(ma, x,yhlod,h2);
					DrawVertexAQ(ma, x,ylod);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, xhlod,ylod,h1);
					DrawVertexAQ(ma, xlod,yhlod,h4);
					DrawVertexAQ(ma, xlod,ylod);
					ma->EndStrip();
					DrawVertexAQ(ma, xlod,yhlod,h4);
					DrawVertexAQ(ma, xlod,y);
					DrawVertexAQ(ma, xhlod,yhlod,h3);
					DrawVertexAQ(ma, x,y);
					DrawVertexAQ(ma, x,yhlod,h2);
					ma->EndStrip();
				}
			}
		}
		if (inStrip) {
			ma->EndStrip();
			inStrip=false;
		}
	}

	int yst = std::max(ystart - lod, minty);
	int yed = std::min(yend + lod, maxty);
	int nloop = (yed - yst) / lod + 1;
	ma->EnlargeArrays(8 * nloop);

	//!rita yttre begr?snings yta mot n?ta lod
	if (maxlx < maxtx && maxlx >= mintx) {
		x = maxlx;
		const int xlod = x + lod;

		for (y = yst; y < yed; y += lod) {
			DrawVertexAQ(ma, x, y      );
			DrawVertexAQ(ma, x, y + lod);
			const int yhdx = y * smfReadMap->heightMapSizeX + x;

			if (y % dlod) {
				const float h = (GetVisibleVertexHeight(yhdx - lhdx + lod) + GetVisibleVertexHeight(yhdx + lhdx + lod)) * hmcxp + GetVisibleVertexHeight(yhdx+lod) * camxpart;
				DrawVertexAQ(ma, xlod, y, h);
				DrawVertexAQ(ma, xlod, y + lod);
			} else {
				const float h = (GetVisibleVertexHeight(yhdx+lod) + GetVisibleVertexHeight(yhdx+dhdx+lod)) * hmcxp + GetVisibleVertexHeight(yhdx+lhdx+lod) * camxpart;
				DrawVertexAQ(ma, xlod,y);
				DrawVertexAQ(ma, xlod,y+lod,h);
			}
			ma->EndStrip();
		}
	}

	if (minlx > mintx && minlx < maxtx) {
		x = minlx - lod;
		const int xlod = x + lod;

		for(y = yst; y < yed; y += lod) {
			int yhdx = y * smfReadMap->heightMapSizeX + x;
			if(y%dlod){
				const float h = (GetVisibleVertexHeight(yhdx-lhdx) + GetVisibleVertexHeight(yhdx+lhdx)) * hcxp + GetVisibleVertexHeight(yhdx) * mcxp;
				DrawVertexAQ(ma, x,y,h);
				DrawVertexAQ(ma, x,y+lod);
			} else {
				const float h = (GetVisibleVertexHeight(yhdx) + GetVisibleVertexHeight(yhdx+dhdx)) * hcxp + GetVisibleVertexHeight(yhdx+lhdx) * mcxp;
				DrawVertexAQ(ma, x,y);
				DrawVertexAQ(ma, x,y+lod,h);
			}
			DrawVertexAQ(ma, xlod,y);
			DrawVertexAQ(ma, xlod,y+lod);
			ma->EndStrip();
		}
	}
	if (maxly < maxty && maxly > minty) {
		y = maxly;
		const int xs = std::max(xstart -lod, mintx);
		const int xe = std::min(xend + lod, maxtx);

		if (xs < xe) {
			x = xs;
			const int ylod = y + lod;
			const int ydx = y * smfReadMap->heightMapSizeX;
			const int nloop = (xe - xs) / lod + 2; //! two extra for if statment

			ma->EnlargeArrays(2 * nloop);

			if (x % dlod) {
				const int ylhdx = ydx + x + lhdx;
				const float h = (GetVisibleVertexHeight(ylhdx-lod) + GetVisibleVertexHeight(ylhdx+lod)) * hmcyp + GetVisibleVertexHeight(ylhdx) * camypart;
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
					DrawVertexAQ(ma, x+lod,y);
					const int ylhdx = ydx + x + lhdx;
					const float h = (GetVisibleVertexHeight(ylhdx+dlod) + GetVisibleVertexHeight(ylhdx)) * hmcyp + GetVisibleVertexHeight(ylhdx+lod) * camypart;
					DrawVertexAQ(ma, x+lod,ylod,h);
				}
			}
			ma->EndStrip();
		}
	}
	if (minly > minty && minly < maxty) {
		y = minly - lod;
		const int xs = std::max(xstart - lod, mintx);
		const int xe = std::min(xend + lod, maxtx);

		if (xs < xe) {
			x = xs;
			const int ylod = y + lod;
			const int ydx = y * smfReadMap->heightMapSizeX;
			const int nloop = (xe - xs) / lod + 2; //! two extra for if statment

			ma->EnlargeArrays(2 * nloop);

			if (x % dlod) {
				const int yhdx = ydx + x;
				const float h = (GetVisibleVertexHeight(yhdx-lod) + GetVisibleVertexHeight(yhdx + lod)) * hcyp + GetVisibleVertexHeight(yhdx) * mcyp;
				DrawVertexAQ(ma, x, y, h);
				DrawVertexAQ(ma, x, ylod);
			} else {
				DrawVertexAQ(ma, x, y);
				DrawVertexAQ(ma, x, ylod);
			}

			for (x = xs; x < xe; x += lod) {
				if (x % dlod) {
					DrawVertexAQ(ma, x + lod, y);
					DrawVertexAQ(ma, x + lod, ylod);
				} else {
					const int yhdx = ydx + x;
					const float h = (GetVisibleVertexHeight(yhdx+dlod) + GetVisibleVertexHeight(yhdx)) * hcyp + GetVisibleVertexHeight(yhdx+lod) * mcyp;
					DrawVertexAQ(ma, x + lod, y, h);
					DrawVertexAQ(ma, x + lod, ylod);
				}
			}
			ma->EndStrip();
		}
	}
	DrawGroundVertexArrayQ(ma);
}


void CLegacyMeshDrawer::DrawShadowMesh()
{
	{ // profiler scope
		const int NUM_LODS = 4;
		{
			for (int nlod = 0; nlod < NUM_LODS + 1; ++nlod) {
				DoDrawGroundShadowLOD(nlod);
			}
		}
	}
}

#else

CLegacyMeshDrawer::CLegacyMeshDrawer(CSMFReadMap* rm, CSMFGroundDrawer* gd): smfReadMap(rm), smfGroundDrawer(gd) {}

void CLegacyMeshDrawer::DrawMesh(const DrawPass::e& drawPass) {}
void CLegacyMeshDrawer::DrawShadowMesh() {}

#endif

