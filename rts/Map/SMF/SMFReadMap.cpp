/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SMFReadMap.h"
#include "SMFGroundTextures.h"
#include "SMFGroundDrawer.h"
#include "SMFFormat.h"
#include "Map/MapInfo.h"
#include "Game/Camera.h"
#include "Game/LoadScreen.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/bitops.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/ThreadPool.h"
#include "System/myMath.h"
#include "System/Util.h"

#define SSMF_UNCOMPRESSED_NORMALS 0

using std::max;

CONFIG(bool, GroundNormalTextureHighPrecision).defaultValue(false);
CONFIG(float, SMFTexAniso).defaultValue(0.0f);

CR_BIND_DERIVED(CSMFReadMap, CReadMap, (""))


CSMFReadMap::CSMFReadMap(std::string mapname)
	: CEventClient("[CSMFReadMap]", 271950, false)
	, file(mapname)
	, detailTex(0)
	, specularTex(0)
	, shadingTex(0)
	, normalsTex(0)
	, minimapTex(0)
	, splatDetailTex(0)
	, splatDistrTex(0)
	, skyReflectModTex(0)
	, detailNormalTex(0)
	, lightEmissionTex(0)
	, parallaxHeightTex(0)
	, groundDrawer(NULL)
{
	loadscreen->SetLoadMessage("Loading SMF");
	eventHandler.AddClient(this);

	haveSpecularTexture = !(mapInfo->smf.specularTexName.empty());
	haveSplatTexture = (!mapInfo->smf.splatDetailTexName.empty() && !mapInfo->smf.splatDistrTexName.empty());

	ParseHeader();
	LoadHeightMap();
	CReadMap::Initialize();

	LoadMinimap();

	ConfigureAnisotropy();
	InitializeWaterHeightColors();

	CreateSpecularTex();
	CreateSplatDetailTextures();
	CreateGrassTex();
	CreateDetailTex();
	CreateShadingTex();
	CreateNormalTex();

	file.ReadFeatureInfo();
}


CSMFReadMap::~CSMFReadMap()
{
	delete groundDrawer;

	glDeleteTextures(1, &detailTex        );
	glDeleteTextures(1, &specularTex      );
	glDeleteTextures(1, &minimapTex       );
	glDeleteTextures(1, &shadingTex       );
	glDeleteTextures(1, &normalsTex       );
	glDeleteTextures(1, &splatDetailTex   );
	glDeleteTextures(1, &splatDistrTex    );
	glDeleteTextures(1, &grassShadingTex  );
	glDeleteTextures(1, &skyReflectModTex );
	glDeleteTextures(1, &detailNormalTex  );
	glDeleteTextures(1, &lightEmissionTex );
	glDeleteTextures(1, &parallaxHeightTex);
}


void CSMFReadMap::ParseHeader()
{
	const SMFHeader& header = file.GetHeader();

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
	const SMFHeader& header = file.GetHeader();

	cornerHeightMapSynced.resize((mapDims.mapx + 1) * (mapDims.mapy + 1));
	#ifdef USE_UNSYNCED_HEIGHTMAP
	cornerHeightMapUnsynced.resize((mapDims.mapx + 1) * (mapDims.mapy + 1));
	#endif

	heightMapSyncedPtr   = &cornerHeightMapSynced;
	heightMapUnsyncedPtr = &cornerHeightMapUnsynced;

	const float minHgt = mapInfo->smf.minHeightOverride ? mapInfo->smf.minHeight : header.minHeight;
	const float maxHgt = mapInfo->smf.maxHeightOverride ? mapInfo->smf.maxHeight : header.maxHeight;
	float* cornerHeightMapSyncedData = (cornerHeightMapSynced.empty())? NULL: &cornerHeightMapSynced[0];
	float* cornerHeightMapUnsyncedData = (cornerHeightMapUnsynced.empty())? NULL: &cornerHeightMapUnsynced[0];

	// FIXME:
	//     callchain CReadMap::Initialize --> CReadMap::UpdateHeightMapSynced(0, 0, mapDims.mapx, mapDims.mapy) -->
	//     PushVisibleHeightMapUpdate --> (next UpdateDraw) UpdateHeightMapUnsynced(0, 0, mapDims.mapx, mapDims.mapy)
	//     initializes the UHM a second time
	//     merge them some way so UHM & shadingtex is available from the time readMap got created
	file.ReadHeightmap(cornerHeightMapSyncedData, cornerHeightMapUnsyncedData, minHgt, (maxHgt - minHgt) / 65536.0f);
}


