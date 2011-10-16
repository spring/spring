/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SMFReadMap.h"
#include "SMFGroundDrawer.h"
#include "SMFGroundTextures.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "System/Config/ConfigHandler.h"
#include "System/FastMath.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/mmgr.h"
#include "System/Util.h"

#ifdef USE_GML
#include "lib/gml/gmlsrv.h"
extern gmlClientServer<void, int, CUnit*>* gmlProcessor;

void CSMFGroundDrawer::DoDrawGroundRowMT(void* c, int bty) {
	((CSMFGroundDrawer*) c)->DoDrawGroundRow(cam2, bty);
}

void CSMFGroundDrawer::DoDrawGroundShadowLODMT(void* c, int nlod) {
	((CSMFGroundDrawer*) c)->DoDrawGroundShadowLOD(nlod);
}

#endif

#define CLAMP(i) Clamp((i), 0, smfMap->maxHeightMapIdx)

using std::min;
using std::max;

CONFIG(int, GroundDetail).defaultValue(40);
CONFIG(bool, MultiThreadDrawGround).defaultValue(true);
CONFIG(bool, MultiThreadDrawGroundShadow).defaultValue(false);

CONFIG(int, MaxDynamicMapLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(int, AdvMapShading).defaultValue(1);

CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* rm): smfMap(rm)
{
	groundTextures = new CSMFGroundTextures(smfMap);

	viewRadius = configHandler->GetInt("GroundDetail");
	viewRadius += (viewRadius & 1); //! we need a multiple of 2
	if (configHandler->IsSet("ROAM")){
		useROAM=configHandler->GetInt("ROAM");
	}else{
		useROAM=true;
		configHandler->Set("ROAM",1);
	}
	numBigTexX=rm->numBigTexX;
	numBigTexY=rm->numBigTexY;
	useShaders = false;
	waterDrawn = false;

	waterPlaneCamInDispList  = 0;
	waterPlaneCamOutDispList = 0;

	if (mapInfo->water.hasWaterPlane) {
		waterPlaneCamInDispList = glGenLists(1);
		glNewList(waterPlaneCamInDispList, GL_COMPILE);
		CreateWaterPlanes(false);
		glEndList();

		waterPlaneCamOutDispList = glGenLists(1);
		glNewList(waterPlaneCamOutDispList, GL_COMPILE);
		CreateWaterPlanes(true);
		glEndList();
	}

#ifdef USE_GML
	multiThreadDrawGround = configHandler->GetBool("MultiThreadDrawGround");
	multiThreadDrawGroundShadow = configHandler->GetBool("MultiThreadDrawGroundShadow");
#endif

	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicMapLights"));
	advShading = LoadMapShaders();
	if (useROAM){
		#ifdef USE_UNSYNCED_HEIGHTMAP
			LOG("unshmap null value= %f \n",*readmap->GetCornerHeightMapUnsynced());
			LOG("syncedhmap null value= %f \n",*readmap->GetCornerHeightMapSynced());
			
			//ROAM todo: yeah it takes the synced map, not the unsynced one...
			landscape.Init(readmap->GetCornerHeightMapSynced(),gs->mapx,gs->mapy);
		#else
			landscape.Init(readmap->GetCornerHeightMapSynced(),gs->mapx,gs->mapy);
		#endif
		visibilitygrid=new bool[rm->numBigTexX*rm->numBigTexY];
		shc=0;
		dc=0;
	}

}

CSMFGroundDrawer::~CSMFGroundDrawer(void)
{
	delete groundTextures;

	shaderHandler->ReleaseProgramObjects("[SMFGroundDrawer]");

	configHandler->Set("GroundDetail", viewRadius);

	if (waterPlaneCamInDispList) {
		glDeleteLists(waterPlaneCamInDispList, 1);
		glDeleteLists(waterPlaneCamOutDispList, 1);
	}

#ifdef USE_GML
	configHandler->Set("MultiThreadDrawGround", multiThreadDrawGround ? 1 : 0);
	configHandler->Set("MultiThreadDrawGroundShadow", multiThreadDrawGroundShadow ? 1 : 0);
#endif

	lightHandler.Kill();
}

