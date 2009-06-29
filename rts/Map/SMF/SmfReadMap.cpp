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

//CR_REG_METADATA(CSmfReadMap, (
//				))

CBaseGroundDrawer* CSmfReadMap::GetGroundDrawer ()
{
	return groundDrawer;
}


CSmfReadMap::CSmfReadMap(std::string mapname)
	: file(mapname)
{
	PrintLoadMsg("Opening map file");

	ConfigureAnisotropy();
	usePBO = !!configHandler->Get("UsePBO", 1);

	for(int a=0;a<1024;++a){
		for(int b=0;b<3;++b){
			float c=max(mapInfo->water.minColor[b],mapInfo->water.baseColor[b]-mapInfo->water.absorb[b]*a);
			waterHeightColors[a*4+b]=(unsigned char)(c*210);
		}
		waterHeightColors[a*4+3]=1;
	}

	const SMFHeader& header = file.GetHeader();

	width=header.mapx;
	height=header.mapy;
	gs->mapx=header.mapx;
	gs->mapy=header.mapy;
	gs->mapSquares = gs->mapx*gs->mapy;
	gs->hmapx=gs->mapx/2;
	gs->hmapy=gs->mapy/2;
	gs->pwr2mapx=next_power_of_2(gs->mapx);
	gs->pwr2mapy=next_power_of_2(gs->mapy);

//	logOutput.Print("%i %i",gs->mapx,gs->mapy);
	float3::maxxpos=gs->mapx*SQUARE_SIZE-1;
	float3::maxzpos=gs->mapy*SQUARE_SIZE-1;

	heightmap=new float[(gs->mapx+1)*(gs->mapy+1)];

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

	PrintLoadMsg("Loading detail textures");

	detailTexName = mapInfo->smf.detailTexName;

	CBitmap bm;
	if (!bm.Load(detailTexName)) {
		throw content_error("Could not load detail texture from file " + detailTexName);
	}
	glGenTextures(1, &detailTex);
	glBindTexture(GL_TEXTURE_2D, detailTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);
	if (anisotropy != 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	PrintLoadMsg("Creating overhead texture");

	unsigned char* buf=new unsigned char[MINIMAP_SIZE];
	file.ReadMinimap(buf);

	glGenTextures(1, &minimapTex);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	int offset=0;
	for (unsigned int i = 0; i < MINIMAP_NUM_MIPMAP; i++) {
		int mipsize = 1024>>i;

		int size = ((mipsize+3)/4)*((mipsize+3)/4)*8;

		glCompressedTexImage2DARB(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipsize, mipsize, 0, size, buf + offset);

		offset += size;
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MINIMAP_NUM_MIPMAP-1 );

	delete[] buf;

	PrintLoadMsg("Creating ground shading");

	unsigned char* bufZero = new unsigned char[gs->pwr2mapx * gs->pwr2mapy * 4];
	memset(bufZero, 0, gs->pwr2mapx * gs->pwr2mapy * 4);

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gs->pwr2mapx, gs->pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, bufZero);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	if (anisotropy != 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	delete[] bufZero;

	HeightmapUpdated(0, gs->mapx, 0, gs->mapy);

	groundDrawer=new CBFGroundDrawer(this);

	file.ReadFeatureInfo ();
}


CSmfReadMap::~CSmfReadMap()
{
	delete groundDrawer;
	delete[] heightmap;
	if (detailTex) glDeleteTextures (1, &detailTex);
	if (minimapTex) glDeleteTextures (1, &minimapTex);
	if (shadowTex) glDeleteTextures (1, &shadowTex);
}


void CSmfReadMap::HeightmapUpdatedNow(int x1, int x2, int y1, int y2)
{
	x1-=x1&3;
	x2+=(20004-x2)&3;

	y1-=y1&3;
	y2+=(20004-y2)&3;

	int xsize=x2-x1;
	int ysize=y2-y1;

	//logOutput.Print("%i %i %i %i",x1,x2,y1,y2);
	unsigned char* tempMem=new unsigned char[xsize*ysize*4];
	for(int y=0;y<ysize;++y){
		for(int x=0;x<xsize;++x){
			float height = centerheightmap[(x+x1)+(y+y1)*gs->mapx];

			if(height<0){
				int h=(int)-height;

				if(height>-10){
					float3 light = GetLightValue(x+x1,y+y1)*210.0f;
					float wc=-height*0.1f;
					tempMem[(y*xsize+x)*4+0] = (unsigned char)(waterHeightColors[h*4+0]*wc+light.x*(1-wc));
					tempMem[(y*xsize+x)*4+1] = (unsigned char)(waterHeightColors[h*4+1]*wc+light.y*(1-wc));
					tempMem[(y*xsize+x)*4+2] = (unsigned char)(waterHeightColors[h*4+2]*wc+light.z*(1-wc));
				} else if(h<1024){
					tempMem[(y*xsize+x)*4+0] = waterHeightColors[h*4+0];
					tempMem[(y*xsize+x)*4+1] = waterHeightColors[h*4+1];
					tempMem[(y*xsize+x)*4+2] = waterHeightColors[h*4+2];
				} else {
					tempMem[(y*xsize+x)*4+0] = waterHeightColors[1023*4+0];
					tempMem[(y*xsize+x)*4+1] = waterHeightColors[1023*4+1];
					tempMem[(y*xsize+x)*4+2] = waterHeightColors[1023*4+2];
				}
				tempMem[(y*xsize+x)*4+3] = EncodeHeight(height);
			} else {
				float3 light = GetLightValue(x+x1,y+y1)*210.0f;
				tempMem[(y*xsize+x)*4] = (unsigned char)light.x;
				tempMem[(y*xsize+x)*4+1] = (unsigned char)light.y;
				tempMem[(y*xsize+x)*4+2] = (unsigned char)light.z;
				tempMem[(y*xsize+x)*4+3] = 255;
			}
		}
	}
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexSubImage2D(GL_TEXTURE_2D,0,x1,y1,xsize,ysize,GL_RGBA,GL_UNSIGNED_BYTE,tempMem);

	delete[] tempMem;

}


float3 CSmfReadMap::GetLightValue(int x, int y)
{
	float3 n1 = facenormals[((y * gs->mapx) + x) * 2] +
	            facenormals[((y * gs->mapx) + x) * 2 + 1];
	n1.ANormalize();

	float3 light = mapInfo->light.groundSunColor * mapInfo->light.sunDir.dot(n1);
	for (int a = 0; a < 3; ++a) {
		if (light[a] < 0.0f) {
			light[a] = 0.0f;
		}
	}

	light+=mapInfo->light.groundAmbientColor;
	for (int a = 0; a < 3; ++a) {
		if (light[a] > 1.0f){
			light[a] = 1.0f;
		}
	}

	return light;
}


void CSmfReadMap::DrawMinimap ()
{
	glDisable(GL_ALPHA_TEST);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,2);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

	if(groundDrawer->DrawExtraTex()){
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
		glBindTexture(GL_TEXTURE_2D, groundDrawer->infoTex);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	static float isx=gs->mapx/float(gs->pwr2mapx);
	static float isy=gs->mapy/float(gs->pwr2mapy);

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
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDisable(GL_TEXTURE_2D);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDisable(GL_TEXTURE_2D);
}


void CSmfReadMap::GridVisibility (CCamera *cam, int quadSize, float maxdist, CReadMap::IQuadDrawer *qd, int extraSize)
{
	const SMFHeader& header = file.GetHeader();

	int cx=(int)(cam->pos.x/(SQUARE_SIZE*quadSize));
	int cy=(int)(cam->pos.z/(SQUARE_SIZE*quadSize));

	int drawSquare=int(maxdist/(SQUARE_SIZE*quadSize))+1;

	int drawQuadsX = header.mapx / quadSize;

	int sy=cy-drawSquare;
	if(sy<0)
		sy=0;
	int ey=cy+drawSquare;
	if(ey>=header.mapy/quadSize)
		ey=header.mapy/quadSize-1;

	int sxi=cx-drawSquare;
	if(sxi<0)
		sxi=0;
	int exi=cx+drawSquare;
	if(exi>drawQuadsX-1)
		exi=drawQuadsX-1;

	for(int y=sy;y<=ey;y++){
		int sx = sxi;
		int ex = exi;
		float xtest, xtest2;
		std::vector<CBFGroundDrawer::fline>::iterator fli;
		for (fli = groundDrawer->left.begin(); fli != groundDrawer->left.end(); fli++) {
			xtest  = ((fli->base + fli->dir * ( y * quadSize)            ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest > xtest2)
				xtest = xtest2;

			xtest = xtest / quadSize;

			if (xtest - extraSize > sx)
				sx = ((int) xtest) - extraSize;
		}
		for (fli = groundDrawer->right.begin(); fli != groundDrawer->right.end(); fli++) {
			xtest  = ((fli->base + fli->dir *  (y * quadSize)           ));
			xtest2 = ((fli->base + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest < xtest2)
				xtest = xtest2;

			xtest = xtest / quadSize;

			if (xtest + extraSize < ex)
				ex = ((int) xtest) + extraSize;
		}

		for(int x=sx;x<=ex;x++)
			qd->DrawQuad (x,y);
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


const char *CSmfReadMap::GetFeatureType (int typeID)
{
	return file.GetFeatureType(typeID);
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