void CSMFReadMap::LoadMinimap()
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


void CSMFReadMap::InitializeWaterHeightColors()
{
	for (int a = 0; a < 1024; ++a) {
		for (int b = 0; b < 3; ++b) {
			const float absorbColor = mapInfo->water.baseColor[b] - mapInfo->water.absorb[b] * a;
			const float clampedColor = std::max(mapInfo->water.minColor[b], absorbColor);
			waterHeightColors[a * 4 + b] = std::min(255.0f, clampedColor * 255.0f);
		}
		waterHeightColors[a * 4 + 3] = 1;
	}
}


void CSMFReadMap::CreateSpecularTex()
{
	if (!haveSpecularTexture) {
		return;
	}

	CBitmap specularTexBM;
	CBitmap skyReflectModTexBM;
	CBitmap detailNormalTexBM;
	CBitmap lightEmissionTexBM;
	CBitmap parallaxHeightTexBM;

	if (!specularTexBM.Load(mapInfo->smf.specularTexName)) {
		// maps wants specular lighting, but no moderation
		specularTexBM.channels = 4;
		specularTexBM.AllocDummy(SColor(255,255,255,255));
	}

	specularTex = specularTexBM.CreateTexture(false);

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

	if (parallaxHeightTexBM.Load(mapInfo->smf.parallaxHeightTexName)) {
		parallaxHeightTex = parallaxHeightTexBM.CreateTexture(false);
	}
}

void CSMFReadMap::CreateSplatDetailTextures()
{
	if (!haveSplatTexture) {
		return;
	}

	CBitmap splatDistrTexBM;
	CBitmap splatDetailTexBM;

	// if the map supplies an intensity- AND a distribution-texture for
	// detail-splat blending, the regular detail-texture is not used
	if (!splatDetailTexBM.Load(mapInfo->smf.splatDetailTexName)) {
		// default detail-texture should be all-grey
		splatDetailTexBM.channels = 4;
		splatDetailTexBM.AllocDummy(SColor(127,127,127,127));
	}

	if (!splatDistrTexBM.Load(mapInfo->smf.splatDistrTexName)) {
		splatDistrTexBM.channels = 4;
		splatDistrTexBM.AllocDummy(SColor(255,0,0,0));
	}

	splatDetailTex = splatDetailTexBM.CreateTexture(true);
	splatDistrTex = splatDistrTexBM.CreateTexture(true);
}


void CSMFReadMap::CreateGrassTex()
{
	grassShadingTex = minimapTex;

	CBitmap grassShadingTexBM;
	if (grassShadingTexBM.Load(mapInfo->smf.grassShadingTexName)) {
		grassShadingTex = grassShadingTexBM.CreateTexture(true);
	}
}


void CSMFReadMap::CreateDetailTex()
{
	CBitmap detailTexBM;
	if (!detailTexBM.Load(mapInfo->smf.detailTexName)) {
		throw content_error("Could not load detail texture from file " + mapInfo->smf.detailTexName);
	}

	glGenTextures(1, &detailTex);
	glBindTexture(GL_TEXTURE_2D, detailTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	if (anisotropy != 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, detailTexBM.xsize, detailTexBM.ysize, GL_RGBA, GL_UNSIGNED_BYTE, detailTexBM.mem);
}


void CSMFReadMap::CreateShadingTex()
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mapDims.pwr2mapx, mapDims.pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	shadingTexBuffer.resize(mapDims.mapx * mapDims.mapy * 4, 0);
	shadingTexUpdateNeeded   = false;
	shadingTexUpdateProgress = -1;
}


