/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cmath>

#include "GrassDrawer.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/Wind.h"
#include "System/EventHandler.h"
#include "System/GlobalRNG.h"
#include "System/SpringMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Color.h"
#include "System/Exceptions.h"
#include "System/StringUtil.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"

CONFIG(int, GrassDetail).defaultValue(7).headlessValue(0).minimumValue(0).description("Sets how detailed the engine rendered grass will be on any given map.");


// uses a 'synced' RNG s.t. grass turfs generated from the same
// seed also share identical sequences, otherwise an unpleasant
// shimmering effect occurs when zooming
#if 0
typedef CGlobalRNG<LCG16, true> GrassRNG;
#else
typedef CGlobalRNG<PCG32, true> GrassRNG;
#endif

class CGrassBlockDrawer: public CReadMap::IQuadDrawer
{
public:
	void ResetState() override {
		inViewQuads.clear();
		inViewQuads.reserve(32);
	}
	void DrawQuad(int x, int y) override {
		inViewQuads.emplace_back(x, y);
	}

public:
	std::vector<int2> inViewQuads;
};


static constexpr float turfSize        = 20.0f;            // single turf size
static constexpr float partTurfSize    = turfSize * 1.0f;  // single turf size
static constexpr int   grassSquareSize = 4;                // mapsquares per grass square
static constexpr int   grassBlockSize  = 4;                // grass squares per grass block
static constexpr int   blockMapSize    = grassSquareSize * grassBlockSize;

static constexpr int   GSSSQ = SQUARE_SIZE * grassSquareSize;
static constexpr int   BMSSQ = SQUARE_SIZE * blockMapSize;

static GrassRNG drawRNG;
static GrassRNG turfRNG;

static std::array<CMatrix44f, 128> turfMatrices;

static CGrassBlockDrawer blockDrawer;

// managed by WorldDrawer
CGrassDrawer* grassDrawer = nullptr;



