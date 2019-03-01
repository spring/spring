/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <fstream>

#include "SelectionKeyHandler.h"
#include "Game/Camera/CameraController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/StringHash.h"
#include "System/UnorderedSet.hpp"

CSelectionKeyHandler selectionKeys;


std::string CSelectionKeyHandler::ReadToken(std::string& str)
{
	std::string ret;

	size_t index = 0;
	while ((index < str.length()) && (str[index] != '_') && (str[index] != '+')) {
		index++;
	}

	ret = str.substr(0, index);
	str = str.substr(index, std::string::npos);

	return ret;
}


std::string CSelectionKeyHandler::ReadDelimiter(std::string& str)
{
	std::string ret = str.substr(0, 1);
	if (!str.empty()) {
		str = str.substr(1, std::string::npos);
	} else {
		str = "";
	}
	return ret;
}


namespace {
	struct Filter {
	public:
		typedef std::pair<std::string, Filter*> Pair;
		typedef std::vector<Pair> Map;

		/// Contains all existing filter singletons.
		static Map& all(bool sort) {
			static Map map;

			if (map.empty())
				map.reserve(20);
			if (sort)
				std::sort(map.begin(), map.end(), [](const Pair& a, const Pair& b) { return (a.first < b.first); });

			return map;
		}

		virtual ~Filter() = default;

		/// Called immediately before the filter is used.
		virtual void Prepare() {}

		/// Called immediately before the filter is used for every parameter.
		virtual void SetParam(int index, const std::string& value) {
			assert(false);
		}

		/**
		 * Actual filtering, should return false if unit should be removed
		 * from proposed selection.
		 */
		virtual bool ShouldIncludeUnit(const CUnit* unit) const = 0;

		/// Number of arguments this filter has.
		const int numArgs;

	protected:
		Filter(const std::string& name, int args) : numArgs(args) {
			all(false).emplace_back(name, this);
		}
	};



	// prototype / factory based approach might be better at some point?
	// for now these singleton filters seem ok. (they are not reentrant tho!)

#define DECLARE_FILTER_EX(name, args, condition, extra, init) \
	struct name ## _Filter : public Filter { \
		name ## _Filter() : Filter(#name, args) { init; } \
		bool ShouldIncludeUnit(const CUnit* unit) const override { return condition; } \
		extra \
	} name ## _filter_instance; \

#define DECLARE_FILTER(name, condition) \
	DECLARE_FILTER_EX(name, 0, condition, ,)


	DECLARE_FILTER(Builder, unit->unitDef->IsBuilderUnit())
	DECLARE_FILTER(Building, unit->unitDef->IsBuildingUnit())
	DECLARE_FILTER(Transport, unit->unitDef->IsTransportUnit())
	DECLARE_FILTER(Aircraft, unit->unitDef->IsAirUnit())
	DECLARE_FILTER(Weapons, !unit->weapons.empty())
	DECLARE_FILTER(Idle, unit->commandAI->commandQue.empty())
	DECLARE_FILTER(Waiting, !unit->commandAI->commandQue.empty() &&
	               (unit->commandAI->commandQue.front().GetID() == CMD_WAIT))
	DECLARE_FILTER(InHotkeyGroup, unit->group != nullptr)
	DECLARE_FILTER(Radar, unit->radarRadius || unit->sonarRadius || unit->jammerRadius)
	DECLARE_FILTER(ManualFireUnit, unit->unitDef->canManualFire)

	DECLARE_FILTER_EX(WeaponRange, 1, unit->maxRange > minRange,
		float minRange;
		void SetParam(int index, const std::string& value) override {
			minRange = atof(value.c_str());
		},
		minRange = 0.0f;
	)

	DECLARE_FILTER_EX(AbsoluteHealth, 1, unit->health > minHealth,
		float minHealth;
		void SetParam(int index, const std::string& value) override {
			minHealth = atof(value.c_str());
		},
		minHealth = 0.0f;
	)

	DECLARE_FILTER_EX(RelativeHealth, 1, unit->health / unit->maxHealth > minHealth,
		float minHealth;
		void SetParam(int index, const std::string& value) override {
			minHealth = atof(value.c_str()) * 0.01f; // convert from percent
		},
		minHealth = 0.0f;
	)

	DECLARE_FILTER_EX(InPrevSel, 0, prevTypes.find(unit->unitDef->id) != prevTypes.end(),
		spring::unordered_set<int> prevTypes;
		void Prepare() override {
			const auto& selUnits = selectedUnitsHandler.selectedUnits;

			prevTypes.clear();
			prevTypes.reserve(selUnits.size());

			for (const int unitID: selUnits) {
				const CUnit* u = unitHandler.GetUnit(unitID);
				const UnitDef* ud = u->unitDef;
				prevTypes.insert(ud->id);
			}
		},
	)

	DECLARE_FILTER_EX(NameContain, 1, unit->unitDef->humanName.find(name) != std::string::npos,
		std::string name;
		void SetParam(int index, const std::string& value) override {
			name = value;
		},
	)

	DECLARE_FILTER_EX(Category, 1, unit->category == cat,
		unsigned int cat;
		void SetParam(int index, const std::string& value) override {
			cat = CCategoryHandler::Instance()->GetCategory(value);
		},
		cat = 0;
	)
