/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "Sm3Map.h"
#include "Sm3GroundDrawer.h"
#include "terrain/TerrainNode.h"

#include "Game/Camera.h"
#include "Game/GameSetup.h"
#include "Map/MapInfo.h"
#include "Map/MapParser.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/myGL.h"
#include "System/ConfigHandler.h"
#include "System/LogOutput.h"
#include "System/TdfParser.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/byteorder.h"

#include <stdexcept>
#include <fstream>



struct Sm3LoadCB: terrain::ILoadCallback
{
	void Write(const char* msg) { logOutput.Print("%s", msg); }
};

// FIXME - temporary, until the LuaParser change is done
static const TdfParser& GetMapDefParser()
{
	static TdfParser tdf(MapParser::GetMapConfigName(gameSetup->MapFile()));
	return tdf;
}


CR_BIND_DERIVED(CSm3ReadMap, CReadMap, (""))

CSm3ReadMap::CSm3ReadMap(const std::string& mapName)
{
	groundDrawer = 0;
	minimapTexture = 0;
	numFeatures = 0;

	try {
		std::string lmsg = "Loading " + mapName;
		PrintLoadMsg(lmsg.c_str());

		if (!mapInfo->sm3.minimap.empty()) {
			CBitmap bmp;
			if (bmp.Load(mapInfo->sm3.minimap)) {
				minimapTexture = bmp.CreateTexture(true);
			}
		}


		renderer = new terrain::Terrain();

		{
			// load the heightmap in advance
			Sm3LoadCB cb;
			renderer->LoadHeightMap(GetMapDefParser(), &cb);

			width = renderer->GetHeightmapWidth() - 1;
			height = renderer->GetHeightmapWidth() - 1; //! note: not height
		}

		CReadMap::Initialize();

		if (GetMapDefParser().SectionExist("map\\featuretypes")) {
			const int numTypes = atoi(GetMapDefParser().SGetValueDef("0", "map\\featuretypes\\numtypes").c_str());
			for (int a = 0; a < numTypes; a++) {
				char loc[100];
				SNPRINTF(loc, 100, "map\\featuretypes\\type%d", a);
				featureTypes.push_back (new std::string(GetMapDefParser().SGetValueDef("TreeType0", loc)));
			}
		}

		LoadFeatureData();
	} catch (content_error& e) {
		ErrorMessageBox(e.what(), "Error:", MBF_OK);
	}
}

CSm3ReadMap::~CSm3ReadMap()
{
	delete groundDrawer;
	delete renderer;

	for (std::vector<std::string*>::iterator fti = featureTypes.begin(); fti != featureTypes.end(); ++fti)
		delete *fti;
	featureTypes.clear();

	glDeleteTextures(1, &minimapTexture);
}



void CSm3ReadMap::ConfigNotify(const std::string& key, const std::string& value)
{
	if (key == "Shadows") {
		bool drawShadows = (std::atoi(value.c_str()) > 0);

		if (renderer->config.useShadowMaps != drawShadows) {
			renderer->config.useShadowMaps = drawShadows;
			renderer->ReloadShaders();
		}
	}
}


CBaseGroundDrawer* CSm3ReadMap::GetGroundDrawer() { return groundDrawer; }
void CSm3ReadMap::NewGroundDrawer() {
	renderer->config.cacheTextures = false;
	renderer->config.forceFallbackTexturing = !!configHandler->Get("SM3ForceFallbackTex", 0);

	if (!renderer->config.forceFallbackTexturing && GLEW_ARB_fragment_shader && GLEW_ARB_shading_language_100) {
		renderer->config.useBumpMaps = true;
		renderer->config.anisotropicFiltering = 0.0f;
	}

	renderer->config.useStaticShadow = false;
	renderer->config.useShadowMaps = shadowHandler->drawShadows;
	renderer->config.terrainNormalMaps = false;
	renderer->config.normalMapLevel = 3;

	/*
	int numStages = atoi(mapDefParser.SGetValueDef("0", "map\\terrain\\numtexturestages").c_str());
	int maxStages = configHandler->Get("SM3MaxTextureStages", 10);
	if (numStages > maxStages) {
		renderer->config.cacheTextures = true;
		renderer->config.cacheTextureSize = 256;
		// renderer->config.detailMod
	}
	*/

	Sm3LoadCB loadcb;
	terrain::LightingInfo lightInfo;
		lightInfo.ambient = mapInfo->light.groundAmbientColor;
	terrain::StaticLight light;
		light.color = mapInfo->light.groundSunColor;
		light.directional = false;
		light.position = mapInfo->light.sunDir * 1000000.0f;
	lightInfo.staticLights.push_back(light);

	renderer->Load(GetMapDefParser(), &lightInfo, &loadcb);

	groundDrawer = new CSm3GroundDrawer(this);

	configHandler->NotifyOnChange(this);
}


void CSm3ReadMap::UpdateHeightmapUnsynced(int x1, int y1, int x2, int y2)
{
	// heightmap is [width+1][height+1]
	x1 -= 2; x2 += 2;
	y1 -= 2; y2 += 2;

	if (x1 <     0) x1 =     0;
	if (x1 > width) x1 = width;
	if (x2 <     0) x2 =     0;
	if (x2 > width) x2 = width;

	if (y1 <     0) y1 =      0;
	if (y1 > width) y1 = height;
	if (y2 <     0) y2 =      0;
	if (y2 > width) y2 = height;

	renderer->HeightmapUpdated(x1, y1, x2 - x1, y2 - y1);
}

