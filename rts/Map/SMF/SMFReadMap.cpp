/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // mem{set,cpy}

#include "SMFReadMap.h"
#include "SMFGroundTextures.h"
#include "SMFGroundDrawer.h"
#include "SMFFormat.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/LoadScreen.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/bitops.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/ThreadPool.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringHash.h"

#define SSMF_UNCOMPRESSED_NORMALS 0

using std::max;

CONFIG(bool, GroundNormalTextureHighPrecision).defaultValue(false);
CONFIG(float, SMFTexAniso).defaultValue(4.0f).minimumValue(0.0f);
CONFIG(float, SSMFTexAniso).defaultValue(4.0f).minimumValue(0.0f);



CSMFMapFile CSMFReadMap::mapFile;

std::vector<float> CSMFReadMap::cornerHeightMapSynced;
std::vector<float> CSMFReadMap::cornerHeightMapUnsynced;

std::vector<unsigned char> CSMFReadMap::shadingTexBuffer;
std::vector<unsigned char> CSMFReadMap::waterHeightColors;

static std::vector<float> normalPixels;
static std::vector<unsigned char> shadingPixels;



CSMFReadMap::CSMFReadMap(const std::string& mapName): CEventClient("[CSMFReadMap]", 271950, false)
{
	loadscreen->SetLoadMessage("Loading SMF");
	eventHandler.AddClient(this);

	mapFile.Close();
	mapFile.Open(mapName);

	haveSpecularTexture = !(mapInfo->smf.specularTexName.empty());
	haveSplatDetailDistribTexture = (!mapInfo->smf.splatDetailTexName.empty() && !mapInfo->smf.splatDistrTexName.empty());
	haveSplatNormalDistribTexture = false;

	for (const MapTexture& mapTex: splatNormalTextures) {
		assert(!mapTex.HasLuaTex());
		assert(mapTex.GetID() == 0);
	}

	for (const std::string& texName: mapInfo->smf.splatDetailNormalTexNames) {
		haveSplatNormalDistribTexture |= !texName.empty();
	}

	// Detail Normal Splatting requires at least one splatDetailNormalTexture and a distribution texture
	haveSplatNormalDistribTexture &= !mapInfo->smf.splatDistrTexName.empty();

	ParseHeader();
	LoadHeightMap();
	CReadMap::Initialize();

	LoadMinimap();

	ConfigureTexAnisotropyLevels();
	InitializeWaterHeightColors();

	CreateSpecularTex();
	CreateSplatDetailTextures();
	CreateGrassTex();
	CreateDetailTex();
	CreateShadingTex();
	CreateNormalTex();

	mapFile.ReadFeatureInfo();
}



void CSMFReadMap::ParseHeader()
{
	const SMFHeader& header = mapFile.GetHeader();

	mapDims.mapx = header.mapx;
	mapDims.mapy = header.mapy;

	numBigTexX      = (header.mapx / bigSquareSize);
	numBigTexY      = (header.mapy / bigSquareSize);
	bigTexSize      = (SQUARE_SIZE * bigSquareSize);
	tileMapSizeX    = (header.mapx / tileScale);
	tileMapSizeY    = (header.mapy / tileScale);
	tileCount       = (header.mapx * header.mapy) / (tileScale * tileScale);
	mapSizeX        = (header.mapx * SQUARE_SIZE);
	mapSizeZ        = (header.mapy * SQUARE_SIZE);
	maxHeightMapIdx = ((header.mapx + 1) * (header.mapy + 1)) - 1;
	heightMapSizeX  =  (header.mapx + 1);
}


void CSMFReadMap::LoadHeightMap()
{
	const SMFHeader& header = mapFile.GetHeader();

	cornerHeightMapSynced.clear();
	cornerHeightMapSynced.resize((mapDims.mapx + 1) * (mapDims.mapy + 1));
	#ifdef USE_UNSYNCED_HEIGHTMAP
	cornerHeightMapUnsynced.clear();
	cornerHeightMapUnsynced.resize((mapDims.mapx + 1) * (mapDims.mapy + 1));
	#endif

	heightMapSyncedPtr   = &cornerHeightMapSynced;
	heightMapUnsyncedPtr = &cornerHeightMapUnsynced;

	const float minHgt = mapInfo->smf.minHeightOverride ? mapInfo->smf.minHeight : header.minHeight;
	const float maxHgt = mapInfo->smf.maxHeightOverride ? mapInfo->smf.maxHeight : header.maxHeight;
	float* cornerHeightMapSyncedData = (cornerHeightMapSynced.empty())? nullptr: &cornerHeightMapSynced[0];
	float* cornerHeightMapUnsyncedData = (cornerHeightMapUnsynced.empty())? nullptr: &cornerHeightMapUnsynced[0];

	// FIXME:
	//     callchain CReadMap::Initialize --> CReadMap::UpdateHeightMapSynced(0, 0, mapDims.mapx, mapDims.mapy) -->
	//     PushVisibleHeightMapUpdate --> (next UpdateDraw) UpdateHeightMapUnsynced(0, 0, mapDims.mapx, mapDims.mapy)
	//     initializes the UHM a second time
	//     merge them some way so UHM & shadingtex is available from the time readMap got created
	mapFile.ReadHeightmap(cornerHeightMapSyncedData, cornerHeightMapUnsyncedData, minHgt, (maxHgt - minHgt) / 65536.0f);
}


