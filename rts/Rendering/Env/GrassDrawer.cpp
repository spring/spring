/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

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
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"

CONFIG(int, GrassDetail).defaultValue(7);

static const float turfSize        = 20.0f;            // single turf size
static const float partTurfSize    = turfSize * 0.6f;  // single turf size
static const int   grassSquareSize = 4;                // mapsquares per grass square
static const int   grassBlockSize  = 4;                // grass squares per grass block
static const int   blockMapSize    = grassSquareSize * grassBlockSize;

static const int   gSSsq = SQUARE_SIZE * grassSquareSize;
static const int   bMSsq = SQUARE_SIZE * blockMapSize;


inline float fRand(float size)
{
	return float(rand()) / RAND_MAX * size;
}

CGrassDrawer::CGrassDrawer()
{
	const int detail = configHandler->GetInt("GrassDetail");

	if (detail == 0) {
		grassOff = true;
		return;
	} else {
		grassOff = false;
	}

	MapBitmapInfo grassbm;
	unsigned char* grassdata = readmap->GetInfoMap("grass", &grassbm);

	if (grassdata) {
		if (grassbm.width != gs->mapx / grassSquareSize || grassbm.height != gs->mapy / grassSquareSize) {
			char b[128];
			SNPRINTF(b, sizeof(b), "grass-map has wrong size (%dx%d, should be %dx%d)\n",
				grassbm.width, grassbm.height, gs->mapx / 4, gs->mapy / 4);
			throw std::runtime_error(b);
		}

		const int grassMapSize = gs->mapx * gs->mapy / (grassSquareSize * grassSquareSize);
		grassMap = new unsigned char[grassMapSize];

		memcpy(grassMap, grassdata, grassMapSize);
		readmap->FreeInfoMap("grass", grassdata);
	} else {
		grassOff = true;
		return;
	}

	// TODO: get rid of the magic constants
	maxGrassDist = 800 + math::sqrt((float) detail) * 240;
	maxDetailedDist = 146 + detail * 24;
	detailedBlocks = int((maxDetailedDist - 24) / bMSsq) + 1;
	const float detail_lim = std::min(3, detail);
	numTurfs = 3 + int(detail_lim * 0.5f);
	strawPerTurf = 50 + int(math::sqrt(detail_lim) * 10);

	blocksX = gs->mapx / grassSquareSize / grassBlockSize;
	blocksY = gs->mapy / grassSquareSize / grassBlockSize;

	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {
			grass[y * 32 + x].va = 0;
			grass[y * 32 + x].lastSeen = 0;
			grass[y * 32 + x].pos = ZeroVector;
			grass[y * 32 + x].square = 0;

			nearGrass[y * 32 + x].square = -1;
		}
	}

	lastListClean = 0;
	grassDL = glGenLists(1);
	srand(15);
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
}

CGrassDrawer::~CGrassDrawer()
{
	if (grassOff)
		return;

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
	CVertexArray* va;
	int cx, cy;
	CGrassDrawer* gd;

	void DrawQuad(int x, int y);
};


