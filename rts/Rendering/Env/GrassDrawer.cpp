/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cmath>

#include "GrassDrawer.h"
#include "Game/Camera.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/Wind.h"
#include "System/myMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/UnsyncedRNG.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"

CONFIG(int, GrassDetail).defaultValue(7).minimumValue(0).description("Sets how detailed the engine rendered grass will be on any given map.");

static const float turfSize        = 20.0f;            // single turf size
static const float partTurfSize    = turfSize * 0.6f;  // single turf size
static const int   grassSquareSize = 4;                // mapsquares per grass square
static const int   grassBlockSize  = 4;                // grass squares per grass block
static const int   blockMapSize    = grassSquareSize * grassBlockSize;

static const int   gSSsq = SQUARE_SIZE * grassSquareSize;
static const int   bMSsq = SQUARE_SIZE * blockMapSize;

static UnsyncedRNG rng;


CGrassDrawer::CGrassDrawer()
: grassOff(false)
, grassDL(0)
, grassBladeTex(0)
, farTex(0)
, grassMap(nullptr)
{
	const int detail = configHandler->GetInt("GrassDetail");

	// some ATI drivers crash with grass enabled, default to disabled
	if ((detail == 0) || ((detail == 7) && globalRendering->haveATI)) {
		grassOff = true;
		return;
	}

	if (!GLEW_EXT_framebuffer_blit) {
		grassOff = true;
		return;
	}

	{
		MapBitmapInfo grassbm;
		unsigned char* grassdata = readMap->GetInfoMap("grass", &grassbm);

		if (!grassdata) {
			grassOff = true;
			return;
		}

		if (grassbm.width != gs->mapx / grassSquareSize || grassbm.height != gs->mapy / grassSquareSize) {
			char b[128];
			SNPRINTF(b, sizeof(b), "grass-map has wrong size (%dx%d, should be %dx%d)\n",
				grassbm.width, grassbm.height, gs->mapx / 4, gs->mapy / 4);
			throw std::runtime_error(b);
		}
		const int grassMapSize = gs->mapx * gs->mapy / (grassSquareSize * grassSquareSize);
		grassMap = new unsigned char[grassMapSize];
		memcpy(grassMap, grassdata, grassMapSize);
		readMap->FreeInfoMap("grass", grassdata);
	}

	ChangeDetail(detail);

	blocksX = gs->mapx / grassSquareSize / grassBlockSize;
	blocksY = gs->mapy / grassSquareSize / grassBlockSize;

	rng.Seed(15);
	grassDL = glGenLists(1);
	CreateGrassDispList(grassDL);

	{
		CBitmap grassBladeTexBM;
		if (!grassBladeTexBM.Load(mapInfo->grass.grassBladeTexName)) {
			//! map didn't define a grasstex, so generate one
			grassBladeTexBM.channels = 4;
			grassBladeTexBM.Alloc(256,64);

			for (int a = 0; a < 16; ++a) {
				CreateGrassBladeTex(&grassBladeTexBM.mem[a * 16 * 4]);
			}
		}

		glGenTextures(1, &grassBladeTex);
		glBindTexture(GL_TEXTURE_2D, grassBladeTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, grassBladeTexBM.xsize, grassBladeTexBM.ysize, GL_RGBA, GL_UNSIGNED_BYTE, &grassBladeTexBM.mem[0]);
	}

	CreateFarTex();
	LoadGrassShaders();
	configHandler->NotifyOnChange(this);
}

CGrassDrawer::~CGrassDrawer()
{
	configHandler->RemoveObserver(this);

	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {
			if (grass[y * 32 + x].va)
				delete grass[y * 32 + x].va;
		}
	}

	delete[] grassMap;

	glDeleteLists(grassDL, 1);
	glDeleteTextures(1, &grassBladeTex);
	glDeleteTextures(1, &farTex);

	shaderHandler->ReleaseProgramObjects("[GrassDrawer]");
	grassShaders.clear();
}