void CSMFReadMap::LoadMinimap()
{
	CBitmap minimapTexBM;

	if (minimapTexBM.Load(mapInfo->smf.minimapTexName)) {
		minimapTex.SetRawTexID(minimapTexBM.CreateTexture());
		minimapTex.SetRawSize(int2(minimapTexBM.xsize, minimapTexBM.ysize));
		return;
	}

	// the minimap is a static texture
	std::vector<unsigned char> minimapTexBuf(MINIMAP_SIZE, 0);
	mapFile.ReadMinimap(&minimapTexBuf[0]);
	// default; only valid for mip 0
	minimapTex.SetRawSize(int2(1024, 1024));

	glGenTextures(1, minimapTex.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, minimapTex.GetID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MINIMAP_NUM_MIPMAP - 1);
	int offset = 0;
	for (unsigned int i = 0; i < MINIMAP_NUM_MIPMAP; i++) {
		const int mipsize = 1024 >> i;
		const int size = ((mipsize + 3) / 4) * ((mipsize + 3) / 4) * 8;
		glCompressedTexImage2D(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipsize, mipsize, 0, size, &minimapTexBuf[0] + offset);
		offset += size;
	}
}


void CSMFReadMap::InitializeWaterHeightColors()
{
	waterHeightColors.clear();
	waterHeightColors.resize(1024 * 4, 0);

	for (int a = 0; a < 1024; ++a) {
		for (int b = 0; b < 3; ++b) {
			const float absorbColor = waterRendering->baseColor[b] - waterRendering->absorb[b] * a;
			const float clampedColor = std::max(waterRendering->minColor[b], absorbColor);
			waterHeightColors[a * 4 + b] = std::min(255.0f, clampedColor * 255.0f);
		}
		waterHeightColors[a * 4 + 3] = 1;
	}
}


void CSMFReadMap::CreateSpecularTex()
{
	if (!haveSpecularTexture)
		return;

	{
		CBitmap specularTexBM;

		// maps wants specular lighting, but no moderation
		if (!specularTexBM.Load(mapInfo->smf.specularTexName))
			specularTexBM.AllocDummy(SColor(255, 255, 255, 255));

		specularTex.SetRawTexID(specularTexBM.CreateTexture());
		specularTex.SetRawSize(int2(specularTexBM.xsize, specularTexBM.ysize));
	}

	{
		CBitmap skyReflectModTexBM;

		// no default 1x1 textures for these
		if (skyReflectModTexBM.Load(mapInfo->smf.skyReflectModTexName)) {
			skyReflectModTex.SetRawTexID(skyReflectModTexBM.CreateTexture());
			skyReflectModTex.SetRawSize(int2(skyReflectModTexBM.xsize, skyReflectModTexBM.ysize));
		}
	}

	{
		CBitmap blendNormalsTexBM;

		if (blendNormalsTexBM.Load(mapInfo->smf.blendNormalsTexName)) {
			blendNormalsTex.SetRawTexID(blendNormalsTexBM.CreateTexture());
			blendNormalsTex.SetRawSize(int2(blendNormalsTexBM.xsize, blendNormalsTexBM.ysize));
		}
	}

	{
		CBitmap lightEmissionTexBM;

		if (lightEmissionTexBM.Load(mapInfo->smf.lightEmissionTexName)) {
			lightEmissionTex.SetRawTexID(lightEmissionTexBM.CreateTexture());
			lightEmissionTex.SetRawSize(int2(lightEmissionTexBM.xsize, lightEmissionTexBM.ysize));
		}
	}

	{
		CBitmap parallaxHeightTexBM;

		if (parallaxHeightTexBM.Load(mapInfo->smf.parallaxHeightTexName)) {
			parallaxHeightTex.SetRawTexID(parallaxHeightTexBM.CreateTexture());
			parallaxHeightTex.SetRawSize(int2(parallaxHeightTexBM.xsize, parallaxHeightTexBM.ysize));
		}
	}
}

