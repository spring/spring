/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "SMFReadMap.h"
#include "SMFGroundTextures.h"
#include "SMFGroundDrawer.h"
#include "mapfile.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Game/LoadScreen.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/BaseSky.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/bitops.h"
#include "System/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/OpenMP_cond.h"
#include "System/LogOutput.h"
#include "System/mmgr.h"
#include "System/myMath.h"
#include "System/Util.h"

#define SSMF_UNCOMPRESSED_NORMALS 0

using namespace std;

CR_BIND_DERIVED(CSmfReadMap, CReadMap, (""))


CSmfReadMap::CSmfReadMap(std::string mapname): file(mapname)
{
	loadscreen->SetLoadMessage("Loading SMF");

	ConfigureAnisotropy();

	for (int a = 0; a < 1024; ++a) {
		for (int b = 0; b < 3; ++b) {
			float c = max(mapInfo->water.minColor[b], mapInfo->water.baseColor[b] - mapInfo->water.absorb[b] * a);
			waterHeightColors[a * 4 + b] = (unsigned char)(c * 210);
		}
		waterHeightColors[a * 4 + 3] = 1;
	}

	const SMFHeader& header = file.GetHeader();

	width  = header.mapx;
	height = header.mapy;

	cornerHeightMapSynced.resize((width + 1) * (height + 1));
	cornerHeightMapUnsynced.resize((width + 1) * (height + 1));
	groundDrawer = NULL;

	const CMapInfo::smf_t& smf = mapInfo->smf;
	const float minH = smf.minHeightOverride ? smf.minHeight : header.minHeight;
	const float maxH = smf.maxHeightOverride ? smf.maxHeight : header.maxHeight;

	const float base = minH;
	const float mod = (maxH - minH) / 65536.0f;

	file.ReadHeightmap(&cornerHeightMapSynced[0], base, mod);

	CReadMap::Initialize();

	shadingTexPixelRow.resize(gs->mapxp1 * 4, 0);
	shadingTexUpdateIter = 0;
	shadingTexUpdateRate = std::max(1.0f, math::ceil(gs->mapxp1 / float(gs->mapyp1)));
	// with GLSL, the shading texture has very limited use (minimap etc) so we increase the update interval
	if (globalRendering->haveGLSL)
		shadingTexUpdateRate *= 10;


	for (unsigned int a = 0; a < mapname.size(); ++a) {
		mapChecksum += mapname[a];
		mapChecksum *= mapname[a];
	}



	haveSpecularLighting = !(mapInfo->smf.specularTexName.empty());
	haveSplatTexture = (!mapInfo->smf.splatDetailTexName.empty() && !mapInfo->smf.splatDistrTexName.empty());

	CBitmap detailTexBM;
	CBitmap grassShadingTexBM;

	detailTex        = 0;
	shadingTex       = 0;
	normalsTex       = 0;
	minimapTex       = 0;
	specularTex      = 0;
	splatDetailTex   = 0;
	splatDistrTex    = 0;
	skyReflectModTex = 0;
	detailNormalTex  = 0;
	lightEmissionTex = 0;

	if (haveSpecularLighting) {
		CBitmap specularTexBM;
		CBitmap skyReflectModTexBM;
		CBitmap detailNormalTexBM;
		CBitmap lightEmissionTexBM;

		if (!specularTexBM.Load(mapInfo->smf.specularTexName)) {
			// maps wants specular lighting, but no moderation
			specularTexBM.channels = 4;
			specularTexBM.Alloc(1, 1);
			specularTexBM.mem[0] = 255;
			specularTexBM.mem[1] = 255;
			specularTexBM.mem[2] = 255;
			specularTexBM.mem[3] = 255;
		}

		specularTex = specularTexBM.CreateTexture(false);

		if (haveSplatTexture) {
			CBitmap splatDistrTexBM;
			CBitmap splatDetailTexBM;
			// if the map supplies an intensity- and a distribution-texture for
			// detail-splat blending, the regular detail-texture is not used
			if (!splatDetailTexBM.Load(mapInfo->smf.splatDetailTexName)) {
				// default detail-texture should be all-grey
				splatDetailTexBM.channels = 4;
				splatDetailTexBM.Alloc(1, 1);
				splatDetailTexBM.mem[0] = 127;
				splatDetailTexBM.mem[1] = 127;
				splatDetailTexBM.mem[2] = 127;
				splatDetailTexBM.mem[3] = 127;
			}

			if (!splatDistrTexBM.Load(mapInfo->smf.splatDistrTexName)) {
				splatDistrTexBM.channels = 4;
				splatDistrTexBM.Alloc(1, 1);
				splatDistrTexBM.mem[0] = 255;
				splatDistrTexBM.mem[1] = 0;
				splatDistrTexBM.mem[2] = 0;
				splatDistrTexBM.mem[3] = 0;
			}

			splatDetailTex = splatDetailTexBM.CreateTexture(true);
			splatDistrTex = splatDistrTexBM.CreateTexture(true);
		}

		// no default 1x1 textures for these
		if (skyReflectModTexBM.Load(mapInfo->smf.skyReflectModTexName)) {
			skyReflectModTex = skyReflectModTexBM.CreateTexture(false);
		}

		if (detailNormalTexBM.Load(mapInfo->smf.detailNormalTexName)) {
			detailNormalTex = detailNormalTexBM.CreateTexture(false);
		}

		if (lightEmissionTexBM.Load(mapInfo->smf.lightEmissionTexName)) {
			lightEmissionTex = lightEmissionTexBM.CreateTexture(false);
		}
	}

	if (!detailTexBM.Load(mapInfo->smf.detailTexName)) {
		throw content_error("Could not load detail texture from file " + mapInfo->smf.detailTexName);
	}

	{
		glGenTextures(1, &detailTex);
		glBindTexture(GL_TEXTURE_2D, detailTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		if (anisotropy != 0.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}
		glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, detailTexBM.xsize, detailTexBM.ysize, GL_RGBA, GL_UNSIGNED_BYTE, detailTexBM.mem);
	}


	{
		// the minimap is a static texture
		std::vector<unsigned char> minimapTexBuf(MINIMAP_SIZE, 0);
		file.ReadMinimap(&minimapTexBuf[0]);

		glGenTextures(1, &minimapTex);
		glBindTexture(GL_TEXTURE_2D, minimapTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MINIMAP_NUM_MIPMAP - 1);
		int offset = 0;
		for (unsigned int i = 0; i < MINIMAP_NUM_MIPMAP; i++) {
			const int mipsize = 1024 >> i;
			const int size = ((mipsize + 3) / 4) * ((mipsize + 3) / 4) * 8;
			glCompressedTexImage2DARB(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipsize, mipsize, 0, size, &minimapTexBuf[0] + offset);
			offset += size;
		}
	}

	{
		if (grassShadingTexBM.Load(mapInfo->smf.grassShadingTexName)) {
			// generate mipmaps for the grass shading-texture
			grassShadingTex = grassShadingTexBM.CreateTexture(true);
		} else {
			grassShadingTex = minimapTex;
		}
	}


	{
		// the shading/normal texture buffers must have PO2 dimensions
		// (excess elements that no vertices map into are left unused)
		glGenTextures(1, &shadingTex);
		glBindTexture(GL_TEXTURE_2D, shadingTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (anisotropy != 0.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	{
		#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		std::vector<float> normalsTexBuf(gs->pwr2mapx * gs->pwr2mapy * 4, 0.0f);
		#else
		GLenum texFormat = GL_LUMINANCE_ALPHA16F_ARB;

		if (!!configHandler->Get("GroundNormalTextureHighPrecision", 0)) {
			texFormat = GL_LUMINANCE_ALPHA32F_ARB;
		}
		#endif

		glGenTextures(1, &normalsTex);
		glBindTexture(GL_TEXTURE_2D, normalsTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, gs->pwr2mapx, gs->pwr2mapy, 0, GL_RGBA, GL_FLOAT, &normalsTexBuf[0]);
		#else
		glTexImage2D(GL_TEXTURE_2D, 0, texFormat, gs->pwr2mapx, gs->pwr2mapy, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL);
		#endif
	}

	file.ReadFeatureInfo();
}


CSmfReadMap::~CSmfReadMap()
{
	delete groundDrawer;

	cornerHeightMapSynced.clear();
	cornerHeightMapUnsynced.clear();

	if (detailTex        != 0) { glDeleteTextures(1, &detailTex       ); }
	if (specularTex      != 0) { glDeleteTextures(1, &specularTex     ); }
	if (minimapTex       != 0) { glDeleteTextures(1, &minimapTex      ); }
	if (shadingTex       != 0) { glDeleteTextures(1, &shadingTex      ); }
	if (normalsTex       != 0) { glDeleteTextures(1, &normalsTex      ); }
	if (splatDetailTex   != 0) { glDeleteTextures(1, &splatDetailTex  ); }
	if (splatDistrTex    != 0) { glDeleteTextures(1, &splatDistrTex   ); }
	if (grassShadingTex  != 0) { glDeleteTextures(1, &grassShadingTex ); }
	if (skyReflectModTex != 0) { glDeleteTextures(1, &skyReflectModTex); }
	if (detailNormalTex  != 0) { glDeleteTextures(1, &detailNormalTex ); }
	if (lightEmissionTex != 0) { glDeleteTextures(1, &lightEmissionTex); }
}


void CSmfReadMap::NewGroundDrawer() { groundDrawer = new CBFGroundDrawer(this); }
CBaseGroundDrawer* CSmfReadMap::GetGroundDrawer() { return (CBaseGroundDrawer*) groundDrawer; }


void CSmfReadMap::UpdateShadingTexPart(int y, int x1, int y1, int xsize, unsigned char* pixelRow) {
	for (int x = 0; x < xsize; ++x) {
		const int xi = x1 + x;
		const int yi = y1 + y;

		const float* heightMap =
			#ifdef USE_UNSYNCED_HEIGHTMAP
		    &cornerHeightMapUnsynced[0];
			#else
			&cornerHeightMapSynced[0];
			#endif
		const float height = heightMap[xi + yi * gs->mapxp1];

		if (height < 0.0f) {
			const int h = (int) - height & 1023; //! waterHeightColors array just holds 1024 colors
			float light = std::min((DiffuseSunCoeff(x + x1, y + y1) + 0.2f) * 2.0f, 1.0f);

			if (height > -10.0f) {
				const float wc = -height * 0.1f;
				const float3 light3 = GetLightValue(x + x1, y + y1) * (1.0f - wc) * 210.0f;
				light *= wc;

				pixelRow[x * 4 + 0] = (unsigned char) (waterHeightColors[h * 4 + 0] * light + light3.x);
				pixelRow[x * 4 + 1] = (unsigned char) (waterHeightColors[h * 4 + 1] * light + light3.y);
				pixelRow[x * 4 + 2] = (unsigned char) (waterHeightColors[h * 4 + 2] * light + light3.z);
			} else {
				pixelRow[x * 4 + 0] = (unsigned char) (waterHeightColors[h * 4 + 0] * light);
				pixelRow[x * 4 + 1] = (unsigned char) (waterHeightColors[h * 4 + 1] * light);
				pixelRow[x * 4 + 2] = (unsigned char) (waterHeightColors[h * 4 + 2] * light);
			}
			pixelRow[x * 4 + 3] = EncodeHeight(height);
		} else {
			const float3& light = GetLightValue(x + x1, y + y1) * 210.0f;

			pixelRow[x * 4 + 0] = (unsigned char) light.x;
			pixelRow[x * 4 + 1] = (unsigned char) light.y;
			pixelRow[x * 4 + 2] = (unsigned char) light.z;
			pixelRow[x * 4 + 3] = 255;
		}
	}
}

void CSmfReadMap::UpdateHeightMapUnsynced(const HeightMapUpdate& update)
{
	const int x1 = update.x1, y1 = update.y1;
	const int x2 = update.x2, y2 = update.y2;

	{
		// update the visible heights and normals
		static const float*  shm = &cornerHeightMapSynced[0];
		static       float*  uhm = &cornerHeightMapUnsynced[0];
		static const float3* sfn = &faceNormalsSynced[0];
		static       float3* ufn = &faceNormalsUnsynced[0];
		static const float3* scn = &centerNormalsSynced[0];
		static       float3* ucn = &centerNormalsUnsynced[0];
		static       float3* rvn = &rawVertexNormals[0];
		static       float3* vvn = &visVertexNormals[0];

		static const int W = gs->mapxp1;
		static const int H = gs->mapyp1;
		static const int SS = SQUARE_SIZE;

		// a heightmap update over (x1, y1) - (x2, y2) implies the
		// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)
		const int minx = std::max((x1 - 1),     0);
		const int minz = std::max((y1 - 1),     0);
		const int maxx = std::min((x2 + 1), W - 1);
		const int maxz = std::min((y2 + 1), H - 1);

		const int xsize = maxx - minx;
		const int zsize = maxz - minz;

		#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		std::vector<float> pixels((xsize + 1) * (zsize + 1) * 4, 0.0f);
		#else
		std::vector<float> pixels((xsize + 1) * (zsize + 1) * 2, 0.0f);
		#endif

		int z;
		#pragma omp parallel for private(z)
		for (z = minz; z <= maxz; z++) {
			for (int x = minx; x <= maxx; x++) {
				const int vIdxTL = (z    ) * W + x;
				const int vIdxBL = (z + 1) * W + x;
				const int fIdxTL = (z * (W - 1) + x) * 2    ;
				const int fIdxBR = (z * (W - 1) + x) * 2 + 1;

				const bool hasNgbL = (x >     0); const int xOffL = hasNgbL? 1: 0;
				const bool hasNgbR = (x < W - 1); const int xOffR = hasNgbR? 1: 0;
				const bool hasNgbT = (z >     0); const int zOffT = hasNgbT? 1: 0;
				const bool hasNgbB = (z < H - 1); const int zOffB = hasNgbB? 1: 0;

				// pretend there are 8 incident triangle faces per vertex
				// for each these triangles, calculate the surface normal,
				// then average the 8 normals (this stays closest to the
				// heightmap data)
				// if edge vertex, don't add virtual neighbor normals to vn
				const float3 vtl = float3((x - 1) * SS,  shm[((z - zOffT) * W) + (x - xOffL)],  (z - 1) * SS);
				const float3 vtm = float3((x    ) * SS,  shm[((z - zOffT) * W) + (x        )],  (z - 1) * SS);
				const float3 vtr = float3((x + 1) * SS,  shm[((z - zOffT) * W) + (x + xOffR)],  (z - 1) * SS);

				const float3 vml = float3((x - 1) * SS,  shm[((z        ) * W) + (x - xOffL)],  (z    ) * SS);
				const float3 vmm = float3((x    ) * SS,  shm[((z        ) * W) + (x        )],  (z    ) * SS);
				const float3 vmr = float3((x + 1) * SS,  shm[((z        ) * W) + (x + xOffR)],  (z    ) * SS);

				const float3 vbl = float3((x - 1) * SS,  shm[((z + zOffB) * W) + (x - xOffL)],  (z + 1) * SS);
				const float3 vbm = float3((x    ) * SS,  shm[((z + zOffB) * W) + (x        )],  (z + 1) * SS);
				const float3 vbr = float3((x + 1) * SS,  shm[((z + zOffB) * W) + (x + xOffR)],  (z + 1) * SS);

				float3 vn = ZeroVector;
				float3 tn = ZeroVector;
					tn = (hasNgbT && hasNgbL)? (vtl - vmm).cross((vtm - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbT           )? (vtm - vmm).cross((vtr - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbT && hasNgbR)? (vtr - vmm).cross((vmr - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbR           )? (vmr - vmm).cross((vbr - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbB && hasNgbR)? (vbr - vmm).cross((vbm - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbB           )? (vbm - vmm).cross((vbl - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbB && hasNgbL)? (vbl - vmm).cross((vml - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (hasNgbL           )? (vml - vmm).cross((vtl - vmm)): ZeroVector;  if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
				float3 fnTL;
				float3 fnBR;

				// update the raw vertex normal
				rvn[vIdxTL] = vn.ANormalize();

				#ifdef USE_UNSYNCED_HEIGHTMAP
				if (update.los) {
					// update the visible vertex/face height/normal
					uhm[vIdxTL] = shm[vIdxTL];
					vvn[vIdxTL] = rvn[vIdxTL];

					if (hasNgbR && hasNgbB) {
						// x == maxx and z == maxz are illegal indices for these
						ufn[fIdxTL    ] = sfn[fIdxTL    ];
						ufn[fIdxBR    ] = sfn[fIdxBR    ];
						ucn[fIdxTL / 2] = scn[fIdxTL / 2];
					}
				}
				#endif

				// compress the range [-1, 1] to [0, 1] to prevent clamping
				// (ideally, should use an FBO with FP32 texture attachment)
				#if (SSMF_UNCOMPRESSED_NORMALS == 1)
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 0] = ((vvn[vIdxTL].x + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 1] = ((vvn[vIdxTL].y + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 2] = ((vvn[vIdxTL].z + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 3] = 1.0f;
				#else
				//! note: y-coord is regenerated in the shader via "sqrt(1 - x*x - z*z)",
				//!   this gives us 2 solutions but we know that the y-coord always points
				//!   upwards, so we can reconstruct it in the shader.
				pixels[((z - minz) * xsize + (x - minx)) * 2 + 0] = ((vvn[vIdxTL].x + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 2 + 1] = ((vvn[vIdxTL].z + 1.0f) * 0.5f);
				#endif
			}
		}

		if (globalRendering->haveGLSL) {
			// <normalsTex> is not used by ARB shaders
			glBindTexture(GL_TEXTURE_2D, normalsTex);
			#if (SSMF_UNCOMPRESSED_NORMALS == 1)
			glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_RGBA, GL_FLOAT, &pixels[0]);
			#else
			glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_LUMINANCE_ALPHA, GL_FLOAT, &pixels[0]);
			#endif
		}
	}

	{
		// ReadMap::UpdateHeightMapSynced clamps to [0, gs->mapx - 1]
		// but we want it to be [0, gs->mapx] for the shading texture
		const int xsize = Clamp((x2 - x1) + 1, 0, gs->mapx);
		const int ysize = Clamp((y2 - y1) + 1, 0, gs->mapy);

		// update the shading texture (even if the map has specular
		// lighting, we still need it to modulate the minimap image,
		// as well as for several extraTexture overlays)
		// this can be done for diffuse lighting only
		std::vector<unsigned char> pixels(xsize * ysize * 4, 0.0f);

		int y;
		#pragma omp parallel for private(y)
		for (y = 0; y < ysize; ++y) {
			UpdateShadingTexPart(y, x1, y1, xsize, &pixels[y * xsize * 4]);
		}

		// redefine the texture subregion
		glBindTexture(GL_TEXTURE_2D, shadingTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


void CSmfReadMap::UpdateShadingTexture() {
	const int xsize = gs->mapxp1;
	const int ysize = gs->mapyp1;
	int y = shadingTexUpdateIter;

	shadingTexUpdateIter = (shadingTexUpdateIter + 1) % (ysize * shadingTexUpdateRate);
	if ((y % shadingTexUpdateRate) != 0)
		return;

	y /= shadingTexUpdateRate;
//	y = (y * 2 + ((((ysize % 2) == 0) && (y >= (ysize / 2))) ? 1 : 0)) % ysize;

	UpdateShadingTexPart(y, 0, 0, xsize, &shadingTexPixelRow[0]);

	glBindTexture(GL_TEXTURE_2D, shadingTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, xsize, 1, GL_RGBA, GL_UNSIGNED_BYTE, &shadingTexPixelRow[0]);
}


float CSmfReadMap::DiffuseSunCoeff(const int& x, const int& y) const
{
	const float3& N = visVertexNormals[(y * gs->mapxp1) + x];
	const float3& L = sky->GetLight()->GetLightDir();
	return Clamp(L.dot(N), 0.0f, 1.0f);
}


float3 CSmfReadMap::GetLightValue(const int& x, const int& y) const
{
	float3 light =
		mapInfo->light.groundAmbientColor +
		mapInfo->light.groundSunColor * DiffuseSunCoeff(x, y);

	for (int a = 0; a < 3; ++a) {
		if (light[a] > 1.0f) {
			light[a] = 1.0f;
		}
	}

	return light;
}


void CSmfReadMap::DrawMinimap() const
{
	glDisable(GL_ALPHA_TEST);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, shadingTex);
	// glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
	// glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	// glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	// glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (groundDrawer->DrawExtraTex()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		glBindTexture(GL_TEXTURE_2D, groundDrawer->infoTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	static float isx = gs->mapx / float(gs->pwr2mapx);
	static float isy = gs->mapy / float(gs->pwr2mapy);

	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
		glTexCoord2f(0, isy);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0, 1);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB, 0, isy);
		glVertex2f(0, 0);
		glTexCoord2f(0, 0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0, 0);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB, 0, 0);
		glVertex2f(0, 1);
		glTexCoord2f(isx, 0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1, 0);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB, isx, 0);
		glVertex2f(1, 1);
		glTexCoord2f(isx, isy);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1, 1);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB, isx, isy);
		glVertex2f(1, 0);
	glEnd();

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	//glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
	//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDisable(GL_TEXTURE_2D);
}


void CSmfReadMap::GridVisibility(CCamera* cam, int quadSize, float maxdist, CReadMap::IQuadDrawer *qd, int extraSize)
{
	const int cx = (int)(cam->pos.x / (SQUARE_SIZE * quadSize));
	const int cy = (int)(cam->pos.z / (SQUARE_SIZE * quadSize));

	const int drawSquare = int(maxdist / (SQUARE_SIZE * quadSize)) + 1;

	const int drawQuadsX = (int)(gs->mapx / quadSize);
	const int drawQuadsY = (int)(gs->mapy / quadSize);

	int sy = cy - drawSquare;
	int ey = cy + drawSquare;
	int sxi = cx - drawSquare;
	int exi = cx + drawSquare;

	if (sy < 0)
		sy = 0;
	if (ey > drawQuadsY - 1)
		ey = drawQuadsY - 1;

	if (sxi < 0)
		sxi = 0;
	if (exi > drawQuadsX - 1)
		exi = drawQuadsX - 1;

	for (int y = sy; y <= ey; y++) {
		int sx = sxi;
		int ex = exi;
		float xtest, xtest2;
		std::vector<CBFGroundDrawer::fline>::iterator fli;
		for (fli = groundDrawer->left.begin(); fli != groundDrawer->left.end(); ++fli) {
			xtest  = ((fli->base + fli->dir * ( y * quadSize)            ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest > xtest2)
				xtest = xtest2;

			xtest /= quadSize;

			if (xtest - extraSize > sx)
				sx = ((int) xtest) - extraSize;
		}
		for (fli = groundDrawer->right.begin(); fli != groundDrawer->right.end(); ++fli) {
			xtest  = ((fli->base + fli->dir *  (y * quadSize)           ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest < xtest2)
				xtest = xtest2;

			xtest /= quadSize;

			if (xtest + extraSize < ex)
				ex = ((int) xtest) + extraSize;
		}

		for (int x = sx; x <= ex; x++)
			qd->DrawQuad(x, y);
	}
}


int CSmfReadMap::GetNumFeatures ()
{
	return file.GetNumFeatures();
}


int CSmfReadMap::GetNumFeatureTypes()
{
	return file.GetNumFeatureTypes();
}


void CSmfReadMap::GetFeatureInfo(MapFeatureInfo* f)
{
	file.ReadFeatureInfo(f);
}


const char* CSmfReadMap::GetFeatureTypeName (int typeID)
{
	return file.GetFeatureTypeName(typeID);
}


unsigned char* CSmfReadMap::GetInfoMap(const std::string& name, MapBitmapInfo* bmInfo)
{
	// get size
	file.GetInfoMapSize(name, bmInfo);
	if (bmInfo->width <= 0) return NULL;

	// get data
	unsigned char* data = new unsigned char[bmInfo->width * bmInfo->height];
	file.ReadInfoMap(name, data);
	return data;
}


void CSmfReadMap::FreeInfoMap(const std::string& name, unsigned char *data)
{
	delete[] data;
}


void CSmfReadMap::ConfigureAnisotropy()
{
	if (!GLEW_EXT_texture_filter_anisotropic) {
		anisotropy = 0.0f;
		return;
	}

	const char* SMFTexAniso = "SMFTexAniso";

	anisotropy = atof(configHandler->GetString(SMFTexAniso, "0.0").c_str());

	if (anisotropy < 1.0f) {
		anisotropy = 0.0f; // disabled
	} else {
		GLfloat maxAniso;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
		if (anisotropy > maxAniso) {
			anisotropy = maxAniso;
			char buf[64];
			SNPRINTF(buf, sizeof(buf), "%f", anisotropy);
			configHandler->SetString(SMFTexAniso, buf);
		}
	}
}