void CSm3ReadMap::Update() {}
void CSm3ReadMap::Explosion(float x, float y, float strength) {}
GLuint CSm3ReadMap::GetShadingTexture() const { return 0; } // a texture with RGB for shading and A for height
void CSm3ReadMap::DrawMinimap() const
{
	if (!minimapTexture)
		return;

	 // draw the minimap in a quad (with extends: (0,0)-(1,1))
	glDisable(GL_ALPHA_TEST);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, minimapTexture);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

	if(groundDrawer->DrawExtraTex()){
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		glBindTexture(GL_TEXTURE_2D, groundDrawer->infoTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	float isx=gs->mapx/float(gs->pwr2mapx);
	float isy=gs->mapy/float(gs->pwr2mapy);

	glBegin(GL_QUADS);
		glTexCoord2f(0,isy);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,1);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,0,isy);
		glVertex2f(0,0);
		glTexCoord2f(0,0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,0,0);
		glVertex2f(0,1);
		glTexCoord2f(isx,0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,0);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,isx,0);
		glVertex2f(1,1);
		glTexCoord2f(isx,isy);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB,1,1);
		glMultiTexCoord2fARB(GL_TEXTURE2_ARB,isx,isy);
		glVertex2f(1,0);
	glEnd();

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDisable(GL_TEXTURE_2D);
}

// Feature creation
int CSm3ReadMap::GetNumFeatures ()
{
	return (int)numFeatures;
}

int CSm3ReadMap::GetNumFeatureTypes ()
{
	return featureTypes.size();
}

void CSm3ReadMap::GetFeatureInfo (MapFeatureInfo* f)
{
	std::copy(featureInfo,featureInfo+numFeatures,f);
}

const char *CSm3ReadMap::GetFeatureTypeName (int typeID)
{
	return featureTypes[typeID]->c_str();
}

void CSm3ReadMap::LoadFeatureData()
{
		// returns MapFeatureInfo[GetNumFeatures()]
	std::string fd = GetMapDefParser().SGetValueDef(std::string(),"map\\featuredata");
	if (!fd.empty()) {
		CFileHandler fh(fd);
		if (!fh.FileExists())
			throw content_error("Failed to open feature data file: " + fd);

		unsigned char version;
		fh.Read(&version, 1);

		if (version > 0)
			throw content_error("Map feature data has incorrect version, you are probably using an outdated spring version.");

		unsigned int nf;
		fh.Read(&nf, 4);
		numFeatures = swabdword(nf);

		featureInfo = new MapFeatureInfo[numFeatures];
		for (unsigned int a=0;a<numFeatures;a++) {
			MapFeatureInfo& fi = featureInfo[a];
			fh.Read(&fi.featureType, 4);
			fh.Read(&fi.pos, 12);
			fh.Read(&fi.rotation, 4);

			fi.featureType = swabdword(fi.featureType);
			fi.pos.x = swabfloat(fi.pos.x);
			fi.pos.y = swabfloat(fi.pos.y);
			fi.pos.z = swabfloat(fi.pos.z);
			fi.rotation = swabfloat(fi.rotation);
		}
	}/* //testing features...
	else {
		featureTypes.push_back(new std::string("TreeType0"));

		numFeatures = 1000;
		featureInfo = new MapFeatureInfo[numFeatures];

		for (int a=0;a<numFeatures;a++) {
			MapFeatureInfo& fi = featureInfo[a];
			fi.featureType = featureTypes.size()-1;
			fi.pos.x = gs->randFloat() * width * SQUARE_SIZE;
			fi.pos.z = gs->randFloat() * height * SQUARE_SIZE;
			fi.rotation = 0.0f;
		}
	}*/
}

CSm3ReadMap::InfoMap::InfoMap () {
	w = h = 0;
	data = 0;
}
CSm3ReadMap::InfoMap::~InfoMap () {
	if(data) delete[] data;
}

// Bitmaps (such as metal map, grass map, ...), handling them with a string as type seems flexible...
// Some map types:
//   "metal"  -  metalmap
//   "grass"  -  grassmap
unsigned char* CSm3ReadMap::GetInfoMap(const std::string& name, MapBitmapInfo* bm)
{
	std::string map;
	if (!GetMapDefParser().SGetValue(map, "MAP\\INFOMAPS\\" + name))
		return 0;

	CBitmap img;
	// all infomaps are grayscale
	if (!img.LoadGrayscale(map))
		return 0;

	InfoMap& im = infoMaps[name];

	bm->width = im.w = img.xsize;
	bm->height = im.h = img.ysize;
	im.data = img.mem;
	img.mem = 0;

	return im.data;
}


void CSm3ReadMap::FreeInfoMap (const std::string& name, unsigned char *data)
{
	infoMaps.erase (name);
}

struct DrawGridParms
{
	int quadSize;
	CSm3ReadMap::IQuadDrawer *cb;
	float maxdist;
	Frustum *frustum;
};

static void DrawGrid(terrain::TQuad *tq, DrawGridParms *param)
{
	if (tq->InFrustum(param->frustum)) {
		if (tq->width == param->quadSize)
			param->cb->DrawQuad(tq->qmPos.x, tq->qmPos.y);
		else if (tq->width < param->quadSize)
			return;
		else {
			for (int a=0;a<4;a++)
				DrawGrid(tq->childs[a],param);
		}
	}
}

void CSm3ReadMap::GridVisibility(CCamera *cam, int quadSize, float maxdist, IQuadDrawer *cb, int extraSize)
{
	float aspect = cam->viewport[2]/(float)cam->viewport[3];
	tmpFrustum.CalcCameraPlanes(&cam->pos, &cam->right, &cam->up, &cam->forward, cam->GetTanHalfFov(), aspect);

	DrawGridParms dgp;
	dgp.cb = cb;
	dgp.maxdist = maxdist;
	dgp.quadSize = quadSize;
	dgp.frustum = &tmpFrustum;
	DrawGrid(renderer->GetQuadTree(), &dgp);
}