void CSMFReadMap::CreateSplatDetailTextures()
{
	if (!haveSplatDetailDistribTexture)
		return;

	{
		CBitmap splatDetailTexBM;

		// if a map supplies an intensity- AND a distribution-texture for
		// detail-splat blending, the regular detail-texture is not used
		// default detail-texture should be all-grey
		if (!splatDetailTexBM.Load(mapInfo->smf.splatDetailTexName))
			splatDetailTexBM.AllocDummy(SColor(127, 127, 127, 127));

		splatDetailTex.SetRawTexID(splatDetailTexBM.CreateTexture(texAnisotropyLevels[true], 0.0f, true));
		splatDetailTex.SetRawSize(int2(splatDetailTexBM.xsize, splatDetailTexBM.ysize));
	}

	{
		CBitmap splatDistrTexBM;

		if (!splatDistrTexBM.Load(mapInfo->smf.splatDistrTexName))
			splatDistrTexBM.AllocDummy(SColor(255, 0, 0, 0));

		splatDistrTex.SetRawTexID(splatDistrTexBM.CreateTexture(texAnisotropyLevels[true], 0.0f, true));
		splatDistrTex.SetRawSize(int2(splatDistrTexBM.xsize, splatDistrTexBM.ysize));
	}

	// only load the splat detail normals if any of them are defined and present
	if (!haveSplatNormalDistribTexture)
		return;

	for (size_t i = 0; i < mapInfo->smf.splatDetailNormalTexNames.size(); i++) {
		if (i == NUM_SPLAT_DETAIL_NORMALS)
			break;

		CBitmap splatDetailNormalTextureBM;

		if (!splatDetailNormalTextureBM.Load(mapInfo->smf.splatDetailNormalTexNames[i])) {
			splatDetailNormalTextureBM.Alloc(1, 1, 4);
			splatDetailNormalTextureBM.GetRawMem()[0] = 127; // RGB is packed standard normal map
			splatDetailNormalTextureBM.GetRawMem()[1] = 127;
			splatDetailNormalTextureBM.GetRawMem()[2] = 255; // With a single upward (+Z) pointing vector
			splatDetailNormalTextureBM.GetRawMem()[3] = 127; // Alpha is diffuse as in old-style detail textures
		}

		splatNormalTextures[i].SetRawTexID(splatDetailNormalTextureBM.CreateTexture(texAnisotropyLevels[true], 0.0f, true));
		splatNormalTextures[i].SetRawSize(int2(splatDetailNormalTextureBM.xsize, splatDetailNormalTextureBM.ysize));
	}

}


void CSMFReadMap::CreateGrassTex()
{
	grassShadingTex.SetRawTexID(minimapTex.GetID());
	grassShadingTex.SetRawSize(int2(1024, 1024));

	CBitmap grassShadingTexBM;

	if (!grassShadingTexBM.Load(mapInfo->smf.grassShadingTexName))
		return;

	// override minimap
	grassShadingTex.SetRawTexID(grassShadingTexBM.CreateMipMapTexture());
	grassShadingTex.SetRawSize(int2(grassShadingTexBM.xsize, grassShadingTexBM.ysize));
}


void CSMFReadMap::CreateDetailTex()
{
	CBitmap detailTexBM;

	if (!detailTexBM.Load(mapInfo->smf.detailTexName))
		detailTexBM.AllocDummy();

	detailTex.SetRawTexID(detailTexBM.CreateTexture(texAnisotropyLevels[false], 0.0f, true));
	detailTex.SetRawSize(int2(detailTexBM.xsize, detailTexBM.ysize));
}


