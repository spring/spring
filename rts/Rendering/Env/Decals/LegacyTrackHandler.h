/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LEGACYTRACKHANDLER_H
#define LEGACYTRACKHANDLER_H


#include <deque>
#include <vector>
#include <string>

#include "System/float3.h"
#include "System/EventClient.h"



class CSolidObject;
class CUnit;
class CVertexArray;

namespace Shader {
	struct IProgramObject;
}

struct TrackPart {
	TrackPart()
		: texPos(0.0f)
		, connected(false)
		, isNewTrack(false)
		, creationTime(0)
	{}
	float3 pos1;
	float3 pos2;
	float texPos;
	bool connected;
	bool isNewTrack;
	unsigned int creationTime;
};

struct UnitTrackStruct {
	UnitTrackStruct(CUnit* owner)
		: owner(owner)
		, lastUpdate(0)
		, lifeTime(0)
		, alphaFalloff(0.0f)
	{}

	CUnit* owner;

	unsigned int lastUpdate;
	unsigned int lifeTime;

	float alphaFalloff;

	TrackPart lastAdded;
	std::deque<TrackPart> parts;
};

struct TrackToClean {
	TrackToClean(UnitTrackStruct* t, std::vector<UnitTrackStruct*>* ts)
		: track(t)
		, tracks(ts)
	{}

	UnitTrackStruct* track;
	std::vector<UnitTrackStruct*>* tracks;
};



class LegacyTrackHandler: public CEventClient
{
public:
	LegacyTrackHandler();
	~LegacyTrackHandler();

	void Draw();

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
	void DrawTracks();
	void CleanTracks();


	static void RemoveTrack(CUnit* unit);
	void AddTrack(CUnit* unit, const float3& newPos);
	int GetTrackType(const std::string& name);
	unsigned int LoadTexture(const std::string& name);

private:
	struct TrackType {
		TrackType(const std::string& s, unsigned t): name(s), texture(t) {}
		std::string name;
		std::vector<UnitTrackStruct*> tracks;
		unsigned int texture;
	};

	std::vector<TrackType> trackTypes;

	enum DecalShaderProgram {
		DECAL_SHADER_ARB,
		DECAL_SHADER_GLSL,
		DECAL_SHADER_CURR,
		DECAL_SHADER_LAST
	};

	std::vector<Shader::IProgramObject*> decalShaders;

	std::vector<UnitTrackStruct*> tracksToBeAdded;
	std::vector<TrackToClean>     tracksToBeCleaned;
	std::vector<UnitTrackStruct*> tracksToBeDeleted;
};


#endif // LEGACYTRACKHANDLER_H