void CGrassBlockDrawer::DrawQuad(int x, int y)
{
	const float maxDetailedDist = gd->maxDetailedDist;

	int xgbs = x * grassBlockSize, xgbsx = 0;
	int ygbs = y * grassBlockSize, ygbsy = 0;

	CGrassDrawer::NearGrassStruct* nearGrass = gd->nearGrass;

	if (abs(x - cx) <= gd->detailedBlocks && abs(y - cy) <= gd->detailedBlocks) {
		//! blocks close to the camera
		ygbsy = ygbs;

		for (int y2 = 0; y2 < grassBlockSize; ++y2) {
			xgbsx = xgbs;

			const unsigned char* gm = gd->grassMap +
				ygbsy * gs->mapx / grassSquareSize +
				xgbsx;

			for (int x2 = 0; x2 < grassBlockSize; ++x2) { //!loop over all squares in block
				if (*gm) {
					float3 squarePos((xgbsx + 0.5f) * gSSsq, 0.0f, (ygbsy + 0.5f) * gSSsq);
						squarePos.y = ground->GetHeightReal(squarePos.x, squarePos.z, false);

					const float sqdist = (camera->pos - squarePos).SqLength();

					if (sqdist < (maxDetailedDist * maxDetailedDist)) {
						//! close grass, draw directly
						srand(ygbsy * 1025 + xgbsx);
						rand();
						rand();

						for (int a = 0; a < gd->numTurfs; a++) {
							const float dx = (xgbsx + fRand(1)) * gSSsq;
							const float dy = (ygbsy + fRand(1)) * gSSsq;
							const float col = 0.62f;

							float3 pos(dx, ground->GetHeightReal(dx, dy, false), dy);
								pos.y -= ground->GetSlope(dx, dy, false) * 10.0f + 0.03f;

							glColor3f(col, col, col);

							glPushMatrix();
							glTranslatef3(pos);
							CGrassDrawer::NearGrassStruct* ng = &nearGrass[(ygbsy & 31) * 32 + (xgbsx & 31)];

							if (ng->square != ygbsy * 2048 + xgbsx) {
								const float3 v = squarePos - camera->pos;
								ng->rotation = GetHeadingFromVector(v.x, v.z) * 180.0f / 32768 + 180; //FIXME make more random
								ng->square = ygbsy * 2048 + xgbsx;
							}

							glRotatef(ng->rotation, 0.0f, 1.0f, 0.0f);
							glCallList(gd->grassDL);
							glPopMatrix();
						}
					} else {
						//! near but not close, save for later drawing
						CGrassDrawer::InviewNearGrass iv;
							iv.dist = sqdist;
							iv.x = xgbsx;
							iv.y = ygbsy;
						inviewNearGrass.push_back(iv);
						nearGrass[(ygbsy & 31) * 32 + (xgbsx & 31)].square = -1;
					}

				}

				++gm;
				++xgbsx;
			}

			++ygbsy;
		}

		return;
	}

	float3 dif;
		dif.x = camera->pos.x - ((x + 0.5f) * bMSsq);
		dif.y = 0.0f;
		dif.z = camera->pos.z - ((y + 0.5f) * bMSsq);
	const float dist = dif.Length2D();
	dif /= dist;

	if (dist < gd->maxGrassDist) {
		int curSquare = y * gd->blocksX + x;
		int curModSquare = (y & 31) * 32 + (x & 31);

		CGrassDrawer::GrassStruct* grass = gd->grass + curModSquare;
		grass->lastSeen = gs->frameNum;

		if (grass->square != curSquare) {
			grass->square = curSquare;

			delete grass->va;
			grass->va = NULL;
		}

		if (!grass->va) {
			grass->va = new CVertexArray;
			grass->pos = float3((x + 0.5f) * bMSsq, ground->GetHeightReal((x + 0.5f) * bMSsq, (y + 0.5f) * bMSsq, false), (y + 0.5f) * bMSsq);

			va = grass->va;
			va->Initialize();
			va->EnlargeArrays(grassBlockSize * grassBlockSize * gd->numTurfs * 4, 0, VA_SIZE_TN);

			ygbsy = ygbs;

			for (int y2 = 0; y2 < grassBlockSize; ++y2) {
				//! CAUTION: loop count must match EnlargeArrays above
				xgbsx = xgbs;

				unsigned char* gm = gd->grassMap +
					ygbsy * gs->mapx / grassSquareSize +
					xgbsx;

				for (int x2 = 0; x2 < grassBlockSize; ++x2) {
					//! CAUTION: loop count must match EnlargeArrays above
					if (*gm) {
						srand(ygbsy * 1025 + xgbsx);
						rand();
						rand();

						for (int a = 0; a < gd->numTurfs; a++) {
							//! CAUTION: loop count must match EnlargeArrays above
							const float dx = (xgbsx + fRand(1)) * gSSsq;
							const float dy = (ygbsy + fRand(1)) * gSSsq;
							const float col = 1.0f;

							float3 pos(dx, ground->GetHeightReal(dx, dy, false) + 0.5f, dy);
								pos.y -= (ground->GetSlope(dx, dy, false) * 10.0f + 0.03f);

							va->AddVertexQTN(pos, 0.0f,         0.0f, float3(-partTurfSize, -partTurfSize, col));
							va->AddVertexQTN(pos, 1.0f / 16.0f, 0.0f, float3( partTurfSize, -partTurfSize, col));
							va->AddVertexQTN(pos, 1.0f / 16.0f, 1.0f, float3( partTurfSize,  partTurfSize, col));
							va->AddVertexQTN(pos, 0.0f,         1.0f, float3(-partTurfSize,  partTurfSize, col));
						}
					}

					++gm;
					++xgbsx;
				}

				++ygbsy;
			}
		}

		CGrassDrawer::InviewGrass ig;
			ig.num = curModSquare;
			ig.dist = dist;
		inviewGrass.push_back(ig);
	}
}

