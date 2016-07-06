/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyTrackHandler.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Units/UnitDef.h"
#include "System/EventHandler.h"
#include "System/Util.h"
#include "System/TimeProfiler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"

#include <algorithm>
#include <cctype>



LegacyTrackHandler::LegacyTrackHandler()
	: CEventClient("[LegacyTrackHandler]", 314160, false)
{
	eventHandler.AddClient(this);
	LoadDecalShaders();
}



LegacyTrackHandler::~LegacyTrackHandler()
{
	eventHandler.RemoveClient(this);

	for (TrackType& tt: trackTypes) {
		for (UnitTrackStruct* uts: tt.tracks)
			VectorInsertUnique(tracksToBeDeleted, uts, true);

		glDeleteTextures(1, &tt.texture);
	}

	for (auto ti = tracksToBeAdded.cbegin(); ti != tracksToBeAdded.cend(); ++ti)
		VectorInsertUnique(tracksToBeDeleted, *ti, true);

	for (auto ti = tracksToBeDeleted.cbegin(); ti != tracksToBeDeleted.cend(); ++ti)
		delete *ti;

	trackTypes.clear();
	tracksToBeAdded.clear();
	tracksToBeDeleted.clear();

	shaderHandler->ReleaseProgramObjects("[LegacyTrackHandler]");
	decalShaders.clear();
}


void LegacyTrackHandler::LoadDecalShaders()
{
	#define sh shaderHandler
	decalShaders.resize(DECAL_SHADER_LAST, NULL);

	// SM3 maps have no baked lighting, so decals blend differently
	const bool haveShadingTexture = (readMap->GetShadingTexture() != 0);
	const char* fragmentProgramNameARB = haveShadingTexture?
		"ARB/GroundDecalsSMF.fp":
		"ARB/GroundDecalsSM3.fp";
	const std::string extraDef = haveShadingTexture?
		"#define HAVE_SHADING_TEX 1\n":
		"#define HAVE_SHADING_TEX 0\n";

	decalShaders[DECAL_SHADER_ARB ] = sh->CreateProgramObject("[LegacyTrackHandler]", "DecalShaderARB",  true);
	decalShaders[DECAL_SHADER_GLSL] = sh->CreateProgramObject("[LegacyTrackHandler]", "DecalShaderGLSL", false);
	decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_ARB];

	if (globalRendering->haveGLSL) {
		decalShaders[DECAL_SHADER_GLSL]->AttachShaderObject(sh->CreateShaderObject("GLSL/GroundDecalsVertProg.glsl", "",       GL_VERTEX_SHADER));
		decalShaders[DECAL_SHADER_GLSL]->AttachShaderObject(sh->CreateShaderObject("GLSL/GroundDecalsFragProg.glsl", extraDef, GL_FRAGMENT_SHADER));
		decalShaders[DECAL_SHADER_GLSL]->Link();

		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("decalTex");           // idx 0
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadeTex");           // idx 1
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowTex");          // idx 2
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("mapSizePO2");         // idx 3
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("groundAmbientColor"); // idx 4
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowMatrix");       // idx 5
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowParams");       // idx 6
		decalShaders[DECAL_SHADER_GLSL]->SetUniformLocation("shadowDensity");      // idx 7

		decalShaders[DECAL_SHADER_GLSL]->Enable();
		decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(0, 0); // decalTex  (idx 0, texunit 0)
		decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(1, 1); // shadeTex  (idx 1, texunit 1)
		decalShaders[DECAL_SHADER_GLSL]->SetUniform1i(2, 2); // shadowTex (idx 2, texunit 2)
		decalShaders[DECAL_SHADER_GLSL]->SetUniform2f(3, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE));
		decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(7, sky->GetLight()->GetGroundShadowDensity());
		decalShaders[DECAL_SHADER_GLSL]->Disable();
		decalShaders[DECAL_SHADER_GLSL]->Validate();

		decalShaders[DECAL_SHADER_CURR] = decalShaders[DECAL_SHADER_GLSL];
	} else if (globalRendering->haveARB) {
		decalShaders[DECAL_SHADER_ARB]->AttachShaderObject(sh->CreateShaderObject("ARB/GroundDecals.vp", "", GL_VERTEX_PROGRAM_ARB));
		decalShaders[DECAL_SHADER_ARB]->AttachShaderObject(sh->CreateShaderObject(fragmentProgramNameARB, "", GL_FRAGMENT_PROGRAM_ARB));
		decalShaders[DECAL_SHADER_ARB]->Link();
	}

	#undef sh
}