void CGrassDrawer::ChangeDetail(int detail) {
	// TODO: get rid of the magic constants
	maxGrassDist = 800 + std::sqrt((float) detail) * 240;
	maxDetailedDist = 146 + detail * 24;
	detailedBlocks = int((maxDetailedDist - 24) / bMSsq) + 1;
	const float detail_lim = std::min(3, detail);
	numTurfs = 3 + int(detail_lim * 0.5f);
	strawPerTurf = 50 + int(std::sqrt(detail_lim) * 10);
}


void CGrassDrawer::ConfigNotify(const std::string& key, const std::string& value) {
	if (key == "GrassDetail") {
		ChangeDetail(std::atoi(value.c_str()));
	}
}


void CGrassDrawer::LoadGrassShaders() {
	#define sh shaderHandler
	grassShaders.resize(GRASS_PROGRAM_LAST, NULL);

	static const std::string shaderNames[GRASS_PROGRAM_LAST] = {
		"grassNearAdvShader",
		"grassDistAdvShader",
		"grassDistDefShader",
		// "grassShadGenShader"
	};
	static const std::string shaderDefines[GRASS_PROGRAM_LAST] = {
		"#define DISTANCE_NEAR\n#define HAVE_SHADOW\n",
		"#define DISTANCE_FAR\n#define HAVE_SHADOW\n",
		"#define DISTANCE_FAR\n",
		// "#define GRASS_SHADOW_GEN\n"
	};

	static const int NUM_UNIFORMS = 11;
	static const std::string uniformNames[NUM_UNIFORMS] = {
		"mapSizePO2",
		"mapSize",
		"texOffset",
		"billboardDirX",
		"billboardDirY",
		"billboardDirZ",
		"shadowMatrix",
		"shadowParams",
		"simFrame",
		"windSpeed",
		"camPos"
	};

	const std::string extraDefs =
		(mapInfo->grass.bladeWaveScale > 0.0f)?
		"#define ANIMATION\n":
		"";

	if (globalRendering->haveGLSL) {
		for (int i = GRASS_PROGRAM_NEAR_SHADOW; i < GRASS_PROGRAM_LAST; i++) {
			grassShaders[i] = sh->CreateProgramObject("[GrassDrawer]", shaderNames[i] + "GLSL", false);
			grassShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/GrassVertProg.glsl", shaderDefines[i] + extraDefs, GL_VERTEX_SHADER));
			grassShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/GrassFragProg.glsl", shaderDefines[i] + extraDefs, GL_FRAGMENT_SHADER));
		}

		for (int i = GRASS_PROGRAM_NEAR_SHADOW; i < GRASS_PROGRAM_LAST; i++) {
			grassShaders[i]->Link();


			for (int j = 0; j < NUM_UNIFORMS; j++) {
				grassShaders[i]->SetUniformLocation(uniformNames[j]);
			}

			grassShaders[i]->SetUniformLocation("shadingTex");
			grassShaders[i]->SetUniformLocation("shadowMap");
			grassShaders[i]->SetUniformLocation("grassShadingTex");
			grassShaders[i]->SetUniformLocation("bladeTex");
			grassShaders[i]->SetUniformLocation("infoMap");

			grassShaders[i]->Enable();
			grassShaders[i]->SetUniform2f(0, 1.0f / (gs->pwr2mapx  * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE));
			grassShaders[i]->SetUniform2f(1, 1.0f / (gs->mapx      * SQUARE_SIZE), 1.0f / (gs->mapy     * SQUARE_SIZE));
			grassShaders[i]->SetUniform1i(NUM_UNIFORMS,     0);
			grassShaders[i]->SetUniform1i(NUM_UNIFORMS + 1, 1);
			grassShaders[i]->SetUniform1i(NUM_UNIFORMS + 2, 2);
			grassShaders[i]->SetUniform1i(NUM_UNIFORMS + 3, 3);
			grassShaders[i]->SetUniform1i(NUM_UNIFORMS + 4, 4);
			grassShaders[i]->Disable();
			grassShaders[i]->Validate();
		}
	}

	#undef sh
}