void CSMFReadMap::CreateNormalTex()
{
#if (SSMF_UNCOMPRESSED_NORMALS == 0)
	GLenum texFormat = GL_LUMINANCE_ALPHA16F_ARB;
	if (configHandler->GetBool("GroundNormalTextureHighPrecision")) {
		texFormat = GL_LUMINANCE_ALPHA32F_ARB;
	}
#endif

	normalTexSize.x = mapDims.mapxp1;
	normalTexSize.y = mapDims.mapyp1;
	if (!globalRendering->supportNPOTs) {
		normalTexSize.x = next_power_of_2(normalTexSize.x);
		normalTexSize.y = next_power_of_2(normalTexSize.y);
	}

	glGenTextures(1, &normalsTex);
	glBindTexture(GL_TEXTURE_2D, normalsTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#if (SSMF_UNCOMPRESSED_NORMALS == 1)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, normalTexSize.x, normalTexSize.y, 0, GL_RGBA, GL_FLOAT, NULL);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, texFormat, normalTexSize.x, normalTexSize.y, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL);
#endif
}

void CSMFReadMap::NewGroundDrawer() { groundDrawer = new CSMFGroundDrawer(this); }
CBaseGroundDrawer* CSMFReadMap::GetGroundDrawer() { return groundDrawer; }



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
	// Update VertexNormalsTexture (not used by ARB shaders)
	if (globalRendering->haveGLSL) {
		// texture space is [0 .. mapDims.mapx] x [0 .. mapDims.mapy] (NPOT; vertex-aligned)

		float3* vvn = &visVertexNormals[0];

		// a heightmap update over (x1, y1) - (x2, y2) implies the
		// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)
		const int minx = std::max(update.x1 - 1,        0);
		const int minz = std::max(update.y1 - 1,        0);
		const int maxx = std::min(update.x2 + 1, mapDims.mapx);
		const int maxz = std::min(update.y2 + 1, mapDims.mapy);

		const int xsize = (maxx - minx) + 1;
		const int zsize = (maxz - minz) + 1;

		// Note, it doesn't make sense to use a PBO here.
		// Cause the upstreamed float32s need to be transformed to float16s, which seems to happen on the CPU!
	#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		std::vector<float> pixels(xsize * zsize * 4, 0.0f);
	#else
		std::vector<float> pixels(xsize * zsize * 2, 0.0f);
	#endif

		for (int z = minz; z <= maxz; z++) {
			for (int x = minx; x <= maxx; x++) {
				const float3& vertNormal = vvn[z * mapDims.mapxp1 + x];

			#if (SSMF_UNCOMPRESSED_NORMALS == 1)
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 0] = vertNormal.x;
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 1] = vertNormal.y;
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 2] = vertNormal.z;
				pixels[((z - minz) * xsize + (x - minx)) * 4 + 3] = 1.0f;
			#else
				// note: y-coord is regenerated in the shader via "sqrt(1 - x*x - z*z)",
				//   this gives us 2 solutions but we know that the y-coord always points
				//   upwards, so we can reconstruct it in the shader.
				pixels[((z - minz) * xsize + (x - minx)) * 2 + 0] = vertNormal.x;
				pixels[((z - minz) * xsize + (x - minx)) * 2 + 1] = vertNormal.z;
			#endif
			}
		}

		glBindTexture(GL_TEXTURE_2D, normalsTex);
	#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_RGBA, GL_FLOAT, &pixels[0]);
	#else
		glTexSubImage2D(GL_TEXTURE_2D, 0, minx, minz, xsize, zsize, GL_LUMINANCE_ALPHA, GL_FLOAT, &pixels[0]);
	#endif
	}
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
		std::vector<unsigned char> pixels(xsize * ysize * 4, 0.0f);

		for_mt(0, ysize, [&](const int y) {
			const int idx1 = (y + y1) * mapDims.mapx + x1;
			const int idx2 = (y + y1) * mapDims.mapx + x2;
			UpdateShadingTexPart(idx1, idx2, &pixels[y * xsize * 4]);
		});

		// check if we were in a dynamic sun issued shadingTex update
		// and our updaterect was already updated (buffered, not send to the GPU yet!)
		// if so update it in that buffer, too
		if (shadingTexUpdateProgress > (y1 * mapDims.mapx + x1)) {
			for (int y = 0; y < ysize; ++y) {
				const int idx = (y + y1) * mapDims.mapx + x1;
				memcpy(&shadingTexBuffer[idx * 4] , &pixels[y * xsize * 4], xsize);
			}
		}

		// redefine the texture subregion
		glBindTexture(GL_TEXTURE_2D, shadingTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
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
			const int clampedHeight = std::min((int)(-height), int(sizeof(waterHeightColors) / 4) - 1);
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
		mapInfo->light.groundAmbientColor +
		mapInfo->light.groundSunColor * DiffuseSunCoeff(x, y);
	light *= CGlobalRendering::SMF_INTENSITY_MULT;

	for (int a = 0; a < 3; ++a) {
		light[a] = std::min(light[a], 1.0f);
	}

	return light;
}