bool CSMFGroundDrawer::LoadMapShaders() {
	#define sh shaderHandler

	smfShaderBaseARB = NULL;
	smfShaderReflARB = NULL;
	smfShaderRefrARB = NULL;
	smfShaderCurrARB = NULL;

	smfShaderDefGLSL = NULL;
	smfShaderAdvGLSL = NULL;
	smfShaderCurGLSL = NULL;

	if (configHandler->GetInt("AdvMapShading") == 0) {
		// not allowed to do shader-based map rendering
		return false;
	}

	if (globalRendering->haveARB && !globalRendering->haveGLSL) {
		smfShaderBaseARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderBaseARB", true);
		smfShaderReflARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderReflARB", true);
		smfShaderRefrARB = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderRefrARB", true);
		smfShaderCurrARB = smfShaderBaseARB;

		// the ARB shaders always assume shadows are on
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
		if (globalRendering->haveGLSL) {
			smfShaderDefGLSL = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderDefGLSL", false);
			smfShaderAdvGLSL = sh->CreateProgramObject("[SMFGroundDrawer]", "SMFShaderAdvGLSL", false);
			smfShaderCurGLSL = smfShaderDefGLSL;

			std::string extraDefs;
				extraDefs += (smfMap->HaveSpecularLighting())?
					"#define SMF_ARB_LIGHTING 0\n":
					"#define SMF_ARB_LIGHTING 1\n";
				extraDefs += (smfMap->HaveSplatTexture())?
					"#define SMF_DETAIL_TEXTURE_SPLATTING 1\n":
					"#define SMF_DETAIL_TEXTURE_SPLATTING 0\n";
				extraDefs += (readmap->initMinHeight > 0.0f || mapInfo->map.voidWater)?
					"#define SMF_WATER_ABSORPTION 0\n":
					"#define SMF_WATER_ABSORPTION 1\n";
				extraDefs += (smfMap->GetSkyReflectModTexture() != 0)?
					"#define SMF_SKY_REFLECTIONS 1\n":
					"#define SMF_SKY_REFLECTIONS 0\n";
				extraDefs += (smfMap->GetDetailNormalTexture() != 0)?
					"#define SMF_DETAIL_NORMALS 1\n":
					"#define SMF_DETAIL_NORMALS 0\n";
				extraDefs += (smfMap->GetLightEmissionTexture() != 0)?
					"#define SMF_LIGHT_EMISSION 1\n":
					"#define SMF_LIGHT_EMISSION 0\n";
				extraDefs +=
					("#define BASE_DYNAMIC_MAP_LIGHT " + IntToString(lightHandler.GetBaseLight()) + "\n") +
					("#define MAX_DYNAMIC_MAP_LIGHTS " + IntToString(lightHandler.GetMaxLights()) + "\n");

			static const std::string shadowDefs[2] = {
				"#define HAVE_SHADOWS 0\n",
				"#define HAVE_SHADOWS 1\n",
			};

			Shader::IProgramObject* smfShaders[2] = {
				smfShaderDefGLSL,
				smfShaderAdvGLSL,
			};

			for (unsigned int i = 0; i < 2; i++) {
				smfShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFVertProg.glsl", extraDefs + shadowDefs[i], GL_VERTEX_SHADER));
				smfShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/SMFFragProg.glsl", extraDefs + shadowDefs[i], GL_FRAGMENT_SHADER));
				smfShaders[i]->Link();

				smfShaders[i]->SetUniformLocation("diffuseTex");          // idx  0
				smfShaders[i]->SetUniformLocation("normalsTex");          // idx  1
				smfShaders[i]->SetUniformLocation("shadowTex");           // idx  2
				smfShaders[i]->SetUniformLocation("detailTex");           // idx  3
				smfShaders[i]->SetUniformLocation("specularTex");         // idx  4
				smfShaders[i]->SetUniformLocation("mapSizePO2");          // idx  5
				smfShaders[i]->SetUniformLocation("mapSize");             // idx  6
				smfShaders[i]->SetUniformLocation("texSquare");           // idx  7
				smfShaders[i]->SetUniformLocation("$UNUSED$");            // idx  8
				smfShaders[i]->SetUniformLocation("lightDir");            // idx  9
				smfShaders[i]->SetUniformLocation("cameraPos");           // idx 10
				smfShaders[i]->SetUniformLocation("$UNUSED$");            // idx 11
				smfShaders[i]->SetUniformLocation("shadowMat");           // idx 12
				smfShaders[i]->SetUniformLocation("shadowParams");        // idx 13
				smfShaders[i]->SetUniformLocation("groundAmbientColor");  // idx 14
				smfShaders[i]->SetUniformLocation("groundDiffuseColor");  // idx 15
				smfShaders[i]->SetUniformLocation("groundSpecularColor"); // idx 16
				smfShaders[i]->SetUniformLocation("groundShadowDensity"); // idx 17
				smfShaders[i]->SetUniformLocation("waterMinColor");       // idx 18
				smfShaders[i]->SetUniformLocation("waterBaseColor");      // idx 19
				smfShaders[i]->SetUniformLocation("waterAbsorbColor");    // idx 20
				smfShaders[i]->SetUniformLocation("splatDetailTex");      // idx 21
				smfShaders[i]->SetUniformLocation("splatDistrTex");       // idx 22
				smfShaders[i]->SetUniformLocation("splatTexScales");      // idx 23
				smfShaders[i]->SetUniformLocation("splatTexMults");       // idx 24
				smfShaders[i]->SetUniformLocation("skyReflectTex");       // idx 25
				smfShaders[i]->SetUniformLocation("skyReflectModTex");    // idx 26
				smfShaders[i]->SetUniformLocation("detailNormalTex");     // idx 27
				smfShaders[i]->SetUniformLocation("lightEmissionTex");    // idx 28
				smfShaders[i]->SetUniformLocation("numMapDynLights");     // idx 29
				smfShaders[i]->SetUniformLocation("normalTexGen");        // idx 30
				smfShaders[i]->SetUniformLocation("specularTexGen");      // idx 31

				smfShaders[i]->Enable();
				smfShaders[i]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
				smfShaders[i]->SetUniform1i(1, 5); // normalsTex  (idx 1, texunit 5)
				smfShaders[i]->SetUniform1i(2, 4); // shadowTex   (idx 2, texunit 4)
				smfShaders[i]->SetUniform1i(3, 2); // detailTex   (idx 3, texunit 2)
				smfShaders[i]->SetUniform1i(4, 6); // specularTex (idx 4, texunit 6)
				smfShaders[i]->SetUniform2f(5, (gs->pwr2mapx * SQUARE_SIZE), (gs->pwr2mapy * SQUARE_SIZE));
				smfShaders[i]->SetUniform2f(6, (gs->mapx * SQUARE_SIZE), (gs->mapy * SQUARE_SIZE));
				smfShaders[i]->SetUniform4fv(9, &sky->GetLight()->GetLightDir().x);
				smfShaders[i]->SetUniform3fv(14, &mapInfo->light.groundAmbientColor[0]);
				smfShaders[i]->SetUniform3fv(15, &mapInfo->light.groundSunColor[0]);
				smfShaders[i]->SetUniform3fv(16, &mapInfo->light.groundSpecularColor[0]);
				smfShaders[i]->SetUniform1f(17, sky->GetLight()->GetGroundShadowDensity());
				smfShaders[i]->SetUniform3fv(18, &mapInfo->water.minColor[0]);
				smfShaders[i]->SetUniform3fv(19, &mapInfo->water.baseColor[0]);
				smfShaders[i]->SetUniform3fv(20, &mapInfo->water.absorb[0]);
				smfShaders[i]->SetUniform1i(21, 7); // splatDetailTex (idx 21, texunit 7)
				smfShaders[i]->SetUniform1i(22, 8); // splatDistrTex (idx 22, texunit 8)
				smfShaders[i]->SetUniform4fv(23, &mapInfo->splats.texScales[0]);
				smfShaders[i]->SetUniform4fv(24, &mapInfo->splats.texMults[0]);
				smfShaders[i]->SetUniform1i(25,  9); // skyReflectTex (idx 25, texunit 9)
				smfShaders[i]->SetUniform1i(26, 10); // skyReflectModTex (idx 26, texunit 10)
				smfShaders[i]->SetUniform1i(27, 11); // detailNormalTex (idx 27, texunit 11)
				smfShaders[i]->SetUniform1i(28, 12); // lightEmisionTex (idx 28, texunit 12)
				smfShaders[i]->SetUniform1i(29,  0); // numMapDynLights (unused)
				smfShaders[i]->SetUniform2f(30, 1.0f / ((smfMap->normalTexSize.x - 1) * SQUARE_SIZE), 1.0f / ((smfMap->normalTexSize.y - 1) * SQUARE_SIZE));
				smfShaders[i]->SetUniform2f(31, 1.0f / (gs->mapx * SQUARE_SIZE), 1.0f / (gs->mapy * SQUARE_SIZE));
				smfShaders[i]->Disable();
				smfShaders[i]->Validate();
			}
		}
	}

	#undef sh
	return (smfShaderCurrARB != NULL || smfShaderCurGLSL != NULL);
}