static const bool GrassSort(const CGrassDrawer::InviewGrass& a, const CGrassDrawer::InviewGrass& b) { return (a.dist > b.dist); }
static const bool GrassSortNear(const CGrassDrawer::InviewNearGrass& a, const CGrassDrawer::InviewNearGrass& b) { return (a.dist > b.dist); }



class CGrassBlockDrawer: public CReadMap::IQuadDrawer
{
public:
	std::vector<CGrassDrawer::InviewGrass> inviewGrass;
	std::vector<CGrassDrawer::InviewNearGrass> inviewNearGrass;
	int cx, cy;
	CGrassDrawer* gd;

	void DrawQuad(int x, int y);
};


void CGrassBlockDrawer::DrawQuad(int x, int y)
{
	const float maxDetailedDist = gd->maxDetailedDist;

	CGrassDrawer::NearGrassStruct* nearGrass = gd->nearGrass;

	if (abs(x - cx) <= gd->detailedBlocks && abs(y - cy) <= gd->detailedBlocks) {
		//! blocks close to the camera
		for (int y2 = y * grassBlockSize; y2 < (y + 1) * grassBlockSize; ++y2) {
			for (int x2 = x * grassBlockSize; x2 < (x + 1) * grassBlockSize; ++x2) {
				if (gd->grassMap[y2 * gs->mapx / grassSquareSize + x2]) {
					float3 squarePos((x2 + 0.5f) * gSSsq, 0.0f, (y2 + 0.5f) * gSSsq);
						squarePos.y = CGround::GetHeightReal(squarePos.x, squarePos.z, false);

					const float sqdist = (camera->GetPos() - squarePos).SqLength();

					CGrassDrawer::NearGrassStruct* ng = &nearGrass[(y2 & 31) * 32 + (x2 & 31)];

					if (sqdist < (maxDetailedDist * maxDetailedDist)) {
						//! close grass, draw directly
						rng.Seed(y2 * 1025 + x2);

						for (int a = 0; a < gd->numTurfs; a++) {
							const float dx = (x2 + rng.RandFloat()) * gSSsq;
							const float dy = (y2 + rng.RandFloat()) * gSSsq;

							float3 pos(dx, CGround::GetHeightReal(dx, dy, false), dy);
								pos.y -= CGround::GetSlope(dx, dy, false) * 10.0f + 0.03f;

							if (ng->square != y2 * 2048 + x2) {
								const float3 v = squarePos - camera->GetPos();
								ng->rotation = GetHeadingFromVector(v.x, v.z) * 180.0f / 32768 + 180; //FIXME make more random
								ng->square = y2 * 2048 + x2;
							}

							glPushMatrix();
							glTranslatef3(pos);
							glRotatef(ng->rotation, 0.0f, 1.0f, 0.0f);
							glCallList(gd->grassDL);
							glPopMatrix();
						}
					} else {
						//! near but not close, save for later drawing
						CGrassDrawer::InviewNearGrass iv;
							iv.dist = sqdist;
							iv.x = x2;
							iv.y = y2;
						inviewNearGrass.push_back(iv);
						ng->square = -1;
					}
				}
			}
		}

		return;
	}

	const float3 dif(camera->GetPos().x - ((x + 0.5f) * bMSsq), 0.0f, camera->GetPos().z - ((y + 0.5f) * bMSsq));
	const float dist = dif.SqLength2D();

	if (dist < Square(gd->maxGrassDist)) {
		const int curSquare = y * gd->blocksX + x;
		const int curModSquare = (y & 31) * 32 + (x & 31);

		CGrassDrawer::GrassStruct* grass = gd->grass + curModSquare;
		grass->lastSeen = globalRendering->drawFrame;

		if (grass->square != curSquare) {
			grass->square = curSquare;

			delete grass->va;
			grass->va = NULL;
		}

		if (!grass->va) {
			grass->va = new CVertexArray;
			grass->pos = float3((x + 0.5f) * bMSsq, CGround::GetHeightReal((x + 0.5f) * bMSsq, (y + 0.5f) * bMSsq, false), (y + 0.5f) * bMSsq);

			CVertexArray* va = grass->va;
			va->Initialize();

			for (int y2 = y * grassBlockSize; y2 < (y + 1) * grassBlockSize; ++y2) {
				for (int x2 = x * grassBlockSize; x2 < (x + 1) * grassBlockSize; ++x2) {
					if (gd->grassMap[y2 * gs->mapx / grassSquareSize + x2]) {
						rng.Seed(y2 * 1025 + x2);

						for (int a = 0; a < gd->numTurfs; a++) {
							const float dx = (x2 + rng.RandFloat()) * gSSsq;
							const float dy = (y2 + rng.RandFloat()) * gSSsq;
							const float col = 1.0f;

							float3 pos(dx, CGround::GetHeightReal(dx, dy, false) + 0.5f, dy);
								pos.y -= (CGround::GetSlope(dx, dy, false) * 10.0f + 0.03f);

							va->AddVertexTN(pos, 0.0f,         0.0f, float3(-partTurfSize, -partTurfSize, col));
							va->AddVertexTN(pos, 1.0f / 16.0f, 0.0f, float3( partTurfSize, -partTurfSize, col));
							va->AddVertexTN(pos, 1.0f / 16.0f, 1.0f, float3( partTurfSize,  partTurfSize, col));
							va->AddVertexTN(pos, 0.0f,         1.0f, float3(-partTurfSize,  partTurfSize, col));
						}
					}
				}
			}
		}

		CGrassDrawer::InviewGrass ig;
			ig.num = curModSquare;
			ig.dist = dif.Length2D();
		inviewGrass.push_back(ig);
	}
}