void LegacyTrackHandler::SunChanged()
{
	if (globalRendering->haveGLSL && decalShaders.size() > DECAL_SHADER_GLSL) {
		decalShaders[DECAL_SHADER_GLSL]->Enable();
		decalShaders[DECAL_SHADER_GLSL]->SetUniform1f(7, sky->GetLight()->GetGroundShadowDensity());
		decalShaders[DECAL_SHADER_GLSL]->Disable();
	}
}


void LegacyTrackHandler::AddTracks()
{
	// Delayed addition of new tracks
	for (const UnitTrackStruct* uts: tracksToBeAdded) {
		const CUnit* unit = uts->owner;
		const TrackPart& tp = uts->lastAdded;

		// check if RenderUnitDestroyed pre-empted us
		if (unit == nullptr)
			continue;

		// if the unit is moving in a straight line only place marks at half the rate by replacing old ones
		bool replace = false;

		if (unit->myTrack->parts.size() > 1) {
			std::deque<TrackPart>::iterator pi = --unit->myTrack->parts.end();
			std::deque<TrackPart>::iterator pi2 = pi--;

			replace = (((tp.pos1 + (*pi).pos1) * 0.5f).SqDistance((*pi2).pos1) < 1.0f);
		}

		if (replace) {
			unit->myTrack->parts.back() = tp;
		} else {
			unit->myTrack->parts.push_back(tp);
		}
	}

	tracksToBeAdded.clear();

	for (UnitTrackStruct* uts: tracksToBeDeleted)
		delete uts;

	tracksToBeDeleted.clear();
}


void LegacyTrackHandler::DrawTracks()
{
	unsigned char curPartColor[4] = {255, 255, 255, 255};
	unsigned char nxtPartColor[4] = {255, 255, 255, 255};

	// create and draw the unit footprint quads
	for (TrackType& tt: trackTypes) {
		if (tt.tracks.empty())
			continue;


		CVertexArray* va = GetVertexArray();
		va->Initialize();
		glBindTexture(GL_TEXTURE_2D, tt.texture);

		for (UnitTrackStruct* track: tt.tracks) {
			if (track->parts.empty()) {
				tracksToBeCleaned.push_back(TrackToClean(track, &(tt.tracks)));
				continue;
			}

			if (gs->frameNum > ((track->parts.front()).creationTime + track->lifeTime)) {
				tracksToBeCleaned.push_back(TrackToClean(track, &(tt.tracks)));
				// still draw the track to avoid flicker
				// continue;
			}

			const auto frontPart = track->parts.front();
			const auto backPart  = track->parts.back();

			if (!camera->InView((frontPart.pos1 + backPart.pos1) * 0.5f, frontPart.pos1.distance(backPart.pos1) + 500.0f))
				continue;

			// walk across the track parts from front (oldest) to back (newest) and draw
			// a quad between "connected" parts (ie. parts differing 8 sim-frames in age)
			std::deque<TrackPart>::const_iterator curPart =   (track->parts.begin());
			std::deque<TrackPart>::const_iterator nxtPart = ++(track->parts.begin());

			curPartColor[3] = std::max(0.0f, 255.0f - (gs->frameNum - (*curPart).creationTime) * track->alphaFalloff);

			va->EnlargeArrays(track->parts.size() * 4, 0, VA_SIZE_TC);

			for (; nxtPart != track->parts.end(); ++nxtPart) {
				nxtPartColor[3] = std::max(0.0f, 255.0f - (gs->frameNum - (*nxtPart).creationTime) * track->alphaFalloff);

				if ((*nxtPart).connected) {
					va->AddVertexQTC((*curPart).pos1, (*curPart).texPos, 0, curPartColor);
					va->AddVertexQTC((*curPart).pos2, (*curPart).texPos, 1, curPartColor);
					va->AddVertexQTC((*nxtPart).pos2, (*nxtPart).texPos, 1, nxtPartColor);
					va->AddVertexQTC((*nxtPart).pos1, (*nxtPart).texPos, 0, nxtPartColor);
				}

				curPartColor[3] = nxtPartColor[3];
				curPart = nxtPart;
			}
		}

		va->DrawArrayTC(GL_QUADS);
	}
}

void LegacyTrackHandler::CleanTracks()
{
	// Cleanup old tracks; runs *immediately* after DrawTracks
	for (TrackToClean& ttc: tracksToBeCleaned) {
		UnitTrackStruct* track = ttc.track;

		while (!track->parts.empty()) {
			// stop at the first part that is still too young for deletion
			if (gs->frameNum < ((track->parts.front()).creationTime + track->lifeTime))
				break;

			track->parts.pop_front();
		}

		if (track->parts.empty()) {
			if (track->owner != nullptr) {
				track->owner->myTrack = nullptr;
				track->owner = nullptr;
			}

			VectorErase(*ttc.tracks, track);
			tracksToBeDeleted.push_back(track);
		}
	}

	tracksToBeCleaned.clear();
}