void CSMFReadMap::SunChanged(const float3& sunDir)
{
	if (shadingTexUpdateProgress < 0) {
		shadingTexUpdateProgress = 0;
	} else {
		shadingTexUpdateNeeded = true;
	}
}



void CSMFReadMap::UpdateShadingTexture()
{
	const int xsize = mapDims.mapx;
	const int ysize = mapDims.mapy;
	const int pixels = xsize * ysize;

	// with GLSL, the shading texture has very limited use (minimap etc) so we reduce the updaterate
	//FIXME replace with a real check if glsl is used in terrain rendering!
	//FIXME make configurable? or even FPS depending?
	const int update_rate = (globalRendering->haveGLSL ? 64*64 : 64*128);

	if (shadingTexUpdateProgress < 0) {
		return;
	}

	if (shadingTexUpdateProgress >= pixels) {
		if (shadingTexUpdateNeeded) {
			shadingTexUpdateProgress = 0;
			shadingTexUpdateNeeded   = false;
		} else {
			shadingTexUpdateProgress = -1;
		}

		//FIXME use FBO and blend slowly new and old? (this way update rate could reduced even more -> saves CPU time)
		glBindTexture(GL_TEXTURE_2D, shadingTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, &shadingTexBuffer[0]);
		return;
	}

	const int idx1 = shadingTexUpdateProgress;
	const int idx2 = std::min(idx1 + update_rate, pixels - 1);

	for_mt(idx1, idx2+1, 1025, [&](const int idx){
		const int idx3 = std::min(idx2, idx + 1024);
		UpdateShadingTexPart(idx, idx3, &shadingTexBuffer[idx * 4]);
	});

	shadingTexUpdateProgress += update_rate;
}


void CSMFReadMap::DrawMinimap() const
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

	if (infoTextureHandler->IsEnabled()) {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);

	const float isx = mapDims.mapx / float(mapDims.pwr2mapx);
	const float isy = mapDims.mapy / float(mapDims.pwr2mapy);

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