void CGrassDrawer::SetupGlStateNear()
{
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	const float3 windSpeed =
		wind.GetCurrentDirection() *
		wind.GetCurrentStrength() *
		mapInfo->grass.bladeWaveScale;

	if (shadowHandler->shadowsLoaded && globalRendering->haveGLSL) {
		grassShader = grassShaders[GRASS_PROGRAM_NEAR_SHADOW];
		grassShader->SetFlag("HAVE_INFOTEX", gd->DrawExtraTex());
		grassShader->Enable();

		grassShader->SetUniform2f(2, 0.0f, 0.0f);
		grassShader->SetUniformMatrix4fv(6, false, shadowHandler->shadowMatrix);
		grassShader->SetUniform4fv(7, shadowHandler->GetShadowParams());
		grassShader->SetUniform1f(8, gs->frameNum);
		grassShader->SetUniform3fv(9, &windSpeed.x);
		grassShader->SetUniform3fv(10, &camera->GetPos().x);

		glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());

		glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);
			static const float texConstant[] = {
				mapInfo->light.groundAmbientColor.x * 1.24f,
				mapInfo->light.groundAmbientColor.y * 1.24f,
				mapInfo->light.groundAmbientColor.z * 1.24f,
				1.0f
			};
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texConstant);

		glActiveTextureARB(GL_TEXTURE2_ARB);
			glBindTexture(GL_TEXTURE_2D, readMap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
			glBindTexture(GL_TEXTURE_2D, grassBladeTex);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glBindTexture(GL_TEXTURE_2D, gd->GetActiveInfoTexture());
		}

		glActiveTextureARB(GL_TEXTURE0_ARB);

		if (!globalRendering->haveGLSL) {
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE2_ARB);
			glEnable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glEnable(GL_TEXTURE_2D);
			if (gd->DrawExtraTex()) {
				glActiveTextureARB(GL_TEXTURE4_ARB);
				glEnable(GL_TEXTURE_2D);
			}
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glEnable(GL_TEXTURE_2D);
		}

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMultMatrixf(camera->GetViewMatrix());
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	} else {
		glActiveTextureARB(GL_TEXTURE0_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, grassBladeTex);

		glActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, readMap->GetGrassShadingTexture());
			glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
			SetTexGen(1.0f / (gs->mapx * SQUARE_SIZE), 1.0f / (gs->mapy * SQUARE_SIZE), 0.0f, 0.0f);

		glActiveTextureARB(GL_TEXTURE2_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());

		glMultiTexCoord4f(GL_TEXTURE2_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
			SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0.0f, 0.0f);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glEnable(GL_TEXTURE_2D);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

			glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
			SetTexGen(1.0f / (gs->pwr2mapx * SQUARE_SIZE), 1.0f / (gs->pwr2mapy * SQUARE_SIZE), 0.0f, 0.0f);

			glBindTexture(GL_TEXTURE_2D, gd->GetActiveInfoTexture());
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);

	ISky::SetupFog();
}


