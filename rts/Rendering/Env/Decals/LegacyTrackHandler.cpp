/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyTrackHandler.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/HeightMapTexture.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"

#include <algorithm>
#include <cctype>

static constexpr int DECAL_LEVEL = 3; //FIXME
static constexpr int UPDATE_PERIOD = 8;



LegacyTrackHandler::LegacyTrackHandler()
	: CEventClient("[LegacyTrackHandler]", 314160, false)
{
	eventHandler.AddClient(this);
	LoadDecalShaders();

	unitTracks.reserve(256);
}

LegacyTrackHandler::~LegacyTrackHandler()
{
	eventHandler.RemoveClient(this);

	for (TrackType& tt: trackTypes) {
		glDeleteTextures(1, &tt.texture);
	}

	#ifndef USE_DECALHANDLER_STATE
	shaderHandler->ReleaseProgramObjects("[LegacyTrackHandler]");
	#endif
}


void LegacyTrackHandler::LoadDecalShaders()
{
	#ifndef USE_DECALHANDLER_STATE
	#define sh shaderHandler
	decalShaders.fill(nullptr);

	const std::string extraDef = "#define HAVE_SHADING_TEX " + IntToString(readMap->GetShadingTexture() != 0, "%d") + "\n";
	const float4 invMapSize = {
		1.0f / (mapDims.pwr2mapx * SQUARE_SIZE),
		1.0f / (mapDims.pwr2mapy * SQUARE_SIZE),
		1.0f / (mapDims.mapx * SQUARE_SIZE),
		1.0f / (mapDims.mapy * SQUARE_SIZE),
	};

	decalShaders[DECAL_SHADER_NULL] = Shader::nullProgramObject;
	decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_NULL];
	decalShaders[DECAL_SHADER_GLSL] = sh->CreateProgramObject("[LegacyTrackHandler]", "DecalShaderGLSL");

	decalShaders[DECAL_SHADER_GLSL]->AttachShaderObject(sh->CreateShaderObject("GLSL/GroundDecalsVertProg.glsl", "",       GL_VERTEX_SHADER));
	decalShaders[DECAL_SHADER_GLSL]->AttachShaderObject(sh->CreateShaderObject("GLSL/GroundDecalsFragProg.glsl", extraDef, GL_FRAGMENT_SHADER));
	decalShaders[DECAL_SHADER_GLSL]->Link();

	decalShaders[DECAL_SHADER_GLSL]->SetFlag("HAVE_SHADOWS", false);

	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("decalTex");           // idx  0
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadeTex");           // idx  1
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowTex");          // idx  2
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("heightTex");          // idx  3
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("mapSizePO2");         // idx  4
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("groundAmbientColor"); // idx  5
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("viewMatrix");         // idx  6
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("projMatrix");         // idx  7
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("quadMatrix");         // idx  8
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowMatrix");       // idx  9
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowParams");       // idx 10
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowDensity");      // idx 11
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("decalAlpha");         // idx 12
	decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("gammaExponent");      // idx 13

	decalShaders[DECAL_SHADER_GLSL]->Enable();
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(0, 0); // decalTex  (idx 0, texunit 0)
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(1, 1); // shadeTex  (idx 1, texunit 1)
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(2, 2); // shadowTex (idx 2, texunit 2)
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(3, 3); // heightTex (idx 3, texunit 3)
	decalShaders[DECAL_SHADER_GLSL]->SetUniform4f(4, invMapSize.x, invMapSize.y, invMapSize.z, invMapSize.w);
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(11, sunLighting->groundShadowDensity);
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(12, 1.0f);
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(13, globalRendering->gammaExponent);
	decalShaders[DECAL_SHADER_GLSL]->Disable();
	decalShaders[DECAL_SHADER_GLSL]->Validate();

	decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_GLSL];
	#undef sh
	#endif
}

void LegacyTrackHandler::SunChanged()
{
	#ifndef USE_DECALHANDLER_STATE
	decalShaders[DECAL_SHADER_GLSL]->Enable();
	decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(11, sunLighting->groundShadowDensity);
	decalShaders[DECAL_SHADER_GLSL]->Disable();
	#endif
}