void CSMFReadMap::CreateShadingTex()
{
	shadingTex.SetRawSize(int2(mapDims.pwr2mapx, mapDims.pwr2mapy));

	// the shading/normal texture buffers must have PO2 dimensions
	// (excess elements that no vertices map into are left unused)
	glGenTextures(1, shadingTex.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (texAnisotropyLevels[false] != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, texAnisotropyLevels[false]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mapDims.pwr2mapx, mapDims.pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	shadingTexBuffer.clear();
	shadingTexBuffer.resize(mapDims.mapx * mapDims.mapy * 4, 0);
}


void CSMFReadMap::CreateNormalTex()
{
#if (SSMF_UNCOMPRESSED_NORMALS == 0)
	GLenum texFormat = GL_RG16F;

	if (configHandler->GetBool("GroundNormalTextureHighPrecision"))
		texFormat = GL_RG32F;
#endif

	normalsTex.SetRawSize(int2(mapDims.mapxp1, mapDims.mapyp1));

	glGenTextures(1, normalsTex.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, normalsTex.GetID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if (SSMF_UNCOMPRESSED_NORMALS == 1)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (normalsTex.GetSize()).x, (normalsTex.GetSize()).y, 0, GL_RGBA, GL_FLOAT, nullptr);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, texFormat, (normalsTex.GetSize()).x, (normalsTex.GetSize()).y, 0, GL_RG, GL_FLOAT, nullptr);
#endif
}



void CSMFReadMap::UpdateHeightMapUnsynced(const SRectangle& update)
{
	UpdateVertexNormalsUnsynced(update);
	UpdateFaceNormalsUnsynced(update);
	UpdateNormalTexture(update);
	UpdateShadingTexture(update);
}


void CSMFReadMap::UpdateVertexNormalsUnsynced(const SRectangle& update)
{
	#ifdef USE_UNSYNCED_HEIGHTMAP
	const float*  shm = &cornerHeightMapSynced[0];
		  float*  uhm = &cornerHeightMapUnsynced[0];
		  float3* vvn = &visVertexNormals[0];

	const int W = mapDims.mapxp1;
	const int H = mapDims.mapyp1;
	static const int SS = SQUARE_SIZE;

	// a heightmap update over (x1, y1) - (x2, y2) implies the
	// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)
	const int minx = std::max(update.x1 - 1,     0);
	const int minz = std::max(update.y1 - 1,     0);
	const int maxx = std::min(update.x2 + 1, W - 1);
	const int maxz = std::min(update.y2 + 1, H - 1);

	for_mt(minz, maxz+1, [&](const int z) {
		for (int x = minx; x <= maxx; x++) {
			const int vIdxTL = (z    ) * W + x;

			const int xOffL = (x >     0)? 1: 0;
			const int xOffR = (x < W - 1)? 1: 0;
			const int zOffT = (z >     0)? 1: 0;
			const int zOffB = (z < H - 1)? 1: 0;

			const float sxm1 = (x - 1) * SS;
			const float sx   =       x * SS;
			const float sxp1 = (x + 1) * SS;

			const float szm1 = (z - 1) * SS;
			const float sz   =       z * SS;
			const float szp1 = (z + 1) * SS;

			const int shxm1 = x - xOffL;
			const int shx   = x;
			const int shxp1 = x + xOffR;

			const int shzm1 = (z - zOffT) * W;
			const int shz   =           z * W;
			const int shzp1 = (z + zOffB) * W;

			// pretend there are 8 incident triangle faces per vertex
			// for each these triangles, calculate the surface normal,
			// then average the 8 normals (this stays closest to the
			// heightmap data)
			// if edge vertex, don't add virtual neighbor normals to vn
			const float3 vmm = float3(sx  ,  shm[shz   + shx  ],  sz  );

			const float3 vtl = float3(sxm1,  shm[shzm1 + shxm1],  szm1) - vmm;
			const float3 vtm = float3(sx  ,  shm[shzm1 + shx  ],  szm1) - vmm;
			const float3 vtr = float3(sxp1,  shm[shzm1 + shxp1],  szm1) - vmm;

			const float3 vml = float3(sxm1,  shm[shz   + shxm1],  sz  ) - vmm;
			const float3 vmr = float3(sxp1,  shm[shz   + shxp1],  sz  ) - vmm;

			const float3 vbl = float3(sxm1,  shm[shzp1 + shxm1],  szp1) - vmm;
			const float3 vbm = float3(sx  ,  shm[shzp1 + shx  ],  szp1) - vmm;
			const float3 vbr = float3(sxp1,  shm[shzp1 + shxp1],  szp1) - vmm;

			float3 vn(0.0f, 0.0f, 0.0f);
			vn += vtm.cross(vtl) * (zOffT & xOffL); assert(vtm.cross(vtl).y >= 0.0f);
			vn += vtr.cross(vtm) * (zOffT        ); assert(vtr.cross(vtm).y >= 0.0f);
			vn += vmr.cross(vtr) * (zOffT & xOffR); assert(vmr.cross(vtr).y >= 0.0f);
			vn += vbr.cross(vmr) * (        xOffR); assert(vbr.cross(vmr).y >= 0.0f);
			vn += vtl.cross(vml) * (        xOffL); assert(vtl.cross(vml).y >= 0.0f);
			vn += vbm.cross(vbr) * (zOffB & xOffR); assert(vbm.cross(vbr).y >= 0.0f);
			vn += vbl.cross(vbm) * (zOffB        ); assert(vbl.cross(vbm).y >= 0.0f);
			vn += vml.cross(vbl) * (zOffB & xOffL); assert(vml.cross(vbl).y >= 0.0f);

			// update the visible vertex/face height/normal
			uhm[vIdxTL] = shm[vIdxTL];
			vvn[vIdxTL] = vn.ANormalize();
		}
	});
	#endif
}


void CSMFReadMap::UpdateFaceNormalsUnsynced(const SRectangle& update)
{
	#ifdef USE_UNSYNCED_HEIGHTMAP
	const float3* sfn = &faceNormalsSynced[0];
	      float3* ufn = &faceNormalsUnsynced[0];
	const float3* scn = &centerNormalsSynced[0];
	      float3* ucn = &centerNormalsUnsynced[0];

	// a heightmap update over (x1, y1) - (x2, y2) implies the
	// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)
	const int minx = std::max(update.x1 - 1,          0);
	const int minz = std::max(update.y1 - 1,          0);
	const int maxx = std::min(update.x2 + 1, mapDims.mapxm1);
	const int maxz = std::min(update.y2 + 1, mapDims.mapym1);

	int idx0, idx1;
	for (int z = minz; z <= maxz; z++) {
		idx0  = (z * mapDims.mapx + minx) * 2    ;
		idx1  = (z * mapDims.mapx + maxx) * 2 + 1;
		memcpy(&ufn[idx0], &sfn[idx0], (idx1 - idx0 + 1) * sizeof(float3));

		idx0  = (z * mapDims.mapx + minx);
		idx1  = (z * mapDims.mapx + maxx);
		memcpy(&ucn[idx0], &scn[idx0], (idx1 - idx0 + 1) * sizeof(float3));
	}
	#endif
}


void CSMFReadMap::UpdateNormalTexture(const SRectangle& update)
{
	// Update VertexNormalsTexture;  texture space is [0 .. mapDims.mapx] x [0 .. mapDims.mapy] (NPOT; vertex-aligned)
	float3* vvn = &visVertexNormals[0];

	// a heightmap update over (x1, y1) - (x2, y2) implies the
	// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)
	const int minx = std::max(update.x1 - 1,           0);
	const int minz = std::max(update.y1 - 1,            0);
	const int maxx = std::min(update.x2 + 1, mapDims.mapx);
	const int maxz = std::min(update.y2 + 1, mapDims.mapy);

	const int xsize = (maxx - minx) + 1;
	const int zsize = (maxz - minz) + 1;

	// Note, it doesn't make sense to use a PBO here.
	// Cause the upstreamed float32s need to be transformed to float16s, which seems to happen on the CPU!

#if (SSMF_UNCOMPRESSED_NORMALS == 1)
	normalPixels.clear();
	normalPixels.resize(xsize * zsize * 4, 0.0f);
#else
	normalPixels.clear();
	normalPixels.resize(xsize * zsize * 2, 0.0f);
#endif

	for (int z = minz; z <= maxz; z++) {
		for (int x = minx; x <= maxx; x++) {
			const float3& vertNormal = vvn[z * mapDims.mapxp1 + x];

		#if (SSMF_UNCOMPRESSED_NORMALS == 1)
			normalPixels[((z - minz) * xsize + (x - minx)) * 4 + 0] = vertNormal.x;
			normalPixels[((z - minz) * xsize + (x - minx)) * 4 + 1] = vertNormal.y;
			normalPixels[((z - minz) * xsize + (x - minx)) * 4 + 2] = vertNormal.z;
			normalPixels[((z - minz) * xsize + (x - minx)) * 4 + 3] = 1.0f;
		#else
			// note: y-coord is regenerated in the shader via "sqrt(1 - x*x - z*z)",
			//   this gives us 2 solutions but we know that the y-coord always points
			//   upwards, so we can reconstruct it in the shader.
			normalPixels[((z - minz) * xsize + (x - minx)) * 2 + 0] = vertNormal.x;
			normalPixels[((z - minz) * xsize + (x - minx)) * 2 + 1] = vertNormal.z;
		#endif
		}
	}

	glBindTexture(GL_TEXTURE_2D, normalsTex.GetID());
#if (SSMF_UNCOMPRESSED_NORMALS == 1)
	glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_RGBA, GL_FLOAT, &normalPixels[0]);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_RG, GL_FLOAT, &normalPixels[0]);
