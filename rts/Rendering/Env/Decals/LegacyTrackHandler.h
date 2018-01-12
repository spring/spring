/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LEGACYTRACKHANDLER_H
#define LEGACYTRACKHANDLER_H


#include <array>
#include <deque>
#include <vector>
#include <string>

#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "System/float3.h"
#include "System/EventClient.h"
#include "System/UnorderedMap.hpp"

// comment out if using DecalsDrawerGL4
#define USE_DECALHANDLER_STATE


class CSolidObject;
class CUnit;
struct SolidObjectDecalDef;

namespace Shader {
	struct IProgramObject;
}


class LegacyTrackHandler: public CEventClient
{
public:
	LegacyTrackHandler();
	~LegacyTrackHandler();

	void Draw(Shader::IProgramObject* shader);

public:
	//CEventClient
	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "SunChanged") ||
			(eventName == "RenderUnitDestroyed") ||
			(eventName == "UnitMoved");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void SunChanged();
	void RenderUnitDestroyed(const CUnit*);
	void UnitMoved(const CUnit* unit);

private:
	bool GetDrawTracks() const;

	void LoadDecalShaders();
	void DrawObjectDecals();

	static void BindTextures();
	static void KillTextures();
	void BindShader(const float3& ambientColor);

	void AddTracks();
	void DrawTracks(GL::RenderDataBufferTC* buffer, Shader::IProgramObject* shader);
	void CleanTracks();


	void RemoveTrack(CUnit* unit);
	void AddTrack(const CUnit* unit, const float3& newPos);
	void CreateOrAddTrackPart(const CUnit* unit, const SolidObjectDecalDef& decalDef, const float3& pos);

	int GetTrackType(const SolidObjectDecalDef& def);
	unsigned int LoadTexture(const std::string& name);

private:
	struct TrackPart {
		float3 pos1;
		float3 pos2;

		float texPos = 0.0f;

		unsigned int creationTime = 0;

		bool connected = false;
	};

	struct UnitTrack {
		UnitTrack(                    ) = default;
		UnitTrack(const UnitTrack&  ut) = delete;
		UnitTrack(      UnitTrack&& ut) { *this = std::move(ut); }

		UnitTrack& operator = (const UnitTrack&  ut) = delete;
		UnitTrack& operator = (      UnitTrack&& ut) {
			owner = ut.owner;
			ut.owner = nullptr;

			id = ut.id;
			lastUpdate = ut.lastUpdate;
			lifeTime = ut.lifeTime;

			alphaFalloff = ut.alphaFalloff;

			parts = std::move(ut.parts);
			return *this;
		}

		const CUnit* owner = nullptr;

		unsigned int id = -1u;
		unsigned int lastUpdate = 0;
		unsigned int lifeTime = 0;

		float alphaFalloff = 0.0f;

		std::deque<TrackPart> parts;
	};

	struct TrackType {
		TrackType(const std::string& s, unsigned t): name(s), texture(t) {}

		std::string name;
		std::vector<unsigned int> trackIDs;

		unsigned int texture;
	};
	struct TrackToClean {
		TrackToClean(unsigned int id, unsigned tti): trackID(id), ttIndex(tti) {}

		unsigned int trackID;
		unsigned int ttIndex;
	};

	enum DecalShaderProgram {
		DECAL_SHADER_NULL,
		DECAL_SHADER_GLSL,
		DECAL_SHADER_CURR,
		DECAL_SHADER_LAST
	};

	#ifndef USE_DECALHANDLER_STATE
	std::array<Shader::IProgramObject*, DECAL_SHADER_LAST> decalShaders;
	#endif

	std::vector<TrackType> trackTypes;
	spring::unsynced_map<int, UnitTrack> unitTracks;

	std::vector<unsigned int> updatedTrackIDs;
	std::vector<TrackToClean> cleanedTrackIDs;
};


#endif // LEGACYTRACKHANDLER_H
