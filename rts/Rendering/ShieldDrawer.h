/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#ifndef SHIELD_DRAWER_H
#define SHIELD_DRAWER_H

#include <set>
#include "System/EventClient.h"
#include "System/float3.h"

class CUnit;
class CPlasmaRepulser;
struct WeaponDef;

class CVertexArray;
struct AtlasedTexture;



struct ShieldSegment {
public:
	ShieldSegment(
		const WeaponDef* shieldWeaponDef,
		const float3& shieldSegmentPos,
		const int xpart,
		const int ypart
	);
	~ShieldSegment();

	void Draw(CPlasmaRepulser* shield, CVertexArray* va);

private:
	float3 segmentPos;
	float3 segmentColor;

	float3 vertices[25];
	float3 texCoors[25];

	AtlasedTexture* texture;

	float segmentSize;
	float segmentAlpha;
	bool usePerlinTex;
};



class ShieldDrawer: public CEventClient {
public:
	ShieldDrawer();
	~ShieldDrawer();

	void Draw();

public:
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "RenderUnitCreated" || eventName == "RenderUnitDestroyed");
	}

	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderUnitCreated(const CUnit* u, int);
	void RenderUnitDestroyed(const CUnit* u);

private:
	void DrawUnitShields(const CUnit*, CVertexArray* va);
	void DrawUnitShield(CPlasmaRepulser*, CVertexArray* va);

	// all active units with at least one VISIBLE shield-weapon
	std::set<const CUnit*> units;
};

extern ShieldDrawer* shieldDrawer;

#endif