void CSMFGroundDrawer::UpdateSunDir() {
	/* the GLSL shader may run even w/o shadows and depends on a correct sunDir
	if (!shadowHandler->shadowsLoaded) {
		return;
	}
	*/

	if (smfShaderCurGLSL != NULL) {
		smfShaderCurGLSL->Enable();
		smfShaderCurGLSL->SetUniform4fv(9, &sky->GetLight()->GetLightDir().x);
		smfShaderCurGLSL->SetUniform1f(17, sky->GetLight()->GetGroundShadowDensity());
		smfShaderCurGLSL->Disable();
	} else {
		if (smfShaderCurrARB != NULL) {
			smfShaderCurrARB->Enable();
			smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, sky->GetLight()->GetGroundShadowDensity());
			smfShaderCurrARB->Disable();
		}
	}
}


void CSMFGroundDrawer::CreateWaterPlanes(bool camOufOfMap) {
	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);

	const float xsize = (smfMap->mapSizeX) >> 2;
	const float ysize = (smfMap->mapSizeZ) >> 2;
	const float size = std::min(xsize, ysize);

	CVertexArray* va = GetVertexArray();
	va->Initialize();

	const unsigned char fogColor[4] = {
		(255 * mapInfo->atmosphere.fogColor[0]),
		(255 * mapInfo->atmosphere.fogColor[1]),
		(255 * mapInfo->atmosphere.fogColor[2]),
		 255
	};

	const unsigned char planeColor[4] = {
		(255 * mapInfo->water.planeColor[0]),
		(255 * mapInfo->water.planeColor[1]),
		(255 * mapInfo->water.planeColor[2]),
		 255
	};

	const float alphainc = fastmath::PI2 / 32;
	float alpha,r1,r2;

	float3 p(0.0f, std::min(-200.0f, smfMap->initMinHeight - 400.0f), 0.0f);

	for (int n = (camOufOfMap) ? 0 : 1; n < 4 ; ++n) {
		if ((n == 1) && !camOufOfMap) {
			//! don't render vertices under the map
			r1 = 2 * size;
		} else {
			r1 = n*n * size;
		}

		if (n == 3) {
			//! last stripe: make it thinner (looks better with fog)
			r2 = (n+0.5)*(n+0.5) * size;
		} else {
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
inline void CSMFGroundDrawer::DrawMesh(bool haveShadows, bool inShadowPass, bool drawWaterReflection, bool drawUnitReflection) {

	bool framchanged=false;
	int vispatches=0;
	for (int i=0;i<numBigTexX*numBigTexY;i++){
		landscape.m_Patches[i].SetVisibility();
		if ((bool)landscape.m_Patches[i].isVisibile()!=visibilitygrid[i] &&  drawWaterReflection == false && drawUnitReflection == false){
			framchanged=true;
			visibilitygrid[i]=(bool)landscape.m_Patches[i].isVisibile();
		}
		vispatches+=landscape.m_Patches[i].isVisibile();
	
	}
	//framchanged=true;

	const CCamera* cam = (inShadowPass)? camera: cam2;
	//const bool camMoved = (haveShadows && ((camera->pos - lastCamPos).SqLength() > maxCamDeltaDistSq));
	
	int tricount=0;
	if (shadowHandler->shadowsLoaded==true && !DrawExtraTex()){ 
		//This check is needed, because the shadows are rendered first, and if we
		//update terrain in same frame, we will get flashes of shadows on updates
		//So if shadows are on, we must put the terrain update part in the shadows part.
		if (inShadowPass==true){
			shc++;
			if(framchanged==true || landscape.updateCount>0){ 
				//ONLY reset and retessellate if the set of patches in view has changed, 
				//OR if any one of the terrain blocks have been changed due to explosions/terraform
				landscape.Reset();
				landscape.Tessellate(cam2->pos.x,cam2->pos.y,cam2->pos.z,viewRadius);
				tricount=landscape.Render(this, true, inShadowPass, waterDrawn);
				LOG("ROAM dbg: Framechange, tris=%i, shc=%i, viewrad=%i, inshadowpass=%i, camera=(%5.0f, %5.0f, %5.0f) camera2=  (%5.0f, %5.0f, %5.0f)",
					tricount,
					shc,
					viewRadius,
					inShadowPass,
					camera->pos.x,
					camera->pos.y,
					camera->pos.z,
					cam2->pos.x,
					cam2->pos.y,
					cam2->pos.z
					);
				/*		LogObject() << "Frame changed, id" << shc <<", viewrad is: " << viewRadius << ", triangles pushed:" <<tricount << 
				", inshadowpass=" << inShadowPass <<  " camera:"<< camera->pos.x << " " << camera->pos.y << " " << camera->pos.z 
				<< " cam2:" << cam2->pos.x << " " << cam2->pos.y <<" " << cam2->pos.z <<"\n";
				*/
			}
			else{
				tricount=landscape.Render(this, false, inShadowPass, waterDrawn);
			}								
		}
		else{
			tricount=landscape.Render(this, false, inShadowPass, waterDrawn);
		}
	}else{
		shc++;
		if(framchanged==true || landscape.updateCount>0 ){
			landscape.Reset();
			landscape.Tessellate(cam2->pos.x,cam2->pos.y,cam2->pos.z,viewRadius);
			tricount=landscape.Render(this, true, inShadowPass, waterDrawn);
			LOG("ROAM dbg: Framechange, tris=%i, shc=%i, viewrad=%i, inshadowpass=%i, camera=(%5.0f, %5.0f, %5.0f) camera2=  (%5.0f, %5.0f, %5.0f)",
					tricount,
					shc,
					viewRadius,
					inShadowPass,
					camera->pos.x,
					camera->pos.y,
					camera->pos.z,
					cam2->pos.x,
					cam2->pos.y,
					cam2->pos.z
					);/*LogObject() << "Frame changed, id" << shc <<", viewrad is: " << viewRadius << ", triangles pushed:" <<tricount << 
				", inshadowpass=" << inShadowPass <<  " camera:"<< camera->pos.x << " " << camera->pos.y << " " << camera->pos.z 
				<< " cam2:" << cam2->pos.x << " " << cam2->pos.y <<" " << cam2->pos.z <<"\n";
				*/
		}
		else{
			tricount=landscape.Render(this, false, inShadowPass, waterDrawn);
		}	
	
	}
}
inline void CSMFGroundDrawer::DrawWaterPlane(bool drawWaterReflection) {
	if (!drawWaterReflection) {
		glCallList(camera->pos.IsInBounds()? waterPlaneCamInDispList: waterPlaneCamOutDispList);
	}
}



inline void CSMFGroundDrawer::DrawVertexAQ(CVertexArray* ma, int x, int y)
{
	//! don't send the normals as vertex attributes
	//! (DLOD'ed triangles mess with interpolation)
	//! const float3& n = readmap->vertexNormals[(y * smfMap->heightMapSizeX) + x];

	DrawVertexAQ(ma, x, y, GetVisibleVertexHeight(y * smfMap->heightMapSizeX + x));
}

inline void CSMFGroundDrawer::DrawVertexAQ(CVertexArray* ma, int x, int y, float height)
{
	if (waterDrawn && height < 0.0f) {
		height *= 2.0f;
	}

	ma->AddVertexQ0(x * SQUARE_SIZE, height, y * SQUARE_SIZE);
}

inline void CSMFGroundDrawer::EndStripQ(CVertexArray* ma)
{
	ma->EndStripQ();
}

inline void CSMFGroundDrawer::DrawGroundVertexArrayQ(CVertexArray * &ma)
{
	ma->DrawArray0(GL_TRIANGLE_STRIP);
	ma = GetVertexArray();
}



inline bool CSMFGroundDrawer::BigTexSquareRowVisible(const CCamera* cam, int bty) const {
	const int minz =  bty * smfMap->bigTexSize;
	const int maxz = minz + smfMap->bigTexSize;
	const float miny = readmap->currMinHeight;
	const float maxy = fabs(cam->pos.y);

	const float3 mins(               0, miny, minz);
	const float3 maxs(smfMap->mapSizeX, maxy, maxz);

	return (cam->InView(mins, maxs));
}



inline void CSMFGroundDrawer::FindRange(const CCamera* cam, int& xs, int& xe, int y, int lod) {
	int xt0, xt1;

	const std::vector<CCamera::FrustumLine>& negSides = cam->negFrustumSides;
	const std::vector<CCamera::FrustumLine>& posSides = cam->posFrustumSides;

	std::vector<CCamera::FrustumLine>::const_iterator fli;

	for (fli = negSides.begin(); fli != negSides.end(); ++fli) {
		const float xtf = fli->base + fli->dir * y;
		xt0 = (int)xtf;
		xt1 = (int)(xtf + fli->dir * lod);

		if (xt0 > xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod - lod;

		if (xt0 > xs)
			xs = xt0;
	}
	for (fli = posSides.begin(); fli != posSides.end(); ++fli) {
		const float xtf = fli->base + fli->dir * y;
		xt0 = (int)xtf;
		xt1 = (int)(xtf + fli->dir * lod);

		if (xt0 < xt1)
			xt0 = xt1;

		xt0 = xt0 / lod * lod + lod;

		if (xt0 < xe)
			xe = xt0;
	}
}



inline void CSMFGroundDrawer::DoDrawGroundRow(const CCamera* cam, int bty) {
	if (!BigTexSquareRowVisible(cam, bty)) {
		//! skip this entire row of squares if we can't see it
		return;
	}

	CVertexArray* ma = GetVertexArray();

	bool inStrip = false;
	float x0, x1;
	int x,y;
	int sx = 0;
	int ex = smfMap->numBigTexX;

	//! only process the necessary big squares in the x direction
	const int bigSquareSizeY = bty * smfMap->bigSquareSize;

	const std::vector<CCamera::FrustumLine>& negSides = cam->negFrustumSides;
	const std::vector<CCamera::FrustumLine>& posSides = cam->posFrustumSides;

	std::vector<CCamera::FrustumLine>::const_iterator fli;

	for (fli = negSides.begin(); fli != negSides.end(); ++fli) {
		x0 = fli->base + fli->dir * bigSquareSizeY;
		x1 = x0 + fli->dir * smfMap->bigSquareSize;

		if (x0 > x1)
			x0 = x1;

		x0 /= smfMap->bigSquareSize;

		if (x0 > sx)
			sx = (int) x0;
	}
	for (fli = posSides.begin(); fli != posSides.end(); ++fli) {
		x0 = fli->base + fli->dir * bigSquareSizeY + smfMap->bigSquareSize;
		x1 = x0 + fli->dir * smfMap->bigSquareSize;

		if (x0 < x1)
			x0 = x1;

		x0 /= smfMap->bigSquareSize;

		if (x0 < ex)
			ex = (int) x0;
	}

	if (sx > ex)
		return;

	const float cx2 = cam2->pos.x / SQUARE_SIZE;
	const float cy2 = cam2->pos.z / SQUARE_SIZE;

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

			const int minty = bty * smfMap->bigSquareSize, maxty = minty + smfMap->bigSquareSize;
			const int mintx = btx * smfMap->bigSquareSize, maxtx = mintx + smfMap->bigSquareSize;

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

				FindRange(cam2, /*inout*/ xs, /*inout*/ xe, y, lod);

				// If FindRange modifies (xs, xe) to a (less then) empty range,
				// continue to the next row.
				// If we'd continue, nloop (below) would become negative and we'd
				// allocate a vertex array with negative size.  (mantis #1415)
				if (xe < xs) continue;

				int ylod = y + lod;
				int yhlod = y + hlod;
				int nloop = (xe - xs) / lod + 1;

				ma->EnlargeArrays((52 * nloop), 14 * nloop + 1);

				int yhdx = y * smfMap->heightMapSizeX;
				int ylhdx = yhdx + lod * smfMap->heightMapSizeX;
				int yhhdx = yhdx + hlod * smfMap->heightMapSizeX;

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
							float h1 = (GetVisibleVertexHeight(idx1LOD) + GetVisibleVertexHeight(idx2LOD)) * hocxp + GetVisibleVertexHeight(idx3LOD ) * mocxp;
							float h2 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx1LOD)) * hocxp + GetVisibleVertexHeight(idx1HLOD) * mocxp;
							float h3 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx1LOD)) * hocxp + GetVisibleVertexHeight(idx3HLOD) * mocxp;
							float h4 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx2LOD)) * hocxp + GetVisibleVertexHeight(idx2HLOD) * mocxp;

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
							float h1 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx1LOD)) * hmocyp + GetVisibleVertexHeight(idx1HLOD) * oldcamypart;
							float h2 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx2   )) * hmocyp + GetVisibleVertexHeight(idx3    ) * oldcamypart;
							float h3 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx1LOD)) * hmocyp + GetVisibleVertexHeight(idx3HLOD) * oldcamypart;
							float h4 = (GetVisibleVertexHeight(idx2LOD) + GetVisibleVertexHeight(idx1LOD)) * hmocyp + GetVisibleVertexHeight(idx3LOD ) * oldcamypart;

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
							float h1 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx2LOD)) * hocyp + GetVisibleVertexHeight(idx2HLOD) * mocyp;
							float h2 = (GetVisibleVertexHeight(idx1   ) + GetVisibleVertexHeight(idx2   )) * hocyp + GetVisibleVertexHeight(idx3    ) * mocyp;
							float h3 = (GetVisibleVertexHeight(idx2   ) + GetVisibleVertexHeight(idx1LOD)) * hocyp + GetVisibleVertexHeight(idx3HLOD) * mocyp;
							float h4 = (GetVisibleVertexHeight(idx2LOD) + GetVisibleVertexHeight(idx1LOD)) * hocyp + GetVisibleVertexHeight(idx3LOD ) * mocyp;

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

			const int yst = std::max(ystart - lod, minty);
			const int yed = std::min(yend + lod, maxty);
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
						const int idx1 = CLAMP((y      ) * smfMap->heightMapSizeX + x), idx1LOD = CLAMP(idx1 + lod);
						const int idx2 = CLAMP((y + lod) * smfMap->heightMapSizeX + x), idx2LOD = CLAMP(idx2 + lod);
						const int idx3 = CLAMP((y - lod) * smfMap->heightMapSizeX + x), idx3LOD = CLAMP(idx3 + lod);
						const float h = (GetVisibleVertexHeight(idx3LOD) + GetVisibleVertexHeight(idx2LOD)) * hmcxp +	GetVisibleVertexHeight(idx1LOD) * camxpart;
						DrawVertexAQ(ma, xlod, y, h);
						DrawVertexAQ(ma, xlod, y + lod);
					} else {
						const int idx1 = CLAMP((y       ) * smfMap->heightMapSizeX + x), idx1LOD = CLAMP(idx1 + lod);
						const int idx2 = CLAMP((y +  lod) * smfMap->heightMapSizeX + x), idx2LOD = CLAMP(idx2 + lod);
						const int idx3 = CLAMP((y + dlod) * smfMap->heightMapSizeX + x), idx3LOD = CLAMP(idx3 + lod);
						const float h = (GetVisibleVertexHeight(idx1LOD) + GetVisibleVertexHeight(idx3LOD)) * hmcxp + GetVisibleVertexHeight(idx2LOD) * camxpart;
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
						int idx1 = CLAMP((y      ) * smfMap->heightMapSizeX + x);
						int idx2 = CLAMP((y + lod) * smfMap->heightMapSizeX + x);
						int idx3 = CLAMP((y - lod) * smfMap->heightMapSizeX + x);
						float h = (GetVisibleVertexHeight(idx3) + GetVisibleVertexHeight(idx2)) * hcxp + GetVisibleVertexHeight(idx1) * mcxp;
						DrawVertexAQ(ma, x, y, h);
						DrawVertexAQ(ma, x, y + lod);
					} else {
						int idx1 = CLAMP((y       ) * smfMap->heightMapSizeX + x);
						int idx2 = CLAMP((y +  lod) * smfMap->heightMapSizeX + x);
						int idx3 = CLAMP((y + dlod) * smfMap->heightMapSizeX + x);
						float h = (GetVisibleVertexHeight(idx1) + GetVisibleVertexHeight(idx3)) * hcxp + GetVisibleVertexHeight(idx2) * mcxp;
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
				int xs = std::max(xstart - lod, mintx);
				int xe = std::min(xend + lod,   maxtx);
				FindRange(cam2, xs, xe, y, lod);

				if (xs < xe) {
					x = xs;
					int ylod = y + lod;
					int nloop = (xe - xs) / lod + 2; //! one extra for if statment
					int ylhdx = (y + lod) * smfMap->heightMapSizeX;

					ma->EnlargeArrays((2 * nloop), 1);

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
					EndStripQ(ma);
				}
			}

			if (minly > minty && minly < maxty) {
				y = minly - lod;
				int xs = std::max(xstart - lod, mintx);
				int xe = std::min(xend + lod,   maxtx);
				FindRange(cam2, xs, xe, y, lod);

				if (xs < xe) {
					x = xs;
					int ylod = y + lod;
					int yhdx = y * smfMap->heightMapSizeX;
					int nloop = (xe - xs) / lod + 2; //! one extra for if statment

					ma->EnlargeArrays((2 * nloop), 1);

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
					EndStripQ(ma);
				}
			}

		} //for (int lod = 1; lod < neededLod; lod <<= 1)

		SetupBigSquare(btx, bty);
		DrawGroundVertexArrayQ(ma);
	}
}