void CSMFReadMap::GridVisibility(CCamera* cam, int quadSize, float maxdist, CReadMap::IQuadDrawer* qd, int extraSize)
{
	const int cx = cam->GetPos().x / (SQUARE_SIZE * quadSize);
	const int cy = cam->GetPos().z / (SQUARE_SIZE * quadSize);

	const int drawSquare = int(maxdist / (SQUARE_SIZE * quadSize)) + 1;

	const int drawQuadsX = mapDims.mapx / quadSize;
	const int drawQuadsY = mapDims.mapy / quadSize;

	int sy  = Clamp(cy - drawSquare, 0, drawQuadsY - 1);
	int ey  = Clamp(cy + drawSquare, 0, drawQuadsY - 1);
	int sxi = Clamp(cx - drawSquare, 0, drawQuadsX - 1);
	int exi = Clamp(cx + drawSquare, 0, drawQuadsX - 1);

	// NOTE:
	//     GridVisibility is only ever passed <camera>, not <cam2>
	//     (but only <cam2> has sides calculated for it at present
	//     by SMFGroundDrawer::UpdateCamRestraints, and older code
	//     iterated over SMFGroundDrawer::{left, right})
	// UpdateCamRestraints(cam);
	CCamera* frustumCam = cam2;

	// When called from within Lua for GetVisible{Units, Features}, camera might not be updated
	if (extraSize == INT_MAX) {
		extraSize = 0;
		frustumCam = cam;
		groundDrawer->UpdateCamRestraints(frustumCam);
	}

	const std::vector<CCamera::FrustumLine> negSides = frustumCam->GetNegFrustumSides();
	const std::vector<CCamera::FrustumLine> posSides = frustumCam->GetPosFrustumSides();

	std::vector<CCamera::FrustumLine>::const_iterator fli;

	for (int y = sy; y <= ey; y++) {
		int sx = sxi;
		int ex = exi;
		float xtest, xtest2;

		for (fli = negSides.begin(); fli != negSides.end(); ++fli) {
			xtest  = ((fli->base + fli->dir * ( y * quadSize)            ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest2 < xtest) //use std::min?
				xtest = xtest2;

			xtest /= quadSize;

			if (xtest - extraSize > sx)
				sx = ((int) xtest) - extraSize;
		}
		for (fli = posSides.begin(); fli != posSides.end(); ++fli) {
			xtest  = ((fli->base + fli->dir *  (y * quadSize)           ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest2 > xtest)
				xtest = xtest2;

			xtest /= quadSize;

			if (xtest + extraSize < ex)
				ex = ((int) xtest) + extraSize;
		}

		for (int x = sx; x <= ex; x++)
			qd->DrawQuad(x, y);
	}
}


int CSMFReadMap::GetNumFeatures ()
{
	return file.GetNumFeatures();
}


int CSMFReadMap::GetNumFeatureTypes()
{
	return file.GetNumFeatureTypes();
}


void CSMFReadMap::GetFeatureInfo(MapFeatureInfo* f)
{
	file.ReadFeatureInfo(f);
}


const char* CSMFReadMap::GetFeatureTypeName (int typeID)
{
	return file.GetFeatureTypeName(typeID);
}


unsigned char* CSMFReadMap::GetInfoMap(const std::string& name, MapBitmapInfo* bmInfo)
{
	// get size
	file.GetInfoMapSize(name, bmInfo);
	if (bmInfo->width <= 0) return NULL;

	// get data
	unsigned char* data = new unsigned char[bmInfo->width * bmInfo->height];
	file.ReadInfoMap(name, data);
	return data;
}


void CSMFReadMap::FreeInfoMap(const std::string& name, unsigned char *data)
{
	delete[] data;
}


void CSMFReadMap::ConfigureAnisotropy()
{
	if (!GLEW_EXT_texture_filter_anisotropic) {
		anisotropy = 0.0f;
		return;
	}

	anisotropy = configHandler->GetFloat("SMFTexAniso");

	if (anisotropy < 1.0f) {
		anisotropy = 0.0f; // disabled
	} else {
		GLfloat maxAniso;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
		if (anisotropy > maxAniso) {
			anisotropy = maxAniso;
			configHandler->Set("SMFTexAniso", anisotropy);
		}
	}
}