#endif
}


void CSMFReadMap::UpdateShadingTexture(const SRectangle& update)
{
	// update the shading texture (even if the map has specular
	// lighting, we still need it to modulate the minimap image)
	// this can be done for diffuse lighting only
	{
		// texture space is [0 .. mapDims.mapxm1] x [0 .. mapDims.mapym1]

		// enlarge rect by 1pixel in all directions (cause we use center normals and not corner ones)
		const int x1 = std::max(update.x1 - 1,          0);
		const int y1 = std::max(update.y1 - 1,          0);
		const int x2 = std::min(update.x2 + 1, mapDims.mapxm1);
		const int y2 = std::min(update.y2 + 1, mapDims.mapym1);

		const int xsize = (x2 - x1) + 1; // +1 cause we iterate:
		const int ysize = (y2 - y1) + 1; // x1 <= xi <= x2  (not!  x1 <= xi < x2)

		//TODO switch to PBO?
		shadingPixels.clear();
		shadingPixels.resize(xsize * ysize * 4, 0.0f);

		for_mt(0, ysize, [&](const int y) {
			const int idx1 = (y + y1) * mapDims.mapx + x1;
			const int idx2 = (y + y1) * mapDims.mapx + x2;
			UpdateShadingTexPart(idx1, idx2, &shadingPixels[y * xsize * 4]);
		});

		// check if we were in a dynamic sun issued shadingTex update
		// and our updaterect was already updated (buffered, not send to the GPU yet!)
		// if so update it in that buffer, too
		if (shadingTexUpdateProgress > (y1 * mapDims.mapx + x1)) {
			for (int y = 0; y < ysize; ++y) {
				const int idx = (y + y1) * mapDims.mapx + x1;
				memcpy(&shadingTexBuffer[idx * 4] , &shadingPixels[y * xsize * 4], xsize);
			}
		}

		// redefine the texture subregion
		glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
		glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, &shadingPixels[0]);
	}
}