void CGrassDrawer::ResetGlStateNear()
{
	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	if (shadowHandler->shadowsLoaded && globalRendering->haveGLSL) {
		grassShader->Disable();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		/*glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glActiveTextureARB(GL_TEXTURE0_ARB);*/
	} else {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE3_ARB);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
}


void CGrassDrawer::SetupGlStateFar()
{
	assert(globalRendering->haveGLSL);

	CBaseGroundDrawer* gd = readMap->GetGroundDrawer();

	const float3 windSpeed =
		wind.GetCurrentDirection() *
		wind.GetCurrentStrength() *
		mapInfo->grass.bladeWaveScale;

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.01f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);

	if (shadowHandler->shadowsLoaded) {
		grassShader = grassShaders[GRASS_PROGRAM_DIST_SHADOW];
		grassShader->SetFlag("HAVE_INFOTEX", gd->DrawExtraTex());
		grassShader->Enable();

		grassShader->SetUniformMatrix4fv(6, false, &shadowHandler->shadowMatrix.m[0]);
		grassShader->SetUniform4fv(7, shadowHandler->GetShadowParams());
		grassShader->SetUniform1f(8, gs->frameNum);
		grassShader->SetUniform3fv(9, &windSpeed.x);
		grassShader->SetUniform3fv(10, &camera->GetPos().x);

		glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());

		glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);
			static const float texConstant[] = {
				mapInfo->light.groundAmbientColor.x * 1.24f,
				mapInfo->light.groundAmbientColor.y * 1.24f,
				mapInfo->light.groundAmbientColor.z * 1.24f,
				1.0f
			};
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texConstant);

		glActiveTextureARB(GL_TEXTURE2_ARB);
			glBindTexture(GL_TEXTURE_2D, readMap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
			glBindTexture(GL_TEXTURE_2D, farTex);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glBindTexture(GL_TEXTURE_2D, gd->GetActiveInfoTexture());
		}

		glActiveTextureARB(GL_TEXTURE0_ARB);
	} else {
		grassShader = grassShaders[GRASS_PROGRAM_DIST_BASIC];
		grassShader->Enable();

		grassShader->SetUniform1f(8, gs->frameNum);
		grassShader->SetUniform3fv(9, &windSpeed.x);
		grassShader->SetUniform3fv(10, &camera->GetPos().x);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());

		//glActiveTextureARB(GL_TEXTURE1_ARB);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glBindTexture(GL_TEXTURE_2D, readMap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glBindTexture(GL_TEXTURE_2D, farTex);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glBindTexture(GL_TEXTURE_2D, gd->GetActiveInfoTexture());
		}

		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
}


void CGrassDrawer::ResetGlStateFar()
{
	grassShader->Disable();
	if (shadowHandler->shadowsLoaded) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
	}

	glDepthMask(GL_TRUE);
	glDisable(GL_ALPHA_TEST);

}


