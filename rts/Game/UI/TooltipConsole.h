/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TOOL_TIP_CONSOLE_H
#define TOOL_TIP_CONSOLE_H

#include "InputReceiver.h"
#include <string>

class CUnit;
class CFeature;
class float3;

class CTooltipConsole : public CInputReceiver {
	public:
		CTooltipConsole();
		~CTooltipConsole();

		void Draw();
		bool IsAbove(int x, int y);

		// helpers
		static std::string MakeUnitString(const CUnit* unit);
		static std::string MakeFeatureString(const CFeature* feature);
		static std::string MakeGroundString(const float3& pos);
		static std::string MakeUnitStatsString(
			float health, float maxHealth,
			float currentFuel, float maxFuel,
			float experience, float cost, float maxRange,
			float metalMake,  float metalUse,
			float energyMake, float energyUse);

		bool enabled;

	protected:
		float x, y, w, h;
		bool outFont;
};

extern CTooltipConsole* tooltip;

#endif /* TOOL_TIP_CONSOLE_H */