void CGrassDrawer::SetupGlStateNear()
{
	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

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
		grassShader->SetUniform3fv(10, &camera->pos.x);

		glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());

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
			glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
			glBindTexture(GL_TEXTURE_2D, grassBladeTex);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glBindTexture(GL_TEXTURE_2D, gd->infoTex);
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
			glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());
			glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
			SetTexGen(1.0f / (gs->mapx * SQUARE_SIZE), 1.0f / (gs->mapy * SQUARE_SIZE), 0.0f, 0.0f);

		glActiveTextureARB(GL_TEXTURE2_ARB);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());

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

			glBindTexture(GL_TEXTURE_2D, gd->infoTex);
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
	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

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

	CBaseGroundDrawer* gd = readmap->GetGroundDrawer();

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
		grassShader->Enable();
		grassShader->SetFlag("HAVE_INFOTEX", gd->DrawExtraTex());

		grassShader->SetUniformMatrix4fv(6, false, &shadowHandler->shadowMatrix.m[0]);
		grassShader->SetUniform4fv(7, shadowHandler->GetShadowParams());
		grassShader->SetUniform1f(8, gs->frameNum);
		grassShader->SetUniform3fv(9, &windSpeed.x);
		grassShader->SetUniform3fv(10, &camera->pos.x);

		glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());

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
			glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
			glBindTexture(GL_TEXTURE_2D, farTex);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glBindTexture(GL_TEXTURE_2D, gd->infoTex);
		}

		glActiveTextureARB(GL_TEXTURE0_ARB);
	} else {
		grassShader = grassShaders[GRASS_PROGRAM_DIST_BASIC];
		grassShader->Enable();

		grassShader->SetUniform1f(8, gs->frameNum);
		grassShader->SetUniform3fv(9, &windSpeed.x);
		grassShader->SetUniform3fv(10, &camera->pos.x);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());

		//glActiveTextureARB(GL_TEXTURE1_ARB);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glBindTexture(GL_TEXTURE_2D, readmap->GetGrassShadingTexture());

		glActiveTextureARB(GL_TEXTURE3_ARB);
		glBindTexture(GL_TEXTURE_2D, farTex);

		if (gd->DrawExtraTex()) {
			glActiveTextureARB(GL_TEXTURE4_ARB);
			glBindTexture(GL_TEXTURE_2D, gd->infoTex);
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
	const float grassDistance = maxGrassDist;

	for (std::vector<CGrassDrawer::InviewGrass>::const_iterator gi = inviewGrass.begin(); gi != inviewGrass.end(); ++gi) {
		if ((*gi).dist + 128 < grassDistance) {
			glColor4f(0.62f, 0.62f, 0.62f, 1.0f);
		} else {
			glColor4f(0.62f, 0.62f, 0.62f, 1.0f - ((*gi).dist + 128 - grassDistance) / 128.0f);
		}

		const float3 billboardDirZ = (grass[(*gi).num].pos - camera->pos).ANormalize();
		const float3 billboardDirX = (billboardDirZ.cross(UpVector)).ANormalize();
		const float3 billboardDirY = billboardDirX.cross(billboardDirZ);

		const float ang = math::acos(billboardDirZ.y);
		const int texPart = std::min(15, int(std::max(0, int((ang + PI / 16 - PI / 2) / PI * 30))));

		grassShader->SetUniform2f(2, texPart / 16.0f, 0.0f);
		grassShader->SetUniform3f(3,  billboardDirX.x,  billboardDirX.y,  billboardDirX.z);
		grassShader->SetUniform3f(4,  billboardDirY.x,  billboardDirY.y,  billboardDirY.z);
		grassShader->SetUniform3f(5, -billboardDirZ.x, -billboardDirZ.y, -billboardDirZ.z);

		grass[(*gi).num].va->DrawArrayTN(GL_QUADS);
	}
}