CGrassDrawer::CGrassDrawer(): CEventClient("[GrassDrawer]", 199992, false)
{
	const int detail = configHandler->GetInt("GrassDetail");

	if (detail == 0)
		return;

	blockCount.x = mapDims.mapx / grassSquareSize / grassBlockSize;
	blockCount.y = mapDims.mapy / grassSquareSize / grassBlockSize;

	// load grass density from map
	{
		MapBitmapInfo grassbm;
		uint8_t* grassdata = readMap->GetInfoMap("grass", &grassbm);

		if (grassdata == nullptr)
			return;

		if (grassbm.width != mapDims.mapx / grassSquareSize || grassbm.height != mapDims.mapy / grassSquareSize) {
			char b[128];
			SNPRINTF(b, sizeof(b), "grass-map has wrong size (%dx%d, should be %dx%d)\n",
				grassbm.width, grassbm.height, mapDims.mapx / grassSquareSize, mapDims.mapy / grassSquareSize);
			throw std::runtime_error(b);
		}

		// grassBlocks.resize(blockCount.x * blockCount.y);
		grassMap.resize(mapDims.mapx * mapDims.mapy / (grassSquareSize * grassSquareSize));

		memcpy(grassMap.data(), grassdata, grassMap.size());
		readMap->FreeInfoMap("grass", grassdata);
	}

	// create/load blade texture
	{
		CBitmap grassBladeTexBM;
		if (!grassBladeTexBM.Load(mapInfo->grass.bladeTexName)) {
			// map didn't define a grasstex, so generate one
			grassBladeTexBM.Alloc(256, 64, 4);

			for (int a = 0; a < 16; ++a) {
				CreateGrassBladeTex(&grassBladeTexBM.GetRawMem()[a * 16 * 4]);
			}
		}
		// grassBladeTexBM.Save("blade.png", false);
		grassBladeTex = grassBladeTexBM.CreateMipMapTexture();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// ??
		glAttribStatePtr->ViewPort(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	}

	// create shaders and finalize
	ChangeDetail(detail);
	defDrawGrass = LoadGrassShaders();
	configHandler->NotifyOnChange(this, {"GrassDetail"});

	// eventclient
	autoLinkEvents = true;
	RegisterLinkedEvents(this);
	eventHandler.AddClient(this);
}

CGrassDrawer::~CGrassDrawer()
{
	eventHandler.RemoveClient(this);
	configHandler->RemoveObserver(this);

	glDeleteTextures(1, &grassBladeTex);

	grassBuffer.Kill();
	shaderHandler->ReleaseProgramObjects("[GrassDrawer]");
}



float CGrassDrawer::IncrDrawDistance() { return (grassDrawDist = Clamp(grassDrawDist *= 1.25f, 1.0f, CGlobalRendering::MAX_VIEW_RANGE)); }
float CGrassDrawer::DecrDrawDistance() { return (grassDrawDist = Clamp(grassDrawDist *= 0.80f, 1.0f, CGlobalRendering::MAX_VIEW_RANGE)); }

void CGrassDrawer::ChangeDetail(int detail) {
	// TODO: get rid of the magic constants
	const int minDetail = std::min(3, detail);

	grassDrawDist = std::sqrt(detail * 1.0f) * 100.0f;

	// turfs per block
	turfDetail.x = std::min(3 + int(minDetail * 0.5f), int(turfMatrices.size()));
	// straws per turf
	turfDetail.y = std::min(50 + int(std::sqrt(minDetail * 1.0f) * 10), mapInfo->grass.maxStrawsPerTurf);

	// recreate turf geometry
	CreateGrassBuffer();
}

void CGrassDrawer::ConfigNotify(const std::string& key, const std::string& value) {
	ChangeDetail(configHandler->GetInt("GrassDetail"));
}


bool CGrassDrawer::LoadGrassShaders() {
	#define sh shaderHandler
	grassShaders.fill(nullptr);

	static const std::string shaderNames[GRASS_PROGRAM_LAST] = {
		"grassNearAdvShader",
		"grassShadGenShader",
	};

	for (int i = 0; i < GRASS_PROGRAM_CURR; i++) {
		grassShaders[i] = sh->CreateProgramObject("[GrassDrawer]", shaderNames[i] + "GLSL");
		grassShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/GrassVertProg.glsl", "", GL_VERTEX_SHADER));
		grassShaders[i]->AttachShaderObject(sh->CreateShaderObject("GLSL/GrassFragProg.glsl", "", GL_FRAGMENT_SHADER));
		grassShaders[i]->Link();

		grassShaders[i]->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());
		grassShaders[i]->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
		grassShaders[i]->SetFlag("SHADOW_GEN", i == GRASS_PROGRAM_SHADOW);

		// idx 0, not settable as an array via name-based API
		// needs to be named "[0]" or state-copying will fail
		grassShaders[i]->SetUniformLocation("turfMatrices[0]");

		grassShaders[i]->Enable();
		grassShaders[i]->SetUniform("mapSizePO2", 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
		grassShaders[i]->SetUniform("mapSize",    1.0f / (mapDims.mapx     * SQUARE_SIZE), 1.0f / (mapDims.mapy     * SQUARE_SIZE));
		grassShaders[i]->SetUniform("bladeTex",        0);
		grassShaders[i]->SetUniform("grassShadingTex", 1);
		grassShaders[i]->SetUniform("shadingTex",      2);
		grassShaders[i]->SetUniform("infoMap",         3);
		grassShaders[i]->SetUniform("shadowMap",       4);
		grassShaders[i]->SetUniform("infoTexIntensityMul", 1.0f);
		grassShaders[i]->SetUniform("specularExponent", sunLighting->specularExponent);
		grassShaders[i]->SetUniform("groundShadowDensity", sunLighting->groundShadowDensity);
		grassShaders[i]->SetUniform("gammaExponent", globalRendering->gammaExponent);
		grassShaders[i]->SetUniform4v<float>("shadowParams", shadowHandler.GetShadowParams());
		grassShaders[i]->SetUniform4v<float>("fogColor", sky->fogColor);
		grassShaders[i]->SetUniformMatrix4x4<float>("shadowMatrix", false, shadowHandler.GetShadowViewMatrix());
		grassShaders[i]->SetUniformMatrix4x4<float>("viewMatrix", false, camera->GetViewMatrix());
		grassShaders[i]->SetUniformMatrix4x4<float>("projMatrix", false, camera->GetProjectionMatrix());
		grassShaders[i]->Disable();
		grassShaders[i]->Validate();

		if (!grassShaders[i]->IsValid())
			return false;
	}

	#undef sh
	return true;
}

void CGrassDrawer::CreateGrassBuffer()
{
	turfRNG.Seed(15);

	grassBuffer.Kill();
	grassBuffer.Init();

	std::vector<VA_TYPE_TN> verts;
	std::vector<uint32_t> indcs;

	verts.reserve(turfDetail.y * 8 * 4);
	indcs.reserve(turfDetail.y * 8 * 6);

	for (int a = 0; a < turfDetail.y; ++a) {
		// generate a single blade
		const float rndVal = turfRNG.NextFloat();
		const float length = mapInfo->grass.bladeHeight * (1.0f + rndVal);
		const float maxAng = mapInfo->grass.bladeAngle * std::max(turfRNG.NextFloat(), 1.0f - smoothstep(0.0f, 1.0f, rndVal));

		// select one of the 16 color shadings
		const float xtexCoord = turfRNG.NextInt(16) / 16.0f;
		const float heightIncr = 1.0f / (2 + int(maxAng * 1.2f + length * 0.2f));


		const float3 randVect = (turfRNG.NextVector2D() * 0.5f).ANormalize();
		// direction to bend into
		const float3 bendVect = randVect.cross(UpVector);
		const float3 sideVect = randVect * (mapInfo->grass.bladeWidth * (-0.15f * rndVal + 1.0f));

		const float3 normalBend = -bendVect;
		const float3 basePos = turfRNG.NextVector2D() * (turfSize - (bendVect * std::sin(maxAng) * length).Length2D());


		// start at bottom of blade
		VA_TYPE_TN prvVertR = {basePos + sideVect - (UpVector * 3.0f), xtexCoord              , 0.0f, normalBend};
		VA_TYPE_TN prvVertL = {basePos - sideVect - (UpVector * 3.0f), xtexCoord + (1.0f / 16), 0.0f, normalBend};

		for (float h = 0.0f; h < 1.0f; h += heightIncr) {
			const float ang = maxAng * h;

			const float3 n = (normalBend * std::cos(ang) + UpVector * std::sin(ang)).ANormalize();
			const float3 edgePos  = (UpVector * std::cos(ang) + bendVect * std::sin(ang)) * length * h;
			const float3 edgePosR = edgePos + sideVect * (1.0f - h);
			const float3 edgePosL = edgePos - sideVect * (1.0f - h);

			verts.push_back(prvVertR);
			verts.push_back(prvVertL);
			verts.push_back({basePos + edgePosR, xtexCoord + (1.0f / 32) * h              , h, (n + sideVect * 0.04f).ANormalize()});
			verts.push_back({basePos + edgePosL, xtexCoord - (1.0f / 32) * h + (1.0f / 16), h, (n - sideVect * 0.04f).ANormalize()});

			indcs.push_back(verts.size() - 4);
			indcs.push_back(verts.size() - 3);
			indcs.push_back(verts.size() - 2);

			indcs.push_back(verts.size() - 3);
			indcs.push_back(verts.size() - 1);
			indcs.push_back(verts.size() - 2);

			prvVertR = verts[verts.size() - 2];
			prvVertL = verts[verts.size() - 1];
		}

		// blade tip; single triangle
		const float3 tipOffset = (UpVector * std::cos(maxAng) + bendVect * std::sin(maxAng)) * length;
		const float3 normalDir = (normalBend * std::cos(maxAng) + UpVector * std::sin(maxAng)).ANormalize();

		verts.push_back({basePos + tipOffset, xtexCoord + (1.0f / 32), 1.0f, normalDir});

		indcs.push_back(verts.size() - 3);
		indcs.push_back(verts.size() - 2);
		indcs.push_back(verts.size() - 1);
	}

	grassBuffer.UploadTN(verts.size(), indcs.size(),  verts.data(), indcs.data());
}

void CGrassDrawer::CreateGrassBladeTex(uint8_t* buf)
{
	float3 redish = float3(0.95f, 0.70f, 0.4f);
	float3 color = mix(mapInfo->grass.color, redish, 0.1f * turfRNG.NextFloat());

	color.x = Clamp(color.x, 0.0f, 1.0f);
	color.y = Clamp(color.y, 0.0f, 1.0f);
	color.z = Clamp(color.z, 0.0f, 1.0f);

	SColor* img = reinterpret_cast<SColor*>(buf);

	for (int y = 0; y < 64; ++y) {
		for (int x = 0; x < 16; ++x) {
			const float brightness = smoothstep(-0.8f, 0.5f, y/63.0f) + ((x%2) == 0 ? 0.035f : 0.0f);
			const float3 c = color * brightness;

			img[y * 256 + x] = SColor(c.r, c.g, c.b, 1.0f);
		}
	}
}




void CGrassDrawer::EnableShader(const GrassShaderProgram type) {
	const float3 windSpeed = envResHandler.GetCurrentWindVec() * mapInfo->grass.bladeWaveScale;
	const float3 fogParams = {sky->fogStart, sky->fogEnd, camera->GetFarPlaneDist()};

	Shader::IProgramObject* ipo = (grassShaders[GRASS_PROGRAM_CURR] = grassShaders[type]);

	ipo->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());
	ipo->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	ipo->Enable();

	ipo->SetUniform("frame", gs->frameNum + globalRendering->timeOffset);
	ipo->SetUniform3v("windSpeed", &windSpeed.x);
	ipo->SetUniform3v("camPos",    &camera->GetPos().x);
	ipo->SetUniform3v("camDir",    &camera->GetDir().x);
	ipo->SetUniform3v("camUp",     &camera->GetUp().x);
	ipo->SetUniform3v("camRight",  &camera->GetRight().x);

	ipo->SetUniform("infoTexIntensityMul", float(infoTextureHandler->InMetalMode()) + 1.0f);
	ipo->SetUniform("specularExponent"   , sunLighting->specularExponent);
	ipo->SetUniform("groundShadowDensity", sunLighting->groundShadowDensity);
	ipo->SetUniform("gammaExponent"      , globalRendering->gammaExponent);
	ipo->SetUniformMatrix4x4<float>("shadowMatrix", false, shadowHandler.GetShadowViewMatrix());
	ipo->SetUniform4v<float>("shadowParams", shadowHandler.GetShadowParams());

	switch (type) {
		case GRASS_PROGRAM_OPAQUE: {
			ipo->SetUniformMatrix4x4<float>("viewMatrix", false, camera->GetViewMatrix());
			ipo->SetUniformMatrix4x4<float>("projMatrix", false, camera->GetProjectionMatrix());
		} break;
		case GRASS_PROGRAM_SHADOW: {
			ipo->SetUniformMatrix4x4<float>("viewMatrix", false, shadowHandler.GetShadowViewMatrix());
			ipo->SetUniformMatrix4x4<float>("projMatrix", false, shadowHandler.GetShadowProjMatrix());
		} break;
		default: {
		} break;
	}

	ipo->SetUniform3v("ambientLightColor",  &sunLighting->modelAmbientColor.x);
	ipo->SetUniform3v("diffuseLightColor",  &sunLighting->modelDiffuseColor.x);
	ipo->SetUniform3v("specularLightColor", &sunLighting->modelSpecularColor.x);
	ipo->SetUniform3v("sunDir",             &sky->GetLight()->GetLightDir().x);
	ipo->SetUniform3v("fogParams",          &fogParams.x);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// NB: has to use another RNG so density reduction is not influenced
static float3 GetRandomTurfParams(const int2& blockPos)
{
	float3 p;
	p.x = (blockPos.x + turfRNG.NextFloat()) * GSSSQ;
	p.y = (blockPos.y + turfRNG.NextFloat()) * GSSSQ;
	p.z = (turfRNG.NextFloat() * 360.0f) * math::DEG_TO_RAD; // rotation
	return p;
}

static float GetGrassBlockCamDistSq(const float3& pos, const int2 coors)
{
	const float qx = coors.x * GSSSQ;
	const float qz = coors.y * GSSSQ;

	const float3 mid = {qx, CGround::GetHeightReal(qx, qz, false), qz};
	const float3 dif = pos - mid;

	return (dif.SqLength());
}



unsigned int CGrassDrawer::DrawBlock(const float3& camPos, const int2& blockPos, unsigned int turfMatIndex)
{
	const float blockDist = GetGrassBlockCamDistSq(camPos, blockPos);
	const float drawProb = std::min(1.0f, Square(grassDrawDist) / blockDist);

	if (drawProb < 0.001f)
		return 0;

	drawRNG.Seed(blockPos.y * (mapDims.mapx / grassSquareSize) + blockPos.x);
	turfRNG.Seed(blockPos.y * (mapDims.mapx / grassSquareSize) + blockPos.x);

	unsigned int numVisibleTurfs = 0;

	// TODO: LODs?
	for (int a = 0; a < turfDetail.x; a++) {
		if (drawRNG.NextFloat() > drawProb)
			continue;

		const float3& rtp = GetRandomTurfParams(blockPos);
		const float3  pos = {rtp.x, CGround::GetHeightReal(rtp.x, rtp.y, false) - CGround::GetSlope(rtp.x, rtp.y, false) * 30.0f, rtp.y};

		turfMatrices[turfMatIndex + numVisibleTurfs].LoadIdentity();
		turfMatrices[turfMatIndex + numVisibleTurfs].Translate(pos);
		turfMatrices[turfMatIndex + numVisibleTurfs].RotateY(-rtp.z);

		numVisibleTurfs += 1;
	}

	return numVisibleTurfs;
}

void CGrassDrawer::DrawBlocks(const CCamera* cam)
{
	Shader::IProgramObject* grassShader = grassShaders[GRASS_PROGRAM_CURR];

	const unsigned int maxTurfsPerBatch = turfMatrices.size() - turfDetail.x + 1;
	      unsigned int numInstanceTurfs = 0;

	for (const int2 idx: blockDrawer.inViewQuads) {
		for (int y = idx.y * grassBlockSize; y < (idx.y + 1) * grassBlockSize; ++y) {
			for (int x = idx.x * grassBlockSize; x < (idx.x + 1) * grassBlockSize; ++x) {
				if (grassMap[y * (mapDims.mapx / grassSquareSize) + x] == 0)
					continue;

				if ((numInstanceTurfs += DrawBlock(cam->GetPos(), {x, y}, numInstanceTurfs)) < maxTurfsPerBatch)
					continue;

				grassShader->SetUniformMatrix4fv(0, -int(numInstanceTurfs), false, &turfMatrices[0].m[0]);
				grassBuffer.SubmitIndexedInstanced(GL_TRIANGLES, 0, grassBuffer.GetNumIndcs<uint32_t>(), numInstanceTurfs);

				numInstanceTurfs = 0;
			}
		}
	}

	if (numInstanceTurfs == 0)
		return;

	// draw any leftovers
	grassShader->SetUniformMatrix4fv(0, -int(numInstanceTurfs), false, &turfMatrices[0].m[0]);
	grassBuffer.SubmitIndexedInstanced(GL_TRIANGLES, 0, grassBuffer.GetNumIndcs<uint32_t>(), numInstanceTurfs);
}



void CGrassDrawer::Update()
{
	// grass is never drawn in any special (non-opaque) pass
	CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	// update visible blocks
	updateVisibility |= (prvUpdateCamPos != cam->GetPos());
	updateVisibility |= (prvUpdateCamDir != cam->GetDir());

	if (updateVisibility) {
		prvUpdateCamPos = cam->GetPos();
		prvUpdateCamDir = cam->GetDir();

		blockDrawer.ResetState();
		readMap->GridVisibility(nullptr, &blockDrawer, grassDrawDist * grassDrawDist, blockMapSize);

		updateVisibility = false;
	}
}



void CGrassDrawer::SetupStateShadow()
{
	EnableShader(GRASS_PROGRAM_SHADOW);

	glActiveTexture(GL_TEXTURE0);

	glAttribStatePtr->DisableCullFace();
	glAttribStatePtr->PolygonOffset(5.0f, 15.0f);
	glAttribStatePtr->PolygonOffsetFill(GL_TRUE);
}

void CGrassDrawer::ResetStateShadow()
{
	glAttribStatePtr->EnableCullFace();
	glAttribStatePtr->PolygonOffsetFill(GL_FALSE);

	grassShaders[GRASS_PROGRAM_CURR]->Disable();
}

void CGrassDrawer::DrawShadow()
{
	if (luaDrawGrass) {
		eventHandler.DrawGrass();
		return;
	}

	if (!defDrawGrass || readMap->GetGrassShadingTexture() == 0)
		return;

	if (!blockDrawer.inViewQuads.empty()) {
		SetupStateShadow();
		#if 0
		DrawBlocks(CCameraHandler::GetCamera(CCamera::CAMTYPE_SHADOW));
		#else
		// note: use player camera here s.t. all blocks it can see are shadowed
		DrawBlocks(CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER));
		#endif
		ResetStateShadow();
	}
}



