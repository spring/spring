#include "StdAfx.h"
#include "SmfReadMap.h"

#include "mapfile.h"
#include "Map/MapInfo.h"
#include "Rendering/GL/myGL.h"
#include "FileSystem/FileHandler.h"
#include "ConfigHandler.h"
#include "BFGroundTextures.h"
#include "BFGroundDrawer.h"
#include "LogOutput.h"
#include "Sim/Features/FeatureHandler.h"
#include "myMath.h"
#include "Platform/errorhandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "Game/GameSetup.h"
#include "bitops.h"
#include "mmgr.h"
#include "Util.h"
#include "Exceptions.h"

using namespace std;

CR_BIND_DERIVED(CSmfReadMap, CReadMap, (""))


CSmfReadMap::CSmfReadMap(std::string mapname): file(mapname)
{
	PrintLoadMsg("Loading map");

	ConfigureAnisotropy();
	usePBO = !!configHandler->Get("UsePBO", 1);

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
	gs->mapx = header.mapx;
	gs->mapy = header.mapy;
	gs->mapSquares = gs->mapx*gs->mapy;
	gs->hmapx = gs->mapx/2;
	gs->hmapy = gs->mapy/2;
	gs->pwr2mapx = next_power_of_2(gs->mapx);
	gs->pwr2mapy = next_power_of_2(gs->mapy);

	float3::maxxpos = gs->mapx * SQUARE_SIZE - 1;
	float3::maxzpos = gs->mapy * SQUARE_SIZE - 1;

	heightmap = new float[(gs->mapx + 1) * (gs->mapy + 1)];

	const CMapInfo::smf_t& smf = mapInfo->smf;
	const float minH = smf.minHeightOverride ? smf.minHeight : header.minHeight;
	const float maxH = smf.maxHeightOverride ? smf.maxHeight : header.maxHeight;

	const float base = minH;
	const float mod = (maxH - minH) / 65536.0f;

	file.ReadHeightmap(heightmap, base, mod);

	CReadMap::Initialize();

	for (unsigned int a = 0; a < mapname.size(); ++a) {
		mapChecksum += mapname[a];
		mapChecksum *= mapname[a];
	}



	haveSpecularLighting = !(mapInfo->smf.specularTexName.empty());

	CBitmap detailTexBM;
	CBitmap specularTexBM;

	if (!detailTexBM.Load(mapInfo->smf.detailTexName)) {
		throw content_error("Could not load detail texture from file " + mapInfo->smf.detailTexName);
	}

	if (haveSpecularLighting) {
		if (!specularTexBM.Load(mapInfo->smf.specularTexName)) {
			// maps wants specular lighting, but no moderation
			specularTexBM.Alloc(1, 1);
			specularTexBM.mem[0] = 255;
			specularTexBM.mem[1] = 255;
			specularTexBM.mem[2] = 255;
			specularTexBM.mem[3] = 255;
		}
	} else {
		// map does not want any specular lighting contribution
		specularTexBM.Alloc(1, 1);
		specularTexBM.mem[0] = 0;
		specularTexBM.mem[1] = 0;
		specularTexBM.mem[2] = 0;
		specularTexBM.mem[3] = 255;
	}

	specularTex = specularTexBM.CreateTexture(false);



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
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
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
		// the shading/normal texture buffers must have PO2 dimensions
		// (excess elements that no vertices map into are left unused)
		std::vector<unsigned char> shadingTexBuf(gs->pwr2mapx * gs->pwr2mapy * 4, 0);

		glGenTextures(1, &shadingTex);
		glBindTexture(GL_TEXTURE_2D, shadingTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (anisotropy != 0.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, &shadingTexBuf[0]);
	}

	{
		std::vector<float> normalsTexBuf(gs->pwr2mapx * gs->pwr2mapy * 4, 0.0f);

		glGenTextures(1, &normalsTex);
		glBindTexture(GL_TEXTURE_2D, normalsTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, gs->pwr2mapx, gs->pwr2mapy, 0, GL_RGBA, GL_FLOAT, &normalsTexBuf[0]);
	}


	groundDrawer = new CBFGroundDrawer(this);
	file.ReadFeatureInfo();
}


CSmfReadMap::~CSmfReadMap()
{
	delete groundDrawer;
	delete[] heightmap;

	if (detailTex  ) { glDeleteTextures(1, &detailTex  ); }
	if (specularTex) { glDeleteTextures(1, &specularTex); }
	if (minimapTex ) { glDeleteTextures(1, &minimapTex ); }
	if (shadingTex ) { glDeleteTextures(1, &shadingTex ); }
	if (normalsTex ) { glDeleteTextures(1, &normalsTex ); }
}