const float CSMFReadMap::GetCenterHeightUnsynced(const int x, const int y) const
{
	const float* hm = GetCornerHeightMapUnsynced();
	const float h =
		hm[(y    ) * mapDims.mapxp1 + (x    )] +
		hm[(y    ) * mapDims.mapxp1 + (x + 1)] +
		hm[(y + 1) * mapDims.mapxp1 + (x    )] +
		hm[(y + 1) * mapDims.mapxp1 + (x + 1)];

	return h * 0.25f;
}


void CSMFReadMap::UpdateShadingTexPart(int idx1, int idx2, unsigned char* dst) const
{
	for (int idx = idx1; idx <= idx2; ++idx) {
		const int i = idx - idx1;
		const int xi = idx % mapDims.mapx;
		const int yi = idx / mapDims.mapx;

		const float height = GetCenterHeightUnsynced(xi, yi);

		if (height < 0.0f) {
			// Underwater
			const int clampedHeight = std::min((int)(-height), int(waterHeightColors.size() / 4) - 1);
			float lightIntensity = std::min((DiffuseSunCoeff(xi, yi) + 0.2f) * 2.0f, 1.0f);

			if (height > -10.0f) {
				const float wc = -height * 0.1f;
				const float3 lightColor = GetLightValue(xi, yi) * (1.0f - wc) * 255.0f;

				lightIntensity *= wc;

				dst[i * 4 + 0] = (unsigned char) (waterHeightColors[clampedHeight * 4 + 0] * lightIntensity + lightColor.x);
				dst[i * 4 + 1] = (unsigned char) (waterHeightColors[clampedHeight * 4 + 1] * lightIntensity + lightColor.y);
				dst[i * 4 + 2] = (unsigned char) (waterHeightColors[clampedHeight * 4 + 2] * lightIntensity + lightColor.z);
			} else {
				dst[i * 4 + 0] = (unsigned char) (waterHeightColors[clampedHeight * 4 + 0] * lightIntensity);
				dst[i * 4 + 1] = (unsigned char) (waterHeightColors[clampedHeight * 4 + 1] * lightIntensity);
				dst[i * 4 + 2] = (unsigned char) (waterHeightColors[clampedHeight * 4 + 2] * lightIntensity);
			}
			dst[i * 4 + 3] = EncodeHeight(height);
		} else {
			// Above water
			const float3& light = GetLightValue(xi, yi) * 255.0f;
			dst[i * 4 + 0] = (unsigned char) light.x;
			dst[i * 4 + 1] = (unsigned char) light.y;
			dst[i * 4 + 2] = (unsigned char) light.z;
			dst[i * 4 + 3] = 255;
		}
	}
}


float CSMFReadMap::DiffuseSunCoeff(const int x, const int y) const
{
	const float3& N = centerNormalsUnsynced[y * mapDims.mapx + x];
	const float3& L = sky->GetLight()->GetLightDir();
	return Clamp(L.dot(N), 0.0f, 1.0f);
}


float3 CSMFReadMap::GetLightValue(const int x, const int y) const
{
	float3 light =
		sunLighting->groundAmbientColor +
		sunLighting->groundDiffuseColor * DiffuseSunCoeff(x, y);

	for (int a = 0; a < 3; ++a) {
		light[a] = std::min(light[a] * CGlobalRendering::SMF_INTENSITY_MULT, 1.0f);
	}

	return light;
}

void CSMFReadMap::SunChanged()
{
	if (shadingTexUpdateProgress < 0) {
		shadingTexUpdateProgress = 0;
	} else {
		shadingTexUpdateNeeded = true;
	}

	groundDrawer->SunChanged();
}