void CGrassDrawer::SetupStateOpaque()
{
	// bind textures
	{
		glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, grassBladeTex);
		glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, readMap->GetGrassShadingTexture());
		glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
		glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	}

	// bind shader
	{
		EnableShader(GRASS_PROGRAM_OPAQUE);

		if (shadowHandler.ShadowsLoaded())
			shadowHandler.SetupShadowTexSampler(GL_TEXTURE4);
	}

	glActiveTexture(GL_TEXTURE0);
	glAttribStatePtr->DisableBlendMask();
	glAttribStatePtr->EnableDepthMask();
}

void CGrassDrawer::ResetStateOpaque()
{
	grassShaders[GRASS_PROGRAM_CURR]->Disable();

	glAttribStatePtr->EnableBlendMask();
}

void CGrassDrawer::Draw()
{
	if (luaDrawGrass) {
		// bypass engine rendering
		eventHandler.DrawGrass();
		return;
	}

	if (!defDrawGrass || readMap->GetGrassShadingTexture() == 0)
		return;

	glAttribStatePtr->PushBits(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

	if (!blockDrawer.inViewQuads.empty()) {
		SetupStateOpaque();
		DrawBlocks(CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER));
		ResetStateOpaque();
	}

	glAttribStatePtr->PopBits();
}




