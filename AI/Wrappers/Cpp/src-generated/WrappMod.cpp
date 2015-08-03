/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappMod.h"

#include "IncludesSources.h"

	springai::WrappMod::WrappMod(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappMod::~WrappMod() {

	}

	int springai::WrappMod::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappMod::Mod* springai::WrappMod::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Mod* internal_ret = NULL;
		internal_ret = new springai::WrappMod(skirmishAIId);
		return internal_ret;
	}


	const char* springai::WrappMod::GetFileName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Mod_getFileName(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetHash() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getHash(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMod::GetHumanName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Mod_getHumanName(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMod::GetShortName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Mod_getShortName(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMod::GetVersion() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Mod_getVersion(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMod::GetMutator() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Mod_getMutator(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	const char* springai::WrappMod::GetDescription() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Mod_getDescription(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappMod::GetAllowTeamColors() {

		bool internal_ret_int;

		internal_ret_int = bridged_Mod_getAllowTeamColors(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappMod::GetConstructionDecay() {

		bool internal_ret_int;

		internal_ret_int = bridged_Mod_getConstructionDecay(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetConstructionDecayTime() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getConstructionDecayTime(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetConstructionDecaySpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getConstructionDecaySpeed(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetMultiReclaim() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getMultiReclaim(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetReclaimMethod() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimMethod(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetReclaimUnitMethod() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimUnitMethod(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetReclaimUnitEnergyCostFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimUnitEnergyCostFactor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetReclaimUnitEfficiency() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimUnitEfficiency(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetReclaimFeatureEnergyCostFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimFeatureEnergyCostFactor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappMod::GetReclaimAllowEnemies() {

		bool internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimAllowEnemies(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappMod::GetReclaimAllowAllies() {

		bool internal_ret_int;

		internal_ret_int = bridged_Mod_getReclaimAllowAllies(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetRepairEnergyCostFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getRepairEnergyCostFactor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetResurrectEnergyCostFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getResurrectEnergyCostFactor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetCaptureEnergyCostFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getCaptureEnergyCostFactor(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetTransportGround() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getTransportGround(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetTransportHover() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getTransportHover(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetTransportShip() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getTransportShip(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetTransportAir() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getTransportAir(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetFireAtKilled() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getFireAtKilled(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetFireAtCrashing() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getFireAtCrashing(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetFlankingBonusModeDefault() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getFlankingBonusModeDefault(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetLosMipLevel() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getLosMipLevel(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappMod::GetAirMipLevel() {

		int internal_ret_int;

		internal_ret_int = bridged_Mod_getAirMipLevel(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetLosMul() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getLosMul(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	float springai::WrappMod::GetAirLosMul() {

		float internal_ret_int;

		internal_ret_int = bridged_Mod_getAirLosMul(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappMod::GetRequireSonarUnderWater() {

		bool internal_ret_int;

		internal_ret_int = bridged_Mod_getRequireSonarUnderWater(this->GetSkirmishAIId());
		return internal_ret_int;
	}