void CSMFReadMap::UpdateShadingTexture()
{
	const int xsize = mapDims.mapx;
	const int ysize = mapDims.mapy;
	const int pixels = xsize * ysize;

	// shading texture no longer has much use (minimap etc), limit its updaterate
	//FIXME make configurable or FPS-dependent?
	constexpr int updateRate = 64*64;

	if (shadingTexUpdateProgress < 0)
		return;

	if (shadingTexUpdateProgress >= pixels) {
		if (shadingTexUpdateNeeded) {
			shadingTexUpdateProgress = 0;
			shadingTexUpdateNeeded   = false;
		} else {
			shadingTexUpdateProgress = -1;
		}

		//FIXME use FBO and blend slowly new and old? (this way update rate could reduced even more -> saves CPU time)
		glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, &shadingTexBuffer[0]);
		return;
	}

	const int idx1 = shadingTexUpdateProgress;
	const int idx2 = std::min(idx1 + updateRate, pixels - 1);

	for_mt(idx1, idx2+1, 1025, [&](const int idx){
		const int idx3 = std::min(idx2, idx + 1024);
		UpdateShadingTexPart(idx, idx3, &shadingTexBuffer[idx * 4]);
	});

	shadingTexUpdateProgress += updateRate;
}


void CSMFReadMap::BindMiniMapTextures() const
{
	// tc (0,0) - (1,1)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, minimapTex.GetID());

	glActiveTexture(GL_TEXTURE2);

	// tc (0,0) - (isx,isy)
	if (infoTextureHandler->IsEnabled()) {
		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	} else {
		// just bind this since HAVE_INFOTEX is not available to the minimap shader
		glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
	}

	// tc (0,0) - (isx,isy)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
}


void CSMFReadMap::GridVisibility(CCamera* cam, IQuadDrawer* qd, float maxDist, int quadSize, int extraSize)
{
	if (cam == nullptr) {
		// allow passing in a custom camera for grid-visibility testing
		// otherwise this culls using the state of whichever camera most
		// recently had Update() called on it
		cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_VISCUL);
		// for other cameras, KISS and just assume caller has done this
		cam->CalcFrustumLines(GetCurrMinHeight() - 100.0f, GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	}

	// figure out the camera's own quad
	const int cx = cam->GetPos().x / (SQUARE_SIZE * quadSize);
	const int cy = cam->GetPos().z / (SQUARE_SIZE * quadSize);

	// and how many quads fit into the given maxDist
	const int drawSquare = int(maxDist / (SQUARE_SIZE * quadSize)) + 1;

	const int drawQuadsX = mapDims.mapx / quadSize;
	const int drawQuadsY = mapDims.mapy / quadSize;

	// clamp the area of quads around the camera to valid range
	const int sy  = Clamp(cy - drawSquare, 0, drawQuadsY - 1);
	const int ey  = Clamp(cy + drawSquare, 0, drawQuadsY - 1);
	const int sxi = Clamp(cx - drawSquare, 0, drawQuadsX - 1);
	const int exi = Clamp(cx + drawSquare, 0, drawQuadsX - 1);

	const CCamera::FrustumLine* negLines = cam->GetNegFrustumLines();
	const CCamera::FrustumLine* posLines = cam->GetPosFrustumLines();

	// iterate over quads row-wise between the left and right frustum lines
	for (int y = sy; y <= ey; y++) {
		int sx = sxi;
		int ex = exi;

		float xtest;
		float xtest2;

		// find the starting x-coordinate
		for (int idx = 0, cnt = negLines[4].sign; idx < cnt; idx++) {
			const CCamera::FrustumLine& fl = negLines[idx];

			xtest  = ((fl.base + fl.dir * ( y * quadSize)            ));
			xtest2 = ((fl.base + fl.dir * ((y * quadSize) + quadSize)));

			xtest = std::min(xtest, xtest2);
			xtest = Clamp(xtest / quadSize, -1.0f, drawQuadsX * 1.0f + 1.0f);

			// increase lower bound
			if ((xtest - extraSize) > sx)
				sx = ((int) xtest) - extraSize;
		}

		// find the ending x-coordinate
		for (int idx = 0, cnt = posLines[4].sign; idx < cnt; idx++) {
			const CCamera::FrustumLine& fl = posLines[idx];

			xtest  = ((fl.base + fl.dir *  (y * quadSize)            ));
			xtest2 = ((fl.base + fl.dir * ((y * quadSize) + quadSize)));

			xtest = std::max(xtest, xtest2);
			xtest = Clamp(xtest / quadSize, -1.0f, drawQuadsX * 1.0f + 1.0f);

			// decrease upper bound
			if ((xtest + extraSize) < ex)
				ex = ((int) xtest) + extraSize;
		}

		for (int x = sx; x <= ex; x++) {
			qd->DrawQuad(x, y);
		}
	}
}


int CSMFReadMap::GetNumFeatures() { return mapFile.GetNumFeatures(); }
int CSMFReadMap::GetNumFeatureTypes() { return mapFile.GetNumFeatureTypes(); }

void CSMFReadMap::GetFeatureInfo(MapFeatureInfo* f) { mapFile.ReadFeatureInfo(f); }

