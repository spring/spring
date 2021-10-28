/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include "System/float3.h"
#include "Rendering/Common/ModelDrawerData.h"
#include "Rendering/UnitDefImage.h"
#include "Game/GlobalUnsynced.h"

struct SolidObjectGroundDecal;
struct S3DModel;
class CUnitDrawer;

namespace icon {
	class CIconData;
}
namespace GL {
	struct GeometryBuffer;
}

struct GhostSolidObject {
public:
	void IncRef() { (refCount++); }
	bool DecRef() { return ((refCount--) > 1); }

public:
	SolidObjectGroundDecal* decal; //FIXME defined in legacy decal handler with a lot legacy stuff
	S3DModel* model;

	float3 pos;
	float3 dir;

	int facing; //FIXME replaced with dir-vector just legacy decal drawer uses this
	uint8_t team;
	int refCount;
	int lastDrawFrame;
};

class CUnitDrawerData : public CUnitDrawerDataBase {
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) override {
		return
			eventName == "RenderUnitCreated" || eventName == "RenderUnitDestroyed" ||
			eventName == "UnitEnteredRadar"  || eventName == "UnitEnteredLos"      ||
			eventName == "UnitLeftRadar"     || eventName == "UnitLeftLos"         ||
			eventName == "PlayerChanged";
	}

	void RenderUnitCreated(const CUnit* unit, int cloaked) override;
	void RenderUnitDestroyed(const CUnit* unit) override;

	void UnitEnteredRadar(const CUnit* unit, int allyTeam) override;
	void UnitLeftRadar(const CUnit* unit, int allyTeam) override { UnitEnteredRadar(unit, allyTeam); }

	void UnitEnteredLos(const CUnit* unit, int allyTeam) override;
	void UnitLeftLos(const CUnit* unit, int allyTeam) override;

	void PlayerChanged(int playerNum) override;
public:
	struct TempDrawUnit {
		const UnitDef* unitDef;

		int team;
		int facing;
		int timeout;

		float3 pos;
		float rotation;

		bool drawAlpha;
		bool drawBorder;
	};
public:
	CUnitDrawerData(bool& mtModelDrawer_);
	virtual ~CUnitDrawerData();
public:
	void SetUnitIconDist(float dist) {
		unitIconDist = dist;
		iconLength = unitIconDist * unitIconDist * 750.0f;
	}

	// IconsAsUI
	float GetUnitIconScaleUI() const { return iconScale; }
	float GetUnitIconFadeStart() const { return iconFadeStart; }
	float GetUnitIconFadeVanish() const { return iconFadeVanish; }
	void SetUnitIconScaleUI(float scale) { iconScale = std::clamp(scale, 0.5f, 2.0f); }
	void SetUnitIconFadeStart(float scale) { iconFadeStart = std::clamp(scale, 1.0f, 10000.0f); }
	void SetUnitIconFadeVanish(float scale) { iconFadeVanish = std::clamp(scale, 1.0f, 10000.0f); }

	// *UnitDefImage
	void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName);
	void SetUnitDefImage(const UnitDef* unitDef, unsigned int texID, int xsize, int ysize);
	uint32_t GetUnitDefImage(const UnitDef* unitDef);
public:
	void Update() override;
	bool IsAlpha(const CUnit* co) const override { return co->IsCloaked(); }
public:
	void AddTempDrawUnit(const TempDrawUnit& tempDrawUnit);

	void UpdateGhostedBuildings();
	void UpdateUnitDefMiniMapIcons(const UnitDef* ud);
public:
	const std::vector<UnitDefImage>& GetUnitDefImages() const { return unitDefImages; }
	      std::vector<UnitDefImage>& GetUnitDefImages() { return unitDefImages; }

	const std::vector<TempDrawUnit>& GetTempOpaqueDrawUnits(int modelType) const { return tempOpaqueUnits[modelType]; }
	const std::vector<TempDrawUnit>& GetTempAlphaDrawUnits(int modelType) const { return  tempAlphaUnits[modelType]; }

	const std::vector<GhostSolidObject*>& GetDeadGhostBuildings(int allyTeam, int modelType) const {
		assert((unsigned)gu->myAllyTeam < deadGhostBuildings.size());
		return deadGhostBuildings[allyTeam][modelType];
	}
	const std::vector<CUnit*           >& GetLiveGhostBuildings(int allyTeam, int modelType) const {
		assert((unsigned)gu->myAllyTeam < liveGhostBuildings.size());
		return liveGhostBuildings[allyTeam][modelType];
	}

	const spring::unsynced_map<icon::CIconData*, std::vector<const CUnit*> >& GetUnitsByIcon() const { return unitsByIcon; }
protected:
	void UpdateObjectDrawFlags(CSolidObject* o) const override;
private:
	const icon::CIconData* GetUnitIcon(const CUnit* unit);

	void UpdateTempDrawUnits(std::vector<TempDrawUnit>& tempDrawUnits);

	void UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed);
	void UpdateUnitIconState(CUnit* unit);
	void UpdateUnitIconStateScreen(CUnit* unit);
	static void UpdateDrawPos(CUnit* unit);

	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const;
	//bool DrawAsIconScreen(CUnit* unit) const;
public:
	// lenghts & distances
	float unitIconDist;
	float iconLength;

	//icons
	bool iconHideWithUI = true;

	// IconsAsUI
	bool useScreenIcons = false;
	float iconZoomDist;
	float iconSizeBase = 32.0f;
	float iconScale = 1.0f;
	float iconFadeStart = 3000.0f;
	float iconFadeVanish = 1000.0f;
private:
	/// AI unit ghosts
	std::array< std::vector<TempDrawUnit>, MODELTYPE_CNT> tempOpaqueUnits;
	std::array< std::vector<TempDrawUnit>, MODELTYPE_CNT> tempAlphaUnits;

	/// buildings that were in LOS_PREVLOS when they died and not in LOS since
	std::vector<std::array<std::vector<GhostSolidObject*>, MODELTYPE_CNT>> deadGhostBuildings;
	/// buildings that left LOS but are still alive
	std::vector<std::array<std::vector<CUnit*>, MODELTYPE_CNT>> liveGhostBuildings;

	spring::unsynced_map<icon::CIconData*, std::vector<const CUnit*> > unitsByIcon;

	std::vector<UnitDefImage> unitDefImages;



	// icons
	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;



	// IconsAsUI
	static constexpr float iconSizeMult = 0.005f; // 1/200
};