void LegacyTrackHandler::AddTracks()
{
	for (unsigned int trackID: updatedTrackIDs) {
		UnitTrack& unitTrack = unitTracks[trackID];
		auto& parts = unitTrack.parts;

		// check if RenderUnitDestroyed pre-empted us
		if (unitTrack.owner == nullptr)
			continue;

		if (parts.size() <= 2)
			continue;

		const TrackPart& nextPart = parts[parts.size() - 1];
		const TrackPart& lastPart = parts[parts.size() - 2];
		const TrackPart& prevPart = parts[parts.size() - 3];

		// if the unit is moving in a straight line, only append parts at
		// half the rate by replacing the last by the most recently added
		if (((prevPart.pos1).SqDistance((nextPart.pos1 + lastPart.pos1) * 0.5f) >= 1.0f))
			continue;

		parts[parts.size() - 2] = nextPart;
		parts.pop_back();
	}

	updatedTrackIDs.clear();
}


void LegacyTrackHandler::DrawTracks(GL::RenderDataBufferTC* buffer, Shader::IProgramObject* shader)
{
	unsigned char curPartColor[4] = {255, 255, 255, 255};
	unsigned char nxtPartColor[4] = {255, 255, 255, 255};

	shader->SetUniform1f(12, 1.0f);
	shader->SetUniformMatrix4fv(8, false, CMatrix44f::Identity());

	// create and draw the unit footprint quads
	for (const TrackType& tt: trackTypes) {
		if (tt.trackIDs.empty())
			continue;


		glBindTexture(GL_TEXTURE_2D, tt.texture);

		for (unsigned int trackID: tt.trackIDs) {
			UnitTrack& track = unitTracks[trackID];
			auto& parts = track.parts;

			if (parts.empty()) {
				cleanedTrackIDs.emplace_back(trackID, &tt - &trackTypes[0]);
				continue;
			}

			const TrackPart& frontPart = parts.front();
			const TrackPart&  backPart = parts.back();

			if (gs->frameNum > (frontPart.creationTime + track.lifeTime)) {
				cleanedTrackIDs.emplace_back(trackID, &tt - &trackTypes[0]);
				// still draw the track to avoid flicker
				// continue;
			}

			if (!camera->InView((frontPart.pos1 + backPart.pos1) * 0.5f, frontPart.pos1.distance(backPart.pos1) + 500.0f))
				continue;

			// walk across the track parts from front (oldest) to back (newest) and draw
			// a quad between "connected" parts (ie. parts differing 8 sim-frames in age)
			auto curPartIt =   (parts.cbegin());
			auto nxtPartIt = ++(parts.cbegin());

			curPartColor[3] = std::max(0.0f, 255.0f - (gs->frameNum - (*curPartIt).creationTime) * track.alphaFalloff);

			for (; nxtPartIt != parts.cend(); ++nxtPartIt) {
				const TrackPart& curPart = *curPartIt;
				const TrackPart& nxtPart = *nxtPartIt;

				nxtPartColor[3] = std::max(0.0f, 255.0f - (gs->frameNum - nxtPart.creationTime) * track.alphaFalloff);

				if (nxtPart.connected) {
					buffer->SafeAppend({curPart.pos1, curPart.texPos, 0.0f, curPartColor});
					buffer->SafeAppend({curPart.pos2, curPart.texPos, 1.0f, curPartColor});
					buffer->SafeAppend({nxtPart.pos2, nxtPart.texPos, 1.0f, nxtPartColor});

					buffer->SafeAppend({nxtPart.pos2, nxtPart.texPos, 1.0f, nxtPartColor});
					buffer->SafeAppend({nxtPart.pos1, nxtPart.texPos, 0.0f, nxtPartColor});
					buffer->SafeAppend({curPart.pos1, curPart.texPos, 0.0f, curPartColor});
				}

				curPartColor[3] = nxtPartColor[3];
				curPartIt = nxtPartIt;
			}
		}

		buffer->Submit(GL_TRIANGLES);
	}
}

void LegacyTrackHandler::CleanTracks()
{
	// remove expired track parts; cleanup empty tracks
	for (TrackToClean& ttc: cleanedTrackIDs) {
		UnitTrack& track = unitTracks[ttc.trackID];
		auto& parts = track.parts;

		while (!parts.empty()) {
			// stop at the first part that is still too young for deletion
			if (gs->frameNum < ((parts.front()).creationTime + track.lifeTime))
				break;

			parts.pop_front();
		}

		if (parts.empty()) {
			spring::VectorErase(trackTypes[ttc.ttIndex].trackIDs, track.id);
			unitTracks.erase(track.id);
		}
	}

	cleanedTrackIDs.clear();
}