void CSMFGroundDrawer::Draw(bool drawWaterReflection, bool drawUnitReflection)
{
	const int baseViewRadius = std::max(4, viewRadius);

	if (mapInfo->map.voidWater && readmap->currMaxHeight < 0.0f) {
		return;
	}

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	// needed for the non-shader case
	glEnable(GL_TEXTURE_2D);

	smfShaderCurGLSL = shadowHandler->shadowsLoaded? smfShaderAdvGLSL: smfShaderDefGLSL;
	useShaders = advShading && (!DrawExtraTex() && ((smfShaderCurrARB != NULL && shadowHandler->shadowsLoaded) || (smfShaderCurGLSL != NULL)));
	waterDrawn = drawWaterReflection;

	if (drawUnitReflection) {
		viewRadius = (int(viewRadius * LODScaleUnitReflection)) & 0xfffffe;
	}
	if (drawWaterReflection) {
		viewRadius = (int(viewRadius * LODScaleReflection)) & 0xfffffe;
	}
	// if (drawWaterRefraction) {
	//     viewRadius = (int(viewRadius * LODScaleRefraction)) & 0xfffffe;
	// }

	viewRadius  = int(viewRadius * fastmath::apxsqrt(45.0f / camera->GetFov()));
	viewRadius  = std::max(std::max(smfMap->numBigTexY, smfMap->numBigTexX), viewRadius);
	viewRadius += (viewRadius & 1); //! we need a multiple of 2
	neededLod   = std::max(1, int((globalRendering->viewRange * 0.125f) / viewRadius) << 1);
	neededLod   = std::min(neededLod, std::min(gs->mapx, gs->mapy));

	UpdateCamRestraints(cam2);
	SetupTextureUnits(drawWaterReflection);

	if (mapInfo->map.voidWater && !waterDrawn) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.9f);
	}

	{ // profiler scope
#ifdef USE_GML
		// Profiler results, 4 threads: multiThreadDrawGround is faster only if ViewRadius is below 60 (probably depends on memory/cache speeds)
		const bool mt = GML_PROFILER(multiThreadDrawGround)

		if (mt) {
			gmlProcessor->Work(
				NULL,                                 // wrk
				&CSMFGroundDrawer::DoDrawGroundRowMT, // wrka
				NULL,                                 // wrkit
				this,                                 // cls
				gmlThreadCount,                       // mt
				FALSE,                                // sm
				NULL,                                 // it
				smfMap->numBigTexY,                   // nu
				50,                                   // l1
				100,                                  // l2
				TRUE                                  // sw
			);
		} else
#endif
		if (useROAM){
			//LogObject() << "Calling DrawMesh with drawWaterReflection " << drawWaterReflection<< ", drawUnitReflection " << drawUnitReflection<< "\n" ;
			DrawMesh(shadowHandler->shadowsLoaded, false, drawWaterReflection, drawUnitReflection);

		}else
			{
				int camBty = math::floor(cam2->pos.z / (smfMap->bigSquareSize * SQUARE_SIZE));
				camBty = std::max(0, std::min(smfMap->numBigTexY - 1, camBty));

				//! try to render in "front to back" (so start with the camera nearest BigGroundLines)
				for (int bty = camBty; bty >= 0; --bty) {
					DoDrawGroundRow(cam2, bty);
				}
				for (int bty = camBty + 1; bty < smfMap->numBigTexY; ++bty) {
					DoDrawGroundRow(cam2, bty);
				}
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

		groundDecals->Draw();
		projectileDrawer->DrawGroundFlashes();
	}

	viewRadius = baseViewRadius;
}