void CGrassDrawer::DrawFarBillboards(const std::vector<CGrassDrawer::InviewGrass>& inviewGrass)
{
	for (std::vector<CGrassDrawer::InviewGrass>::const_iterator gi = inviewGrass.begin(); gi != inviewGrass.end(); ++gi) {
		if ((*gi).dist + 128 < maxGrassDist) {
			glColor4f(0.62f, 0.62f, 0.62f, 1.0f);
		} else {
			glColor4f(0.62f, 0.62f, 0.62f, 1.0f - ((*gi).dist + 128 - maxGrassDist) / 128.0f);
		}

		grass[(*gi).num].va->DrawArrayTN(GL_QUADS);
	}
}


void CGrassDrawer::DrawNearBillboards(const std::vector<InviewNearGrass>& inviewNearGrass)
{
	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(inviewNearGrass.size() * numTurfs * 4, 0, VA_SIZE_TN);

	for (std::vector<InviewNearGrass>::const_iterator gi = inviewNearGrass.begin(); gi != inviewNearGrass.end(); ++gi) {
		const int x = (*gi).x;
		const int y = (*gi).y;

		rng.Seed(y * 1025 + x);

		for (int a = 0; a < numTurfs; a++) {
			const float dx = (x + rng.RandFloat()) * gSSsq;
			const float dy = (y + rng.RandFloat()) * gSSsq;
			const float col = 1.0f;

			float3 pos(dx, CGround::GetHeightReal(dx, dy, false) + 0.5f, dy);
				pos.y -= (CGround::GetSlope(dx, dy, false) * 10.0f + 0.03f);

			va->AddVertexQTN(pos,         0.0f, 0.0f, float3(-partTurfSize, -partTurfSize, col));
			va->AddVertexQTN(pos, 1.0f / 16.0f, 0.0f, float3( partTurfSize, -partTurfSize, col));
			va->AddVertexQTN(pos, 1.0f / 16.0f, 1.0f, float3( partTurfSize,  partTurfSize, col));
			va->AddVertexQTN(pos,         0.0f, 1.0f, float3(-partTurfSize,  partTurfSize, col));
		}
	}

	va->DrawArrayTN(GL_QUADS);
}


void CGrassDrawer::Draw()
{
	if (grassOff || !readMap->GetGrassShadingTexture())
		return;

	glPushAttrib(GL_CURRENT_BIT);
	glColor4f(0.62f, 0.62f, 0.62f, 1.0f);

	SetupGlStateNear();
		static CGrassBlockDrawer drawer;
			drawer.cx = int(camera->GetPos().x / bMSsq);
			drawer.cy = int(camera->GetPos().z / bMSsq);
			drawer.inviewGrass.clear();
			drawer.inviewNearGrass.clear();
			drawer.gd = this;

		readMap->GridVisibility(camera, blockMapSize, maxGrassDist, &drawer);
	ResetGlStateNear();

	if (
		globalRendering->haveGLSL
		&& (!shadowHandler->shadowsLoaded || !globalRendering->atiHacks) // Ati crashes w/o an error when shadows are enabled!?
	) {
		SetupGlStateFar();
			std::sort(drawer.inviewGrass.begin(), drawer.inviewGrass.end(), GrassSort);
			std::sort(drawer.inviewNearGrass.begin(), drawer.inviewNearGrass.end(), GrassSortNear);

			glColor4f(0.62f, 0.62f, 0.62f, 1.0f);
			DrawFarBillboards(drawer.inviewGrass);

			glColor4f(0.62f, 0.62f, 0.62f, 1.0f);
			DrawNearBillboards(drawer.inviewNearGrass);
		ResetGlStateFar();
	}

	glPopAttrib();
	GarbageCollect();
}