bool LegacyTrackHandler::GetDrawTracks() const
{
	//FIXME move track updating to ::Update()
	if (!updatedTrackIDs.empty() || !cleanedTrackIDs.empty())
		return true;

	const auto pred = [](const TrackType& tt) { return (!tt.trackIDs.empty()); };
	const auto iter = std::find_if(trackTypes.begin(), trackTypes.end(), pred);

	return (iter != trackTypes.end());
}


void LegacyTrackHandler::Draw(Shader::IProgramObject* shader)
{
	if (!GetDrawTracks())
		return;

	#ifndef USE_DECALHANDLER_STATE
	{
		glAttribStatePtr->EnableBlendMask();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAttribStatePtr->EnablePolyOfsFill();
		glAttribStatePtr->DisableDepthMask();
		glAttribStatePtr->PolygonOffset(-10.0f, -20.0f);

		BindTextures();
		BindShader(sunLighting->groundAmbientColor * CGlobalRendering::SMF_INTENSITY_MULT);

		AddTracks();
		DrawTracks(GL::GetRenderBufferTC(), decalShaders[DECAL_SHADER_CURR]);
		CleanTracks();

		decalShaders[DECAL_SHADER_CURR]->Disable();
		KillTextures();

		glAttribStatePtr->DisablePolyOfsFill();
		glAttribStatePtr->DisableBlendMask();
	}
	#else
	{
		AddTracks();
		DrawTracks(GL::GetRenderBufferTC(), shader);
		CleanTracks();
	}
	#endif
}


void LegacyTrackHandler::BindTextures()
{
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
	}

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE2);

	glActiveTexture(GL_TEXTURE0);
}