const char* CSMFReadMap::GetFeatureTypeName(int typeID) { return mapFile.GetFeatureTypeName(typeID); }


unsigned char* CSMFReadMap::GetInfoMap(const std::string& name, MapBitmapInfo* bmInfo)
{
	// get size
	mapFile.GetInfoMapSize(name, bmInfo);

	if (bmInfo->width <= 0)
		return nullptr;

	unsigned char* data = new unsigned char[bmInfo->width * bmInfo->height];

	CBitmap infomapBM;
	std::string texName;

	switch (hashString(name.c_str())) {
		case hashString("metal"): { texName = mapInfo->smf.metalmapTexName; } break;
		case hashString("type" ): { texName = mapInfo->smf.typemapTexName ; } break;
		case hashString("grass"): { texName = mapInfo->smf.grassmapTexName; } break;
		default: {
			LOG_L(L_WARNING, "[SMFReadMap::%s] unknown texture-name \"%s\"", __func__, name.c_str());
		} break;
	}

	// get data from mapinfo-override texture
	if (!texName.empty() && !infomapBM.LoadGrayscale(texName))
		LOG_L(L_WARNING, "[SMFReadMap::%s] cannot load override-texture \"%s\"", __func__, texName.c_str());

	if (!infomapBM.Empty()) {
		if (infomapBM.xsize == bmInfo->width && infomapBM.ysize == bmInfo->height) {
			memcpy(data, infomapBM.GetRawMem(), bmInfo->width * bmInfo->height);
			return data;
		}

		LOG_L(L_WARNING, "[SMFReadMap::%s] invalid dimensions for override-texture \"%s\": %ix%i != %ix%i",
			__func__, texName.c_str(),
			infomapBM.xsize, infomapBM.ysize,
			bmInfo->width, bmInfo->height
		);
	}

	// get data from map itself
	if (mapFile.ReadInfoMap(name, data))
		return data;

	delete[] data;
	return nullptr;
}


void CSMFReadMap::FreeInfoMap(const std::string& name, unsigned char* data)
{
	delete[] data;
}


void CSMFReadMap::ConfigureTexAnisotropyLevels()
{
	const std::string cfgKeys[2] = {"SMFTexAniso", "SSMFTexAniso"};

	for (unsigned int i = 0; i < 2; i++) {
		texAnisotropyLevels[i] = std::min(configHandler->GetFloat(cfgKeys[i]), globalRendering->maxTexAnisoLvl);
		texAnisotropyLevels[i] *= (texAnisotropyLevels[i] >= 1.0f); // disable AF if less than 1
	}
}


bool CSMFReadMap::SetLuaTexture(const MapTextureData& td) {
	const unsigned int num = Clamp(int(td.num), 0, NUM_SPLAT_DETAIL_NORMALS - 1);

	switch (td.type) {
		case MAP_BASE_GRASS_TEX: { grassShadingTex.SetLuaTexture(td); } break;
		case MAP_BASE_DETAIL_TEX: { detailTex.SetLuaTexture(td); } break;
		case MAP_BASE_MINIMAP_TEX: { minimapTex.SetLuaTexture(td); } break;
		case MAP_BASE_SHADING_TEX: { shadingTex.SetLuaTexture(td); } break;
		case MAP_BASE_NORMALS_TEX: { normalsTex.SetLuaTexture(td); } break;

		case MAP_SSMF_SPECULAR_TEX: { specularTex.SetLuaTexture(td); } break;
		case MAP_SSMF_NORMALS_TEX: { blendNormalsTex.SetLuaTexture(td); } break;

		case MAP_SSMF_SPLAT_DISTRIB_TEX: { splatDistrTex.SetLuaTexture(td); } break;
		case MAP_SSMF_SPLAT_DETAIL_TEX: { splatDetailTex.SetLuaTexture(td); } break;
		case MAP_SSMF_SPLAT_NORMAL_TEX: { splatNormalTextures[num].SetLuaTexture(td); } break;

		case MAP_SSMF_SKY_REFLECTION_TEX: { skyReflectModTex.SetLuaTexture(td); } break;
		case MAP_SSMF_LIGHT_EMISSION_TEX: { lightEmissionTex.SetLuaTexture(td); } break;
		case MAP_SSMF_PARALLAX_HEIGHT_TEX: { parallaxHeightTex.SetLuaTexture(td); } break;

		default: {
			return false;
		} break;
	}

	groundDrawer->UpdateRenderState();
	return true;
}

void CSMFReadMap::InitGroundDrawer() { groundDrawer = new CSMFGroundDrawer(this); }
void CSMFReadMap::KillGroundDrawer() { spring::SafeDelete(groundDrawer); }

// not placed in header since type CSMFGroundDrawer is only forward-declared there
inline CBaseGroundDrawer* CSMFReadMap::GetGroundDrawer() { return groundDrawer; }