void CGrassDrawer::DrawShadow()
{
	// Grass self-shadowing doesn't look that good atm
	/*if (grassOff || !readMap->GetGrassShadingTexture())
		return;

	const float3 windSpeed =
		wind.GetCurrentDirection() *
		wind.GetCurrentStrength() *
		mapInfo->grass.bladeWaveScale;

	grassShader = grassShaders[GRASS_PROGRAM_SHADOW_GEN];
	grassShader->Enable();
		grassShader->SetUniform2f(2, 0.0f, 0.0f);
		grassShader->SetUniformMatrix4fv(6, false, shadowHandler->shadowMatrix);
		grassShader->SetUniform4fv(7, shadowHandler->GetShadowParams());
		grassShader->SetUniform1f(8, gs->frameNum);
		grassShader->SetUniform3fv(9, &windSpeed.x);
		grassShader->SetUniform3fv(10, &camera->GetPos().x);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, activeFarTex);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonOffset(5, 15);
	glEnable(GL_POLYGON_OFFSET_FILL);

	static CGrassBlockDrawer drawer;
		drawer.cx = int(camera->GetPos().x / bMSsq);
		drawer.cy = int(camera->GetPos().z / bMSsq);
		drawer.inviewGrass.clear();
		drawer.inviewNearGrass.clear();
		drawer.gd = this;
	readMap->GridVisibility(camera, blockMapSize, maxGrassDist * 10.0f, &drawer);

	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);

	grassShader->Disable();
	*/
}


void CGrassDrawer::GarbageCollect()
{
	for (int i = 0; i < 32*32; ++i) {
		GrassStruct* pGS = &grass[i];
		if ((pGS->lastSeen < globalRendering->drawFrame - 50) && pGS->va) {
			delete pGS->va;
			pGS->va = nullptr;
		}
	}
}


void CGrassDrawer::ResetPos(const float3& pos)
{
	if (grassOff)
		return;

	const int idx =
		(int(pos.z / bMSsq) & 31) * 32 +
		(int(pos.x / bMSsq) & 31);

	if (grass[idx].va) {
		delete grass[idx].va;
		grass[idx].va = nullptr;
	}

	grass[idx].square = -1;
}


void CGrassDrawer::CreateGrassDispList(int listNum)
{
	CVertexArray* va = GetVertexArray();
	va->Initialize();

	for (int a = 0; a < strawPerTurf; ++a) {
		const float maxAng = mapInfo->grass.bladeAngle * rng.RandFloat();
		const float length = mapInfo->grass.bladeHeight + mapInfo->grass.bladeHeight * rng.RandFloat();

		float3 sideVect(rng.RandFloat() - 0.5f, 0.0f, rng.RandFloat() - 0.5f);
		sideVect.ANormalize();
		float3 forwardVect = sideVect.cross(UpVector);
		sideVect *= mapInfo->grass.bladeWidth;

		float3 basePos(30.0f, 0.0f, 30.0f);
		while (basePos.SqLength2D() > (turfSize * turfSize / 4)) {
			basePos  = float3(rng.RandFloat() - 0.5f, 0.f, rng.RandFloat() - 0.5f) * turfSize;
		}

		const float xtexCoord = int(14.9999f * rng.RandFloat()) / 16.0f;
		const int numSections = 1 + int(maxAng * 5.0f);

		// draw single blade
		for (float h = 0; h <= 1.0f; h += (1.0f / numSections)) {
			const float ang = maxAng * h;

			const float3 edgePos  = (UpVector * std::cos(ang) + forwardVect * std::sin(ang)) * length * h;
			const float3 edgePosL = edgePos - sideVect * (1.0f - h);
			const float3 edgePosR = edgePos + sideVect * (1.0f - h);

			if (h == 0.0f) {
				// start with a degenerated triangle
				va->AddVertexT(basePos + edgePosR - float3(0.0f, 0.1f, 0.0f), xtexCoord, h);
				va->AddVertexT(basePos + edgePosR - float3(0.0f, 0.1f, 0.0f), xtexCoord, h);
			} else {
				va->AddVertexT(basePos + edgePosR, xtexCoord, h);
			}

			va->AddVertexT(basePos + edgePosL, xtexCoord + (1.0f / 16), h);
		}

		// end with a degenerated triangle
		// -> this way we can render multiple blades in a single GL_TRIANGLE_STRIP
		const float3 edgePos = (UpVector * std::cos(maxAng) + forwardVect * std::sin(maxAng)) * length;
		va->AddVertexT(basePos + edgePos, xtexCoord + (1.0f / 32), 1.0f);
		va->AddVertexT(basePos + edgePos, xtexCoord + (1.0f / 32), 1.0f);
	}

	glNewList(listNum, GL_COMPILE);
	va->DrawArrayT(GL_TRIANGLE_STRIP);
	glEndList();
}