void LegacyTrackHandler::KillTextures()
{
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (shadowHandler.ShadowsLoaded())
		shadowHandler.ResetShadowTexSampler(GL_TEXTURE2);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void LegacyTrackHandler::BindShader(const float3& ambientColor)
{
	#ifndef USE_DECALHANDLER_STATE
	decalShaders[DECAL_SHADER_CURR]->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	decalShaders[DECAL_SHADER_CURR]->Enable();

	if (decalShaders[DECAL_SHADER_CURR] == decalShaders[DECAL_SHADER_GLSL]) {
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(5, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniformMatrix4fv(6, false, camera->GetViewMatrix());
		decalShaders[DECAL_SHADER_CURR]->SetUniformMatrix4fv(7, false, camera->GetProjectionMatrix());
		decalShaders[DECAL_SHADER_CURR]->SetUniformMatrix4fv(9, false, shadowHandler.GetShadowViewMatrixRaw());
		decalShaders[DECAL_SHADER_CURR]->SetUniform4fv(10, shadowHandler.GetShadowParams());
		decalShaders[DECAL_SHADER_CURR]->SetUniform1f(13, globalRendering->gammaExponent);
	}
	#endif
}



static bool CanReceiveTracks(const float3& pos)
{
	// calculate typemap-index
	const int tmz = pos.z / (SQUARE_SIZE * 2);
	const int tmx = pos.x / (SQUARE_SIZE * 2);
	const int tmi = Clamp(tmz * mapDims.hmapx + tmx, 0, mapDims.hmapx * mapDims.hmapy - 1);

	const uint8_t* typeMap = readMap->GetTypeMapSynced();
	const uint8_t  typeNum = typeMap[tmi];

	return (mapInfo->terrainTypes[typeNum].receiveTracks);

}

void LegacyTrackHandler::CreateOrAddTrackPart(const CUnit* unit, const SolidObjectDecalDef& decalDef, const float3& pos)
{
	const float trackLifeTime = GAME_SPEED * DECAL_LEVEL * decalDef.trackDecalStrength;

	if (trackLifeTime <= 0.0f)
		return;

	UnitTrack& unitTrack = unitTracks[unit->id];

	if (unitTrack.owner != nullptr && unitTrack.lastUpdate >= (gs->frameNum - (UPDATE_PERIOD - 1)))
		return;

	if (!gu->spectatingFullView && !unit->IsInLosForAllyTeam(gu->myAllyTeam))
		return;

	// prepare the new part of the track
	unitTrack.parts.emplace_back();

	TrackPart& nextPart = unitTrack.parts.back();
	nextPart.pos1 = pos + unit->rightdir * decalDef.trackDecalWidth * 0.5f;
	nextPart.pos2 = pos - unit->rightdir * decalDef.trackDecalWidth * 0.5f;
	nextPart.pos1.y = CGround::GetHeightReal(nextPart.pos1.x, nextPart.pos1.z, false);
	nextPart.pos2.y = CGround::GetHeightReal(nextPart.pos2.x, nextPart.pos2.z, false);
	nextPart.creationTime = gs->frameNum;
	unitTrack.lastUpdate = gs->frameNum;

	if (unitTrack.owner == nullptr) {
		unitTrack.owner = unit;

		unitTrack.id = unit->id;
		unitTrack.lifeTime = trackLifeTime;
		unitTrack.alphaFalloff = 255.0f / trackLifeTime;

		auto& decDef = unit->unitDef->decalDef;
		auto& trType = trackTypes[decDef.trackDecalType];

		spring::VectorInsertUnique(trType.trackIDs, unitTrack.id);
	} else {
		const TrackPart& prevPart = unitTrack.parts[unitTrack.parts.size() - 2];

		const float partDist = (nextPart.pos1).distance(prevPart.pos1);
		const float texShift = (partDist / decalDef.trackDecalWidth) * decalDef.trackDecalStretch;

		nextPart.texPos = prevPart.texPos + texShift;
		nextPart.connected = (prevPart.creationTime == (gs->frameNum - UPDATE_PERIOD));
	}

	updatedTrackIDs.push_back(unitTrack.id);
}

void LegacyTrackHandler::AddTrack(const CUnit* unit, const float3& newPos)
{
	const UnitDef* unitDef = unit->unitDef;
	const SolidObjectDecalDef& decalDef = unitDef->decalDef;

	if (!unit->leaveTracks)
		return;
	if (!unitDef->IsGroundUnit())
		return;


	// -2 := failed to load texture, do not try again
	// -1 := texture was not loaded yet, first attempt
	switch (decalDef.trackDecalType) {
		case -2: {                                                                             return; } break;
		case -1: { const_cast<SolidObjectDecalDef&>(decalDef).trackDecalType = GetTrackType(decalDef); } break;
		default: {                                                                                     } break;
	}

	if (decalDef.trackDecalType < 0)
		return;

	if (unitTracks.find(unit->id) == unitTracks.end())
		unitTracks[unit->id] = std::move(UnitTrack{});

	if (!CanReceiveTracks(newPos))
		return;

	CreateOrAddTrackPart(unit, decalDef, newPos + unit->frontdir * decalDef.trackDecalOffset);
}


int LegacyTrackHandler::GetTrackType(const SolidObjectDecalDef& def)
{
	const std::string& lowerName = StringToLower(def.trackDecalTypeName);

	const auto pred = [&](const TrackType& tt) { return (tt.name == lowerName); };
	const auto iter = std::find_if(trackTypes.begin(), trackTypes.end(), pred);

	if (iter != trackTypes.end())
		return (iter - trackTypes.begin());

	const GLuint texID = LoadTexture(lowerName);
	if (texID == 0)
		return -2;

	trackTypes.emplace_back(lowerName, texID);
	return (trackTypes.size() - 1);
}


unsigned int LegacyTrackHandler::LoadTexture(const std::string& name)
{
	std::string fullName = name;
	if (fullName.find_first_of('.') == std::string::npos)
		fullName += ".bmp";

	if ((fullName.find_first_of('\\') == std::string::npos) && (fullName.find_first_of('/') == std::string::npos))
		fullName = std::string("bitmaps/tracks/") + fullName;

	CBitmap bm;
	if (!bm.Load(fullName)) {
		LOG_L(L_WARNING, "Could not load track decal from file %s", fullName.c_str());
		return 0;
	}

	if (FileSystem::GetExtension(fullName) == "bmp") {
		// bitmaps don't have an alpha channel
		// so use: red := brightness & green := alpha
		const unsigned char* rmem = bm.GetRawMem();
		      unsigned char* wmem = bm.GetRawMem();

		for (int y = 0; y < bm.ysize; ++y) {
			for (int x = 0; x < bm.xsize; ++x) {
				const int index = ((y * bm.xsize) + x) * 4;
				const int brightness = rmem[index + 0];

				wmem[index + 3] = rmem[index + 1];
				wmem[index + 0] = (brightness * 90) / 255;
				wmem[index + 1] = (brightness * 60) / 255;
				wmem[index + 2] = (brightness * 30) / 255;
			}
		}
	}

	return bm.CreateMipMapTexture();
}




void LegacyTrackHandler::UnitMoved(const CUnit* unit)
{
	AddTrack(unit, unit->pos);
}

void LegacyTrackHandler::RenderUnitDestroyed(const CUnit* unit)
{
	// pre-empt AddTracks, but keep the track so it can decay normally
	unitTracks[unit->id].owner = nullptr;
}