void CSmfReadMap::UpdateHeightmapUnsynced(int x1, int y1, int x2, int y2)
{
	{
		// discretize x1 and y1 to step-sizes of 4
		//   for x1 and y1, [0, 1, 2, 3, 4, 5, 6, 7, 8, ...] maps to [0, 0, 0, 0, 4, 4, 4, 4, 8, ...]
		//   for x2 and y2, [0, 1, 2, 3, 4, 5, 6, 7, 8, ...] maps to [0, 4, 4, 4, 4, 8, 8, 8, 8, ...]
		x1 -= (x1 & 3);
		y1 -= (y1 & 3);
		x2 += ((20004 - x2) & 3);
		y2 += ((20004 - y2) & 3);

		const int xsize = x2 - x1;
		const int ysize = y2 - y1;

		// update the shading texture (even if the map has specular
		// lighting, we still need it to modulate the minimap image)
		// this can be done for diffuse lighting only
		std::vector<unsigned char> pixels(xsize * ysize * 4, 0.0f);

		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				const int xi = x1 + x, yi = y1 + y;
				const float& height = centerheightmap[(xi) + (yi) * gs->mapx];

				if (height < 0.0f) {
					const int h = (int) - height & 1023; //! waterHeightColors array just holds 1024 colors
					float light = std::min((DiffuseSunCoeff(x + x1, y + y1) + 0.2f) * 2.0f, 1.0f);

					if (height > -10.0f) {
						const float wc = -height * 0.1f;
						const float3 light3 = GetLightValue(x + x1, y + y1) * (1.0f - wc) * 210.0f;
						light *= wc;

						pixels[(y * xsize + x) * 4 + 0] = (unsigned char) (waterHeightColors[h * 4 + 0] * light + light3.x);
						pixels[(y * xsize + x) * 4 + 1] = (unsigned char) (waterHeightColors[h * 4 + 1] * light + light3.y);
						pixels[(y * xsize + x) * 4 + 2] = (unsigned char) (waterHeightColors[h * 4 + 2] * light + light3.z);
					} else {
						pixels[(y * xsize + x) * 4 + 0] = (unsigned char) (waterHeightColors[h * 4 + 0] * light);
						pixels[(y * xsize + x) * 4 + 1] = (unsigned char) (waterHeightColors[h * 4 + 1] * light);
						pixels[(y * xsize + x) * 4 + 2] = (unsigned char) (waterHeightColors[h * 4 + 2] * light);
					}
					pixels[(y * xsize + x) * 4 + 3] = EncodeHeight(height);
				} else {
					const float3 light = GetLightValue(x + x1, y + y1) * 210.0f;

					pixels[(y * xsize + x) * 4 + 0] = (unsigned char) light.x;
					pixels[(y * xsize + x) * 4 + 1] = (unsigned char) light.y;
					pixels[(y * xsize + x) * 4 + 2] = (unsigned char) light.z;
					pixels[(y * xsize + x) * 4 + 3] = 255;
				}
			}
		}

		// redefine the texture subregion
		glBindTexture(GL_TEXTURE_2D, shadingTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	if (haveSpecularLighting) {
		// update the vertex normals
		const float* hm = heightmap;

		const int W = gs->mapx + 1;
		const int H = gs->mapy + 1;

		// a heightmap update over (x1, y1) - (x2, y2) implies the
		// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)
		const int minx = std::max((x1 - 1),     0);
		const int minz = std::max((y1 - 1),     0);
		const int maxx = std::min((x2 + 1), W - 1);
		const int maxz = std::min((y2 + 1), H - 1);

		const int xsize = maxx - minx;
		const int zsize = maxz - minz;

		std::vector<float> pixels((xsize + 1) * (zsize + 1) * 4, 0.0f);

		for (int x = minx; x <= maxx; x++) {
			for (int z = minz; z <= maxz; z++) {
				const int vIdx = (z * (gs->mapx + 1)) + x;

				// don't bother with the edge vertices
				if (x == 0 || x == W - 1) { vertexNormals[vIdx] = UpVector; continue; }
				if (z == 0 || z == H - 1) { vertexNormals[vIdx] = UpVector; continue; }

				// pretend there are 8 incident triangle faces per vertex
				// for each these triangles, calculate the surface normal,
				// then average the 8 normals (this stays closest to the
				// heightmap data)
				const float htl = hm[((z - 1) * W) + (x - 1)]; // vertex to the top-left
				const float htm = hm[((z - 1) * W) + (x    )]; // vertex to the top-middle
				const float htr = hm[((z - 1) * W) + (x + 1)]; // vertex to the top-right

				const float hml = hm[((z    ) * W) + (x - 1)]; // vertex to the middle-left
				const float hmm = hm[((z    ) * W) + (x    )]; // the center vertex
				const float hmr = hm[((z    ) * W) + (x + 1)]; // vertex to the middle-right

				const float hbl = hm[((z + 1) * W) + (x - 1)]; // vertex to the bottom-left
				const float hbm = hm[((z + 1) * W) + (x    )]; // vertex to the bottom-middle
				const float hbr = hm[((z + 1) * W) + (x + 1)]; // vertex to the bottom-right


				const float3 vtl((x - 1) * SQUARE_SIZE, htl, (z - 1) * SQUARE_SIZE);
				const float3 vtm((x    ) * SQUARE_SIZE, htm, (z - 1) * SQUARE_SIZE);
				const float3 vtr((x + 1) * SQUARE_SIZE, htr, (z - 1) * SQUARE_SIZE);

				const float3 vml((x - 1) * SQUARE_SIZE, hml, (z    ) * SQUARE_SIZE);
				const float3 vmm((x    ) * SQUARE_SIZE, hmm, (z    ) * SQUARE_SIZE);
				const float3 vmr((x + 1) * SQUARE_SIZE, hmr, (z    ) * SQUARE_SIZE);

				const float3 vbl((x - 1) * SQUARE_SIZE, hbl, (z + 1) * SQUARE_SIZE);
				const float3 vbm((x    ) * SQUARE_SIZE, hbm, (z + 1) * SQUARE_SIZE);
				const float3 vbr((x + 1) * SQUARE_SIZE, hbr, (z + 1) * SQUARE_SIZE);

				float3 vn = ZeroVector;
				float3 tn;
					tn = (vtl - vmm).cross((vtm - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vtm - vmm).cross((vtr - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vtr - vmm).cross((vmr - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vmr - vmm).cross((vbr - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vbr - vmm).cross((vbm - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vbm - vmm).cross((vbl - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vbl - vmm).cross((vml - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;
					tn = (vml - vmm).cross((vtl - vmm)); if (tn.y < 0.0f) { tn = -tn; }; vn += tn;

				vertexNormals[vIdx] = vn.ANormalize();

				// compress the range [-1, 1] to [0, 1] to prevent clamping
				// (ideally, should use an FBO with FP32 texture attachment)
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 0] = ((vn.x + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 1] = ((vn.y + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 2] = ((vn.z + 1.0f) * 0.5f);
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 3] = 1.0f;
			}
		}

		glBindTexture(GL_TEXTURE_2D, normalsTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_RGBA, GL_FLOAT, &pixels[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


float CSmfReadMap::DiffuseSunCoeff(const int& x, const int& y) const
{
	const float3& n = centernormals[(y * gs->mapx) + x];
	return Clamp(mapInfo->light.sunDir.dot(n), 0.0f, 1.0f);
}


float3 CSmfReadMap::GetLightValue(const int& x, const int& y) const
{
	float3 light = mapInfo->light.groundSunColor * DiffuseSunCoeff(x, y);
	light += mapInfo->light.groundAmbientColor;

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


void CSmfReadMap::GridVisibility (CCamera *cam, int quadSize, float maxdist, CReadMap::IQuadDrawer *qd, int extraSize)
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
		for (fli = groundDrawer->left.begin(); fli != groundDrawer->left.end(); fli++) {
			xtest  = ((fli->base + fli->dir * ( y * quadSize)            ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest > xtest2)
				xtest = xtest2;

			xtest /= quadSize;

			if (xtest - extraSize > sx)
				sx = ((int) xtest) - extraSize;
		}
		for (fli = groundDrawer->right.begin(); fli != groundDrawer->right.end(); fli++) {
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


int CSmfReadMap::GetNumFeatureTypes ()
{
	return file.GetNumFeatureTypes();
}


void CSmfReadMap::GetFeatureInfo (MapFeatureInfo* f)
{
	file.ReadFeatureInfo(f);
}


const char *CSmfReadMap::GetFeatureTypeName (int typeID)
{
	return file.GetFeatureTypeName(typeID);
}


unsigned char *CSmfReadMap::GetInfoMap (const std::string& name, MapBitmapInfo* bmInfo)
{
	// get size
	*bmInfo = file.GetInfoMapSize(name);
	if (bmInfo->width <= 0) return NULL;

	// get data
	unsigned char* data = new unsigned char[bmInfo->width * bmInfo->height];
	file.ReadInfoMap(name, data);
	return data;
}


void CSmfReadMap::FreeInfoMap (const std::string& name, unsigned char *data)
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