void CGrassDrawer::CreateGrassBladeTex(unsigned char* buf)
{
	float3 col( mapInfo->grass.color + float3(0.11f * rng.RandFloat(), 0.08f * rng.RandFloat(), 0.11f * rng.RandFloat()) );
	col.x = Clamp(col.x, 0.f, 1.f);
	col.y = Clamp(col.y, 0.f, 1.f);
	col.z = Clamp(col.z, 0.f, 1.f);

	SColor* img = reinterpret_cast<SColor*>(buf);
	for(int y=0;y<64;++y){
		for(int x=0;x<16;++x){
			const float brightness = (0.4f + 0.6f * (y/63.0f));
			img[y*256+x] = SColor(col.r * brightness, col.g * brightness, col.b * brightness, 1.0f);
		}
	}
}

void CGrassDrawer::CreateFarTex()
{
	int sizeMod=2;

	glGenTextures(1, &farTex);
	glBindTexture(GL_TEXTURE_2D, farTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSpringTexStorage2D(GL_TEXTURE_2D, -1, GL_RGBA8, 1024, 64);

	FBO fboTex;
	fboTex.Bind();
	fboTex.AttachTexture(farTex);
	fboTex.CheckStatus("GRASSDRAWER1");

	FBO fbo;
	fbo.Bind();
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, 1024 * sizeMod, 64 * sizeMod);
	fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 1024 * sizeMod, 64 * sizeMod);
	fbo.CheckStatus("GRASSDRAWER2");

	if (!fboTex.IsValid() || !fbo.IsValid()) {
		grassOff = true;
		return;
	}

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D, grassBladeTex);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);

	glViewport(0,0,1024*sizeMod, 64*sizeMod);
	glClearColor(0.75f,0.9f,0.75f,0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.f,0.f,0.f,0.f);

	// render turf from different vertical angles
	for (int a=0;a<16;++a) {
		glViewport(a*64*sizeMod, 0, 64*sizeMod, 64*sizeMod);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef((a-1)*90/15.0f,1,0,0);
		glTranslatef(0,-0.5f,0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-partTurfSize, partTurfSize, -partTurfSize, partTurfSize, -turfSize, turfSize);

		glCallList(grassDL);
	}

	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// scale down the rendered fartextures (MSAA) and write to the final texture
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER, fbo.fboId);
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, fboTex.fboId);
	glBlitFramebufferEXT(0, 0, 1024*sizeMod, 64*sizeMod,
		0, 0, 1024, 64,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);

	fbo.Unbind();

	// compute mipmaps
	glBindTexture(GL_TEXTURE_2D, farTex);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void CGrassDrawer::AddGrass(const float3& pos)
{
	if (grassOff)
		return;

	const int x = int(pos.x) / (SQUARE_SIZE * grassSquareSize);
	const int z = int(pos.z) / (SQUARE_SIZE * grassSquareSize);
	assert(x >= 0 && x < (gs->mapx - 1) / grassSquareSize);
	assert(z >= 0 && z < (gs->mapy - 1) / grassSquareSize);

	grassMap[z * gs->mapx / grassSquareSize + x] = 1;
}

void CGrassDrawer::RemoveGrass(const float3& pos)
{
	if (grassOff)
		return;

	const int x = int(pos.x) / (SQUARE_SIZE * grassSquareSize);
	const int z = int(pos.z) / (SQUARE_SIZE * grassSquareSize);
	assert(x >= 0 && x <= (gs->mapx - 1) / grassSquareSize);
	assert(z >= 0 && z <= (gs->mapy - 1) / grassSquareSize);

	grassMap[z * gs->mapx / grassSquareSize + x] = 0;
	ResetPos(pos);
}