void CGrassDrawer::DrawNearBillboards(const std::vector<InviewNearGrass>& inviewNearGrass)
{
	for (std::vector<InviewNearGrass>::const_iterator gi = inviewNearGrass.begin(); gi != inviewNearGrass.end(); ++gi) {
		const int x = (*gi).x;
		const int y = (*gi).y;

		if (grassMap[y * gs->mapx / grassSquareSize + x]) {
			float3 squarePos((x + 0.5f) * gSSsq, 0.0f, (y + 0.5f) * gSSsq);
				squarePos.y = ground->GetHeightReal(squarePos.x, squarePos.z, false);
			const float3 billboardDirZ = (squarePos - camera->pos).ANormalize();
			const float3 billboardDirX = (billboardDirZ.cross(UpVector)).ANormalize();
			const float3 billboardDirY = billboardDirX.cross(billboardDirZ);

			const float ang = math::acos(billboardDirZ.y);
			const int texPart = std::min(15, int(std::max(0, int((ang + PI / 16 - PI / 2) / PI * 30))));

			grassShader->SetUniform2f(2, texPart / 16.0f, 0.0f);
			grassShader->SetUniform3f(3,  billboardDirX.x,  billboardDirX.y,  billboardDirX.z);
			grassShader->SetUniform3f(4,  billboardDirY.x,  billboardDirY.y,  billboardDirY.z);
			grassShader->SetUniform3f(5, -billboardDirZ.x, -billboardDirZ.y, -billboardDirZ.z);

			srand(y * 1025 + x);
			rand();
			rand();

			CVertexArray* va = GetVertexArray();
			va->Initialize();
			va->EnlargeArrays(numTurfs * 4, 0, VA_SIZE_TN);

			for (int a = 0; a < numTurfs; a++) {
				//! CAUTION: loop count must match EnlargeArrays above
				const float dx = (x + fRand(1)) * gSSsq;
				const float dy = (y + fRand(1)) * gSSsq;
				const float col = 1.0f;

				float3 pos(dx, ground->GetHeightReal(dx, dy, false) + 0.5f, dy);
					pos.y -= (ground->GetSlope(dx, dy, false) * 10.0f + 0.03f);

				va->AddVertexQTN(pos,         0.0f, 0.0f, float3(-partTurfSize, -partTurfSize, col));
				va->AddVertexQTN(pos, 1.0f / 16.0f, 0.0f, float3( partTurfSize, -partTurfSize, col));
				va->AddVertexQTN(pos, 1.0f / 16.0f, 1.0f, float3( partTurfSize,  partTurfSize, col));
				va->AddVertexQTN(pos,         0.0f, 1.0f, float3(-partTurfSize,  partTurfSize, col));
			}

			va->DrawArrayTN(GL_QUADS);
		}
	}
}


