/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HUD_DRAWER_HDR
#define HUD_DRAWER_HDR

class CUnit;
struct HUDDrawer {
public:
	HUDDrawer(): draw(true) {}
	void Draw(const CUnit*);

	void SetDraw(bool b) { draw = b; }
	bool GetDraw() const { return draw; }

	static HUDDrawer* GetInstance();

private:
	void DrawModel(const CUnit*);
	void DrawUnitDirectionArrow(const CUnit*);
	void DrawCameraDirectionArrow(const CUnit*);
	void DrawWeaponStates(const CUnit*);
	void DrawTargetReticle(const CUnit*);

	bool draw;
};

#define hudDrawer (HUDDrawer::GetInstance())

#endif