inline void CSMFGroundDrawer::DoDrawGroundShadowLOD(int nlod) {
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
	const int ysquaremod = (cy % dlod) / lod;
	const int xsquaremod = (cx % dlod) / lod;

	const float camxpart = (cx2 - (cx / dlod) * dlod) / dlod;
	const float camypart = (cy2 - (cy / dlod) * dlod) / dlod;

	const int minty = 0, maxty = gs->mapy;
	const int mintx = 0, maxtx = gs->mapx;

	const int minly = cy + (-viewRadius + 3 - ysquaremod) * lod, maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
	const int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod, maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

	const int xstart = std::max(minlx, mintx), xend   = std::min(maxlx, maxtx);
	const int ystart = std::max(minly, minty), yend   = std::min(maxly, maxty);

	const int lhdx = lod * smfMap->heightMapSizeX;
	const int hhdx = hlod * smfMap->heightMapSizeX;
	const int dhdx = dlod * smfMap->heightMapSizeX;

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
		int ydx = y * smfMap->heightMapSizeX;
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
				if (x <= cx - vrhlod) {
					const float h1 = (GetVisibleVertexHeight(yhdx+lod) + GetVisibleVertexHeight(ylhdx+lod)) * hocxp + GetVisibleVertexHeight(yhhdx+lod ) * mocxp;
					const float h2 = (GetVisibleVertexHeight(yhdx    ) + GetVisibleVertexHeight(yhdx+lod )) * hocxp + GetVisibleVertexHeight(yhdx+hlod ) * mocxp;
					const float h3 = (GetVisibleVertexHeight(ylhdx   ) + GetVisibleVertexHeight(yhdx+lod )) * hocxp + GetVisibleVertexHeight(yhhdx+hlod) * mocxp;
					const float h4 = (GetVisibleVertexHeight(ylhdx   ) + GetVisibleVertexHeight(ylhdx+lod)) * hocxp + GetVisibleVertexHeight(ylhdx+hlod) * mocxp;

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
				if (y >= cy + vrhlod) {
					const float h1 = (GetVisibleVertexHeight(yhdx     ) + GetVisibleVertexHeight(yhdx+lod)) * hmocyp + GetVisibleVertexHeight(yhdx+hlod ) * oldcamypart;
					const float h2 = (GetVisibleVertexHeight(yhdx     ) + GetVisibleVertexHeight(ylhdx   )) * hmocyp + GetVisibleVertexHeight(yhhdx     ) * oldcamypart;
					const float h3 = (GetVisibleVertexHeight(ylhdx    ) + GetVisibleVertexHeight(yhdx+lod)) * hmocyp + GetVisibleVertexHeight(yhhdx+hlod) * oldcamypart;
					const float h4 = (GetVisibleVertexHeight(ylhdx+lod) + GetVisibleVertexHeight(yhdx+lod)) * hmocyp + GetVisibleVertexHeight(yhhdx+lod ) * oldcamypart;

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
				if (y <= cy - vrhlod) {
					const float h1 = (GetVisibleVertexHeight(ylhdx    ) + GetVisibleVertexHeight(ylhdx+lod)) * hocyp + GetVisibleVertexHeight(ylhdx+hlod) * mocyp;
					const float h2 = (GetVisibleVertexHeight(yhdx     ) + GetVisibleVertexHeight(ylhdx    )) * hocyp + GetVisibleVertexHeight(yhhdx     ) * mocyp;
					const float h3 = (GetVisibleVertexHeight(ylhdx    ) + GetVisibleVertexHeight(yhdx+lod )) * hocyp + GetVisibleVertexHeight(yhhdx+hlod) * mocyp;
					const float h4 = (GetVisibleVertexHeight(ylhdx+lod) + GetVisibleVertexHeight(yhdx+lod )) * hocyp + GetVisibleVertexHeight(yhhdx+lod ) * mocyp;

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

	int yst = std::max(ystart - lod, minty);
	int yed = std::min(yend + lod, maxty);
	int nloop = (yed - yst) / lod + 1;

	if (nloop > 0)
		ma->EnlargeArrays((8 * nloop), 2 * nloop);

	//!rita yttre begr?snings yta mot n?ta lod
	if (maxlx < maxtx && maxlx >= mintx) {
		x = maxlx;
		const int xlod = x + lod;

		for (y = yst; y < yed; y += lod) {
			DrawVertexAQ(ma, x, y      );
			DrawVertexAQ(ma, x, y + lod);
			const int yhdx = y * smfMap->heightMapSizeX + x;

			if (y % dlod) {
				const float h = (GetVisibleVertexHeight(yhdx - lhdx + lod) + GetVisibleVertexHeight(yhdx + lhdx + lod)) * hmcxp + GetVisibleVertexHeight(yhdx+lod) * camxpart;
				DrawVertexAQ(ma, xlod, y, h);
				DrawVertexAQ(ma, xlod, y + lod);
			} else {
				const float h = (GetVisibleVertexHeight(yhdx+lod) + GetVisibleVertexHeight(yhdx+dhdx+lod)) * hmcxp + GetVisibleVertexHeight(yhdx+lhdx+lod) * camxpart;
				DrawVertexAQ(ma, xlod,y);
				DrawVertexAQ(ma, xlod,y+lod,h);
			}
			EndStripQ(ma);
		}
	}

	if (minlx > mintx && minlx < maxtx) {
		x = minlx - lod;
		const int xlod = x + lod;

		for(y = yst; y < yed; y += lod) {
			int yhdx = y * smfMap->heightMapSizeX + x;
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
			EndStripQ(ma);
		}
	}
	if (maxly < maxty && maxly > minty) {
		y = maxly;
		const int xs = std::max(xstart -lod, mintx);
		const int xe = std::min(xend + lod, maxtx);

		if (xs < xe) {
			x = xs;
			const int ylod = y + lod;
			const int ydx = y * smfMap->heightMapSizeX;
			const int nloop = (xe - xs) / lod + 2; //! two extra for if statment

			ma->EnlargeArrays((2 * nloop), 1);

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
			EndStripQ(ma);
		}
	}
	if (minly > minty && minly < maxty) {
		y = minly - lod;
		const int xs = std::max(xstart - lod, mintx);
		const int xe = std::min(xend + lod, maxtx);

		if (xs < xe) {
			x = xs;
			const int ylod = y + lod;
			const int ydx = y * smfMap->heightMapSizeX;
			const int nloop = (xe - xs) / lod + 2; //! two extra for if statment

			ma->EnlargeArrays((2 * nloop), 1);

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
			EndStripQ(ma);
		}
	}
	DrawGroundVertexArrayQ(ma);
}


void CSMFGroundDrawer::DrawShadowPass(void)
{
	if (mapInfo->map.voidWater && readmap->currMaxHeight < 0.0f) {
		return;
	}

	const int NUM_LODS = 4;

	Shader::IProgramObject* po =
		shadowHandler->GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MAP);

	glPolygonOffset(-1.f, -1.f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	po->Enable();

	{ // profiler scope
#ifdef USE_GML
		// Profiler results, 4 threads: multiThreadDrawGroundShadow is rarely faster than single threaded rendering (therefore disabled by default)
		const bool mt = GML_PROFILER(multiThreadDrawGroundShadow)

		if (mt) {
			gmlProcessor->Work(
				NULL,                                       // wrk
				&CSMFGroundDrawer::DoDrawGroundShadowLODMT, // wrka
				NULL,                                       // wrkit
				this,                                       // cls
				gmlThreadCount,                             // mt
				FALSE,                                      // sm
				NULL,                                       // it
				NUM_LODS + 1,                               // nu
				50,                                         // l1
				100,                                        // l2
				TRUE                                        // sw
			);
		} else
#endif
		{
			if (useROAM){
			//LogObject() << "Calling DrawMesh with drawWaterReflection " << drawWaterReflection<< ", drawUnitReflection " << drawUnitReflection<< "\n" ;
			DrawMesh(shadowHandler->shadowsLoaded, true, false, false);

			}else{
				for (int nlod = 0; nlod < NUM_LODS + 1; ++nlod) {
					DoDrawGroundShadowLOD(nlod);
				}
			}
		}
	}

	po->Disable();

	glDisable(GL_POLYGON_OFFSET_FILL);
}




void CSMFGroundDrawer::SetupBigSquare(const int bigSquareX, const int bigSquareY)
{
	groundTextures->BindSquareTexture(bigSquareX, bigSquareY);

	if (useShaders) {
		if (smfShaderCurGLSL != NULL) {
			smfShaderCurGLSL->SetUniform2i(7, bigSquareX, bigSquareY);
		} else {
			if (smfShaderCurrARB != NULL && shadowHandler->shadowsLoaded) {
				smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				smfShaderCurrARB->SetUniform4f(11, -bigSquareX, -bigSquareY, 0, 0);
			}
		}
	} else {
		SetTexGen(1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, -bigSquareX, -bigSquareY);
	}
}






void CSMFGroundDrawer::SetupTextureUnits(bool drawReflection)
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
		glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
		if (drawMode == drawMetal) { // increased brightness for metal spots
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		}
		SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

		glActiveTexture(GL_TEXTURE3);
		if (smfMap->GetDetailTexture()) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
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

		#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
		//! CDynamicWater overrides smfShaderBaseARB during the reflection / refraction
		//! pass to distort underwater geometry, but because it's hard to maintain only
		//! a vertex shader when working with texture combiners we don't enable this
		//! note: we also want to disable culling for these passes
		if (smfShaderCurrARB != smfShaderBaseARB) {
			smfShaderCurrARB->Enable();
			smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);

			smfShaderCurrARB->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
			smfShaderCurrARB->SetUniform4f(12, 1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, 0, 1);
			smfShaderCurrARB->SetUniform4f(13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
			smfShaderCurrARB->SetUniform4f(14, 0.02f, 0.02f, 0, 1);

			if (drawReflection) {
				glAlphaFunc(GL_GREATER, 0.9f);
				glEnable(GL_ALPHA_TEST);
			}
		}
		#endif
	} else {
		if (useShaders) {
			const float3 ambientColor = mapInfo->light.groundAmbientColor * (210.0f / 255.0f);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());

			if (drawReflection) {
				// FIXME: why?
				glAlphaFunc(GL_GREATER, 0.8f);
				glEnable(GL_ALPHA_TEST);
			}

			if (smfShaderCurGLSL != NULL) {
				smfShaderCurGLSL->Enable();
				smfShaderCurGLSL->SetUniform3fv(10, &camera->pos[0]);
				smfShaderCurGLSL->SetUniformMatrix4fv(12, false, shadowHandler->shadowMatrix);
				smfShaderCurGLSL->SetUniform4fv(13, &(shadowHandler->GetShadowParams().x));

				// already on the MV stack
				glLoadIdentity();
				lightHandler.Update(smfShaderCurGLSL);
				glMultMatrixf(camera->GetViewMatrix());

				glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());
				glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, smfMap->GetSpecularTexture());
				glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDetailTexture());
				glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, smfMap->GetSplatDistrTexture());
				glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSkyReflectionTextureID());
				glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, smfMap->GetSkyReflectModTexture());
				glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailNormalTexture());
				glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, smfMap->GetLightEmissionTexture());

				// setup for shadow2DProj
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
				glActiveTexture(GL_TEXTURE0);
			} else {
				smfShaderCurrARB->Enable();
				smfShaderCurrARB->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
				smfShaderCurrARB->SetUniform4f(10, 1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 1);
				smfShaderCurrARB->SetUniform4f(12, 1.0f / smfMap->bigTexSize, 1.0f / smfMap->bigTexSize, 0, 1);
				smfShaderCurrARB->SetUniform4f(13, -floor(camera->pos.x * 0.02f), -floor(camera->pos.z * 0.02f), 0, 0);
				smfShaderCurrARB->SetUniform4f(14, 0.02f, 0.02f, 0, 1);
				smfShaderCurrARB->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
				smfShaderCurrARB->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1);
				smfShaderCurrARB->SetUniform4f(11, 0, 0, 0, sky->GetLight()->GetGroundShadowDensity());

				glMatrixMode(GL_MATRIX0_ARB);
				glLoadMatrixf(shadowHandler->shadowMatrix);
				glMatrixMode(GL_MODELVIEW);

				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
				glActiveTexture(GL_TEXTURE0);
			}
		} else {
			if (smfMap->GetDetailTexture()) {
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
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
			glBindTexture(GL_TEXTURE_2D, smfMap->GetShadingTexture());
			glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f, 1.0f, 1.0f, 1.0f); //fixes a nvidia bug with gltexgen
			SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0, 0);

			//! bind the detail texture a 2nd time to increase the details (-> GL_ADD_SIGNED_ARB is limited -0.5 to +0.5)
			//! (also do this after the shading texture cause of color clamping issues)
			if (smfMap->GetDetailTexture()) {
				glActiveTexture(GL_TEXTURE3);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, smfMap->GetDetailTexture());
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
				glActiveTexture(GL_TEXTURE3);
				glDisable(GL_TEXTURE_2D);
			}

			#ifdef DYNWATER_OVERRIDE_VERTEX_PROGRAM
			//! see comment above
			#endif
		}
	}

	glActiveTexture(GL_TEXTURE0);
}


void CSMFGroundDrawer::ResetTextureUnits(bool drawReflection)
{
	if (!useShaders) {
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
	} else {
		glActiveTexture(GL_TEXTURE4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE9); glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
		glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);

		if (smfShaderCurGLSL != NULL) {
			smfShaderCurGLSL->Disable();
		} else {
			if (smfShaderCurrARB != NULL && shadowHandler->shadowsLoaded) {
				smfShaderCurrARB->Disable();
			}
		}
	}

	if (drawReflection) {
		glDisable(GL_ALPHA_TEST);
	}
}


void CSMFGroundDrawer::Update()
{
	if (mapInfo->map.voidWater && (readmap->currMaxHeight < 0.0f)) {
		return;
	}

	groundTextures->DrawUpdate();
}


void CSMFGroundDrawer::IncreaseDetail()
{
	viewRadius += 2;
	LOG("ViewRadius is now %i", viewRadius);
}

void CSMFGroundDrawer::DecreaseDetail()
{
	viewRadius -= 2;
	LOG("ViewRadius is now %i", viewRadius);
}