void CGrassDrawer::Draw()
{
	if (grassOff || !readmap->GetGrassShadingTexture())
		return;

	glPushAttrib(GL_CURRENT_BIT);
	glColor4f(0.62f, 0.62f, 0.62f, 1.0f);

	SetupGlStateNear();
		GML_RECMUTEX_LOCK(grass); // Draw
		static CGrassBlockDrawer drawer;
			drawer.cx = int(camera->pos.x / bMSsq);
			drawer.cy = int(camera->pos.z / bMSsq);
			drawer.inviewGrass.clear();
			drawer.inviewNearGrass.clear();
			drawer.gd = this;

		readmap->GridVisibility(camera, blockMapSize, maxGrassDist, &drawer);
	ResetGlStateNear();

	if (globalRendering->haveGLSL) {
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
	/*if (grassOff || !readmap->GetGrassShadingTexture())
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
		grassShader->SetUniform3fv(10, &camera->pos.x);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, activeFarTex);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonOffset(5, 15);
	glEnable(GL_POLYGON_OFFSET_FILL);

	GML_RECMUTEX_LOCK(grass); // Draw
	static CGrassBlockDrawer drawer;
		drawer.cx = int(camera->pos.x / bMSsq);
		drawer.cy = int(camera->pos.z / bMSsq);
		drawer.inviewGrass.clear();
		drawer.inviewNearGrass.clear();
		drawer.gd = this;
	readmap->GridVisibility(camera, blockMapSize, maxGrassDist * 10.0f, &drawer);

	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);

	grassShader->Disable();
	*/
}


void CGrassDrawer::GarbageCollect()
{
	const int startClean = (lastListClean * 20) % (32 * 32);
	const int endClean = (gs->frameNum * 20) % (32 * 32);

	if (startClean > endClean) {
		for (GrassStruct* pGS = grass + startClean; pGS < grass + 32 * 32; ++pGS) {
			if ((pGS->lastSeen < gs->frameNum - 50) && pGS->va) {
				delete pGS->va;
				pGS->va = 0;
			}
		}
		for (GrassStruct* pGS = grass; pGS < grass + endClean; ++pGS) {
			if ((pGS->lastSeen < gs->frameNum - 50) && pGS->va) {
				delete pGS->va;
				pGS->va = 0;
			}
		}
	} else {
		for (GrassStruct* pGS = grass + startClean; pGS < grass + endClean; ++pGS) {
			if ((pGS->lastSeen < gs->frameNum - 50) && pGS->va) {
				delete pGS->va;
				pGS->va = 0;
			}
		}
	}

	lastListClean = gs->frameNum;
}


void CGrassDrawer::ResetPos(const float3& pos)
{
	if (grassOff)
		return;

	GML_RECMUTEX_LOCK(grass); // ResetPos

	const int idx =
		(int(pos.z / bMSsq) & 31) * 32 +
		(int(pos.x / bMSsq) & 31);

	if (grass[idx].va) {
		delete grass[idx].va;
		grass[idx].va = 0;
	}

	grass[idx].square = -1;
}


void CGrassDrawer::CreateGrassDispList(int listNum)
{
	CVertexArray* va = GetVertexArray();
	va->Initialize();

	for (int a = 0; a < strawPerTurf; ++a) {
		const float maxAng = fRand(mapInfo->grass.bladeAngle);
		const float length = mapInfo->grass.bladeHeight + fRand(mapInfo->grass.bladeHeight);

		float3 sideVect(fRand(1.0f) - 0.5f, 0.0f, fRand(1.0f) - 0.5f);
		sideVect.ANormalize();
		float3 forwardVect = sideVect.cross(UpVector);
		sideVect *= mapInfo->grass.bladeWidth;

		const float3 cornerPos = (UpVector * math::cos(maxAng) + forwardVect * math::sin(maxAng)) * length;
		float3 basePos(30.0f, 0.0f, 30.0f);

		while (basePos.SqLength2D() > (turfSize * turfSize / 4)) {
			basePos = float3(fRand(turfSize) - turfSize * 0.5f, 0.0f, fRand(turfSize) - turfSize * 0.5f);
		}

		const int xtexOffset = int(fRand(15.9999f));
		const float xtexBase = xtexOffset * (1.0f / 16.0f);
		const int numSections = 1 + int(maxAng * 5.0f);

		for (int b = 0; b < numSections; ++b) {
			const float h = b * (1.0f / numSections);
			const float ang = maxAng * h;

			const float3 edgePosL =
				-sideVect * (1 - h) +
				(UpVector * math::cos(ang) + forwardVect * math::sin(ang)) * length * h;
			const float3 edgePosR =
				sideVect * (1.0f - h) +
				(UpVector * math::cos(ang) + forwardVect * math::sin(ang)) * length * h;

			if (b == 0) {
				va->AddVertexT(basePos + (edgePosR - float3(0.0f, 0.1f, 0.0f)), xtexBase + xtexOffset, h);
				va->AddVertexT(basePos + (edgePosR - float3(0.0f, 0.1f, 0.0f)), xtexBase + xtexOffset, h);
			} else {
				va->AddVertexT((basePos + edgePosR), xtexBase + xtexOffset, h);
			}

			va->AddVertexT((basePos + edgePosL), xtexBase + (1.0f / 16) + xtexOffset, h);
		}

		va->AddVertexT(basePos + cornerPos, xtexBase + xtexOffset + (1.0f / 32), 1.0f);
		va->AddVertexT(basePos + cornerPos, xtexBase + xtexOffset + (1.0f / 32), 1.0f);
	}

	glNewList(listNum, GL_COMPILE);
	va->DrawArrayT(GL_TRIANGLE_STRIP);
	glEndList();
}

void CGrassDrawer::CreateGrassBladeTex(unsigned char* buf)
{
	float3 col( mapInfo->grass.color + float3(fRand(0.11f),fRand(0.08f),fRand(0.11f)) );
	col.x = Clamp(col.x, 0.f, 1.f);
	col.y = Clamp(col.y, 0.f, 1.f);
	col.z = Clamp(col.z, 0.f, 1.f);

	for(int y=0;y<64;++y){
		for(int x=0;x<16;++x){
			const float brightness = (0.4f + 0.6f * (y/64.0f)) * 255.f;
			buf[(y*256+x)*4+0] = (unsigned char)(col.x * brightness);
			buf[(y*256+x)*4+1] = (unsigned char)(col.y * brightness);
			buf[(y*256+x)*4+2] = (unsigned char)(col.z * brightness);
			buf[(y*256+x)*4+3] = 1;
		}
	}
}

void CGrassDrawer::CreateFarTex()
{
	int sizeMod=2;
	unsigned char* buf=new unsigned char[64*sizeMod*1024*sizeMod*4];
	unsigned char* buf2=new unsigned char[256*sizeMod*256*sizeMod*4];
	memset(buf,0,64*sizeMod*1024*sizeMod*4);
	memset(buf2,0,256*sizeMod*256*sizeMod*4);

	FBO fbo;
	fbo.Bind();
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT16, 256*sizeMod, 256*sizeMod);
	fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, 256*sizeMod, 256*sizeMod);
	fbo.CheckStatus("GRASSDRAWER");

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBindTexture(GL_TEXTURE_2D, grassBladeTex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glColor4f(1,1,1,1);
	glViewport(0,0,256*sizeMod,256*sizeMod);

	for(int a=0;a<16;++a){
		glClearColor(0.0f,0.0f,0.0f,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef((a-1)*90/15.0f,1,0,0);
		glTranslatef(0,-0.5f,0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-partTurfSize, partTurfSize, -partTurfSize, partTurfSize, -turfSize, turfSize);

		glCallList(grassDL);

		glReadPixels(0,0,256*sizeMod,256*sizeMod,GL_RGBA,GL_UNSIGNED_BYTE,buf2);

//		CBitmap bm(buf2,512,512);
//		bm.Save("ft.bmp");

		int dx=a*64*sizeMod;
		for(int y=0;y<64*sizeMod;++y){
			for(int x=0;x<64*sizeMod;++x){
				float r=0,g=0,b=0,a=0;
				for(int y2=0;y2<4;++y2){
					for(int x2=0;x2<4;++x2){
						if(buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4]==0 && buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+1]==0 && buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+2]==0){
						} else {
							float s=1;
							if(y!=0 && y!=64*sizeMod-1 && (buf2[((y*4+y2)*256*sizeMod+x*4+x2+1)*4+1]==0 || buf2[((y*4+y2)*256*sizeMod+x*4+x2-1)*4+1]==0 || buf2[((y*4+y2+1)*256*sizeMod+x*4+x2)*4+1]==0 || buf2[((y*4+y2-1)*256*sizeMod+x*4+x2)*4+1]==0)){
								s=0.5f;
							}
							r+=s*buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+0];
							g+=s*buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+1];
							b+=s*buf2[((y*4+y2)*256*sizeMod+x*4+x2)*4+2];
							a+=s;
						}
					}
				}
				if(a==0){
					buf[((y)*1024*sizeMod+x+dx)*4+0]=190;
					buf[((y)*1024*sizeMod+x+dx)*4+1]=230;
					buf[((y)*1024*sizeMod+x+dx)*4+2]=190;
					buf[((y)*1024*sizeMod+x+dx)*4+3]=0;
				} else {
					buf[((y)*1024*sizeMod+x+dx)*4+0]=(unsigned char) (r/a);
					buf[((y)*1024*sizeMod+x+dx)*4+1]=(unsigned char) (g/a);
					buf[((y)*1024*sizeMod+x+dx)*4+2]=(unsigned char) (b/a);
					buf[((y)*1024*sizeMod+x+dx)*4+3]=(unsigned char) (std::min((float)255,a*16));
				}
			}
		}
	}

	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	fbo.Unbind();

	glGenTextures(1, &farTex);
	glBindTexture(GL_TEXTURE_2D, farTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, 1024 * sizeMod, 64 * sizeMod, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	delete[] buf;
	delete[] buf2;
}

void CGrassDrawer::AddGrass(const float3& pos)
{
	if (grassOff)
		return;

	GML_RECMUTEX_LOCK(grass); // AddGrass

	const int x = int(pos.x) / SQUARE_SIZE / grassSquareSize;
	const int z = int(pos.z) / SQUARE_SIZE / grassSquareSize;

	grassMap[z * gs->mapx / grassSquareSize + x] = 1;
}

void CGrassDrawer::RemoveGrass(int x, int z)
{
	if (grassOff)
		return;

	GML_RECMUTEX_LOCK(grass); // RemoveGrass

	grassMap[(z / grassSquareSize) * gs->mapx / grassSquareSize + x / grassSquareSize] = 0;
	ResetPos(float3(x * SQUARE_SIZE, 0.0f, z * SQUARE_SIZE));
}