bool LegacyTrackHandler::GetDrawTracks() const
{
	//FIXME move track updating to ::Update()
	if ((!tracksToBeAdded.empty()) || (!tracksToBeCleaned.empty()) || (!tracksToBeDeleted.empty()))
		return true;

	for (auto& tt: trackTypes) {
		if (!tt.tracks.empty())
			return true;
	}

	return false;
}


void LegacyTrackHandler::Draw()
{
	SCOPED_TIMER("TracksDrawer");

	if (!GetDrawTracks())
		return;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glDepthMask(0);
	glPolygonOffset(-10, -20);

	BindTextures();
	BindShader(sunLighting->groundAmbientColor * CGlobalRendering::SMF_INTENSITY_MULT);

	// draw track decals
	AddTracks();
	DrawTracks();
	CleanTracks();

	decalShaders[DECAL_SHADER_CURR]->Disable();
	KillTextures();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_BLEND);
}


void LegacyTrackHandler::BindTextures()
{
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE1_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);
	}

	if (shadowHandler->ShadowsLoaded()) {
		shadowHandler->SetupShadowTexSampler(GL_TEXTURE2, true);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); //??
	}

	if (infoTextureHandler->IsEnabled()) {
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

		glMultiTexCoord4f(GL_TEXTURE3_ARB, 1.0f,1.0f,1.0f,1.0f); // workaround a nvidia bug with TexGen
		SetTexGen(1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0, 0);

		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	}

	glActiveTexture(GL_TEXTURE0);
}