void CGrassDrawer::AddGrass(const float3& pos)
{
	if (!defDrawGrass)
		return;

	const int x = int(pos.x) / GSSSQ;
	const int z = int(pos.z) / GSSSQ;
	assert(x >= 0 && x < (mapDims.mapx / grassSquareSize));
	assert(z >= 0 && z < (mapDims.mapy / grassSquareSize));

	grassMap[z * (mapDims.mapx / grassSquareSize) + x] = 1;
}

void CGrassDrawer::RemoveGrass(const float3& pos)
{
	if (!defDrawGrass)
		return;

	const int x = int(pos.x) / GSSSQ;
	const int z = int(pos.z) / GSSSQ;
	assert(x >= 0 && x < (mapDims.mapx / grassSquareSize));
	assert(z >= 0 && z < (mapDims.mapy / grassSquareSize));

	grassMap[z * (mapDims.mapx / grassSquareSize) + x] = 0;
}

uint8_t CGrassDrawer::GetGrass(const float3& pos)
{
	if (!defDrawGrass)
		return -1;

	const int x = int(pos.x) / GSSSQ;
	const int z = int(pos.z) / GSSSQ;
	assert(x >= 0 && x < (mapDims.mapx / grassSquareSize));
	assert(z >= 0 && z < (mapDims.mapy / grassSquareSize));

	return grassMap[z * (mapDims.mapx / grassSquareSize) + x];
}