//FIXME: std::strtof is in C99 which M$ doesn't bother to support.
#ifdef _MSC_VER
	#define STRTOF strtod
#else
	#define STRTOF strtof
#endif

	DECLARE_FILTER_EX(RulesParamEquals, 2, unit->modParams.find(param) != unit->modParams.end() &&
			((wantedValueStr.empty()) ? unit->modParams.find(param)->second.valueInt == wantedValue
			: unit->modParams.find(param)->second.valueString == wantedValueStr),
		std::string param;
		std::string wantedValueStr;

		float wantedValue;

		void SetParam(int index, const std::string& value) override {
			switch (index) {
				case 0: {
					param = value;
				} break;
				case 1: {
					const char* cstr = value.c_str();
					char* endNumPos = nullptr;
					wantedValue = STRTOF(cstr, &endNumPos);
					if (endNumPos == cstr) wantedValueStr = value;
				} break;
			}
		},
		wantedValue = 0.0f;
	)

#undef DECLARE_FILTER_EX
#undef DECLARE_FILTER
#undef STRTOF
}



void CSelectionKeyHandler::DoSelection(std::string selectString)
{
	selection.clear();

	std::string s = std::move(ReadToken(selectString));

	switch (hashString(s.c_str())) {
		case hashString("AllMap"): {
			if (!gu->spectatingFullSelect) {
				// team units
				selection.reserve(unitHandler.NumUnitsByTeam(gu->myTeam));

				for (CUnit* unit: unitHandler.GetUnitsByTeam(gu->myTeam)) {
					selection.push_back(unit);
				}
			} else {
				// all units
				selection.reserve((unitHandler.GetActiveUnits()).size());

				for (CUnit* unit: unitHandler.GetActiveUnits()) {
					selection.push_back(unit);
				}
			}
		} break;

		case hashString("Visible"): {
			if (!gu->spectatingFullSelect) {
				// team units in viewport
				selection.reserve(unitHandler.NumUnitsByTeam(gu->myTeam));

				for (CUnit* unit: unitHandler.GetUnitsByTeam(gu->myTeam)) {
					if (!camera->InView(unit->midPos, unit->radius))
						continue;

					selection.push_back(unit);
				}
			} else {
			  // all units in viewport
				selection.reserve((unitHandler.GetActiveUnits()).size());

				for (CUnit* unit: unitHandler.GetActiveUnits()) {
					if (!camera->InView(unit->midPos, unit->radius))
						continue;

					selection.push_back(unit);
				}
			}
		} break;

		case hashString("FromMouse"):
		case hashString("FromMouseC"): {
			// FromMouse uses distance from a point on the ground,
			// so essentially a selection sphere.
			// FromMouseC uses a cylinder shaped volume for selection,
			// so the heights of the units do not matter.
			const bool cylindrical = (s.back() == 'C');

			ReadDelimiter(selectString);

			const float maxDist = Square(atof(ReadToken(selectString).c_str()));
			const float gndDist = CGround::LineGroundCol(camera->GetPos(), camera->GetPos() + mouse->dir * camera->GetFarPlaneDist(), false);

			float3 mp = camera->GetPos() + mouse->dir * gndDist;

			if (cylindrical)
				mp.y = 0.0f;

			if (!gu->spectatingFullSelect) {
				// team units in mouse range
				selection.reserve(unitHandler.NumUnitsByTeam(gu->myTeam));

				for (CUnit* unit: unitHandler.GetUnitsByTeam(gu->myTeam)) {
					float3 up = unit->pos;

					if (cylindrical)
						up.y = 0.0f;

					if (mp.SqDistance(up) >= maxDist)
						continue;

					selection.push_back(unit);
				}
			} else {
				// all units in mouse range
				selection.reserve((unitHandler.GetActiveUnits()).size());

				for (CUnit* unit: unitHandler.GetActiveUnits()) {
					float3 up = unit->pos;

					if (cylindrical)
						up.y = 0.0f;

					if (mp.SqDistance(up) >= maxDist)
						continue;

					selection.push_back(unit);
				}
			}
		} break;

		case hashString("PrevSelection"): {
			selection.reserve(selectedUnitsHandler.selectedUnits.size());

			for (const int unitID: selectedUnitsHandler.selectedUnits) {
				selection.push_back(unitHandler.GetUnit(unitID));
			}
		} break;

		default: {
			LOG_L(L_WARNING, "Unknown source token %s", s.c_str());
			return;
		} break;
	}

	ReadDelimiter(selectString);

	while (true) {
		std::string filter = std::move(ReadDelimiter(selectString));

		if (filter == "+")
			break;

		filter = std::move(ReadToken(selectString));

		bool _not = false;

		if (filter == "Not") {
			_not = true;
			ReadDelimiter(selectString);
			filter = std::move(ReadToken(selectString));
		}


		using FilterPair = Filter::Pair;

		const Filter::Map& filters = Filter::all((numDoSelects++) == 0);

		const auto pred = [](const FilterPair& a, const FilterPair& b) { return (a.first < b.first); };
		const auto iter = std::lower_bound(filters.begin(), filters.end(), FilterPair{filter, nullptr}, pred);

		if (iter != filters.end() && iter->first == filter) {
			iter->second->Prepare();

			for (int i = 0; i < iter->second->numArgs; ++i) {
				ReadDelimiter(selectString);
				iter->second->SetParam(i, ReadToken(selectString));
			}

			auto ui = selection.begin();

			while (ui != selection.end()) {
				if (iter->second->ShouldIncludeUnit(*ui) ^ _not) {
					++ui;
				} else {
					// erase, order is not relevant
					*ui = selection.back();
					selection.pop_back();
				}
			}
		} else {
			LOG_L(L_WARNING, "[%s] unknown token in filter \"%s\"", __func__, filter.c_str());
			return;
		}
	}

	ReadDelimiter(selectString);
	s = std::move(ReadToken(selectString));

	if (s == "ClearSelection") {
		selectedUnitsHandler.ClearSelected();

		ReadDelimiter(selectString);
		s = ReadToken(selectString);
	}

	switch (hashString(s.c_str())) {
		case hashString("SelectAll"): {
			for (CUnit* u: selection) {
				selectedUnitsHandler.AddUnit(u);
			}

			return;
		} break;

		case hashString("SelectOne"): {
			if (selection.empty())
				return;
			if (++selectNumber >= selection.size())
				selectNumber = 0;

			CUnit* sel = selection[selectNumber];

			if (sel == nullptr)
				return;

			selectedUnitsHandler.AddUnit(sel);
			camHandler->CameraTransition(0.8f);

			if (camHandler->GetCurrentControllerNum() != CCameraHandler::CAMERA_MODE_FIRSTPERSON) {
				camHandler->GetCurrentController().SetPos(sel->pos);
			} else {
				// FPS camera
				if (camera->GetRot().x > -1.0f)
					camera->SetRotX(-1.0f);

				camHandler->GetCurrentController().SetPos(sel->pos - camera->GetDir() * 800.0f);
			}

			return;
		} break;

		case hashString("SelectNum"): {
			ReadDelimiter(selectString);
			const int num = atoi(ReadToken(selectString).c_str());

			if (selection.empty())
				return;

			if (selectNumber >= selection.size())
				selectNumber = 0;

			auto ui = selection.begin() + selectNumber;

			for (int a = 0; a < num; ++ui, ++a) {
				if (ui == selection.end())
					ui = selection.begin();

				selectedUnitsHandler.AddUnit(*ui);
			}

			selectNumber += num;
			return;
		} break;

		case hashString("SelectPart"): {
			ReadDelimiter(selectString);

			const float part = atof(ReadToken(selectString).c_str()) * 0.01f;//convert from percent
			const int num = (int)(selection.size() * part);

			if (selection.empty())
				return;

			if (selectNumber >= selection.size())
				selectNumber = 0;

			auto ui = selection.begin() + selectNumber;

			for (int a = 0; a < num; ++ui, ++a) {
				if (ui == selection.end())
					ui = selection.begin();

				selectedUnitsHandler.AddUnit(*ui);
			}

			selectNumber += num;
			return;
		} break;

		default: {
			LOG_L(L_WARNING, "[%s] unknown token in conclusion \"%s\"", __func__, s.c_str());
		} break;
	}
}