void LegacyTrackHandler::KillTextures()
{
	{
		glActiveTexture(GL_TEXTURE3); // infotex
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if (shadowHandler->ShadowsLoaded()) {
		shadowHandler->ResetShadowTexSampler(GL_TEXTURE2, true);

		glActiveTexture(GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	}

	{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	glActiveTexture(GL_TEXTURE0);
}


void LegacyTrackHandler::BindShader(const float3& ambientColor)
{
	decalShaders[DECAL_SHADER_CURR]->Enable();

	if (decalShaders[DECAL_SHADER_CURR] == decalShaders[DECAL_SHADER_ARB]) {
		decalShaders[DECAL_SHADER_CURR]->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(10, 1.0f / (mapDims.pwr2mapx * SQUARE_SIZE), 1.0f / (mapDims.pwr2mapy * SQUARE_SIZE), 0.0f, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(10, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(11, 0.0f, 0.0f, 0.0f, sky->GetLight()->GetGroundShadowDensity());

		glMatrixMode(GL_MATRIX0_ARB);
		glLoadMatrixf(shadowHandler->GetShadowMatrixRaw());
		glMatrixMode(GL_MODELVIEW);
	} else {
		decalShaders[DECAL_SHADER_CURR]->SetUniform4f(4, ambientColor.x, ambientColor.y, ambientColor.z, 1.0f);
		decalShaders[DECAL_SHADER_CURR]->SetUniformMatrix4fv(5, false, shadowHandler->GetShadowMatrixRaw());
		decalShaders[DECAL_SHADER_CURR]->SetUniform4fv(6, &(shadowHandler->GetShadowParams().x));
	}
}


void LegacyTrackHandler::AddTrack(CUnit* unit, const float3& newPos)
{
	if (!unit->leaveTracks)
		return;

	const UnitDef* unitDef = unit->unitDef;
	const SolidObjectDecalDef& decalDef = unitDef->decalDef;

	if (!unitDef->IsGroundUnit())
		return;

	if (decalDef.trackDecalType < -1)
		return;

	if (decalDef.trackDecalType < 0) {
		const_cast<SolidObjectDecalDef&>(decalDef).trackDecalType = GetTrackType(decalDef.trackDecalTypeName);
		if (decalDef.trackDecalType < -1)
			return;
	}

	if (unit->myTrack != NULL && unit->myTrack->lastUpdate >= (gs->frameNum - 7))
		return;

	if (!gu->spectatingFullView && (unit->losStatus[gu->myAllyTeam] & LOS_INLOS) == 0)
		return;

	// calculate typemap-index
	const int tmz = newPos.z / (SQUARE_SIZE * 2);
	const int tmx = newPos.x / (SQUARE_SIZE * 2);
	const int tmi = Clamp(tmz * mapDims.hmapx + tmx, 0, mapDims.hmapx * mapDims.hmapy - 1);

	const unsigned char* typeMap = readMap->GetTypeMapSynced();
	const CMapInfo::TerrainType& terType = mapInfo->terrainTypes[ typeMap[tmi] ];

	if (!terType.receiveTracks)
		return;

	static const int decalLevel = 3; //FIXME
	const float trackLifeTime = GAME_SPEED * decalLevel * decalDef.trackDecalStrength;

	if (trackLifeTime <= 0.0f)
		return;

	const float3 pos = newPos + unit->frontdir * decalDef.trackDecalOffset;


	// prepare the new part of the track; will be copied
	TrackPart trackPart;
	trackPart.pos1 = pos + unit->rightdir * decalDef.trackDecalWidth * 0.5f;
	trackPart.pos2 = pos - unit->rightdir * decalDef.trackDecalWidth * 0.5f;
	trackPart.pos1.y = CGround::GetHeightReal(trackPart.pos1.x, trackPart.pos1.z, false);
	trackPart.pos2.y = CGround::GetHeightReal(trackPart.pos2.x, trackPart.pos2.z, false);
	trackPart.creationTime = gs->frameNum;

	UnitTrackStruct** unitTrack = &unit->myTrack;

	if ((*unitTrack) == nullptr) {
		(*unitTrack) = new UnitTrackStruct(unit);
		(*unitTrack)->lifeTime = trackLifeTime;
		(*unitTrack)->alphaFalloff = 255.0f / trackLifeTime;

		trackPart.texPos = 0;
		trackPart.connected = false;
		trackPart.isNewTrack = true;
	} else {
		const TrackPart& prevPart = (*unitTrack)->lastAdded;

		const float partDist = (trackPart.pos1).distance(prevPart.pos1);
		const float texShift = (partDist / decalDef.trackDecalWidth) * decalDef.trackDecalStretch;

		trackPart.texPos = prevPart.texPos + texShift;
		trackPart.connected = (prevPart.creationTime == (gs->frameNum - 8));
	}

	if (trackPart.isNewTrack) {
		auto& decDef = unit->unitDef->decalDef;
		auto& trType = trackTypes[decDef.trackDecalType];

		VectorInsertUnique(trType.tracks, *unitTrack);
	}

	(*unitTrack)->lastUpdate = gs->frameNum;
	(*unitTrack)->lastAdded = trackPart;

	tracksToBeAdded.push_back(*unitTrack);
}


int LegacyTrackHandler::GetTrackType(const std::string& name)
{
	const std::string& lowerName = StringToLower(name);

	unsigned int a = 0;
	for (auto tt: trackTypes) {
		if (tt.name == lowerName)
			return a;
		++a;
	}

	const GLuint texID = LoadTexture(lowerName);
	if (texID == 0)
		return -2;

	trackTypes.push_back(TrackType(lowerName, texID));
	return (trackTypes.size() - 1);
}


unsigned int LegacyTrackHandler::LoadTexture(const std::string& name)
{
	std::string fullName = name;
	if (fullName.find_first_of('.') == std::string::npos) {
		fullName += ".bmp";
	}
	if ((fullName.find_first_of('\\') == std::string::npos) &&
	    (fullName.find_first_of('/')  == std::string::npos)) {
		fullName = std::string("bitmaps/tracks/") + fullName;
	}

	CBitmap bm;
	if (!bm.Load(fullName)) {
		LOG_L(L_WARNING, "Could not load track decal from file %s", fullName.c_str());
		return 0;
	}
	if (FileSystem::GetExtension(fullName) == "bmp") {
		// bitmaps don't have an alpha channel
		// so use: red := brightness & green := alpha
		for (int y = 0; y < bm.ysize; ++y) {
			for (int x = 0; x < bm.xsize; ++x) {
				const int index = ((y * bm.xsize) + x) * 4;
				bm.mem[index + 3]    = bm.mem[index + 1];
				const int brightness = bm.mem[index + 0];
				bm.mem[index + 0] = (brightness * 90) / 255;
				bm.mem[index + 1] = (brightness * 60) / 255;
				bm.mem[index + 2] = (brightness * 30) / 255;
			}
		}
	}

	return bm.CreateMipMapTexture();
}




void LegacyTrackHandler::RemoveTrack(CUnit* unit)
{
	if (unit->myTrack == nullptr)
		return;

	// same pointer as in tracksToBeAdded, so this also pre-empts DrawTracks
	unit->myTrack->owner = nullptr;
	unit->myTrack = nullptr;
}


void LegacyTrackHandler::UnitMoved(const CUnit* unit)
{
	AddTrack(const_cast<CUnit*>(unit), unit->pos);
}


void LegacyTrackHandler::RenderUnitDestroyed(const CUnit* unit)
{
	RemoveTrack(const_cast<CUnit*>(unit));
}

