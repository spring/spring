/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubDamage.h"

#include "IncludesSources.h"

	springai::StubDamage::~StubDamage() {}
	void springai::StubDamage::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubDamage::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubDamage::SetWeaponDefId(int weaponDefId) {
		this->weaponDefId = weaponDefId;
	}
	int springai::StubDamage::GetWeaponDefId() const {
		return weaponDefId;
	}

	void springai::StubDamage::SetParalyzeDamageTime(int paralyzeDamageTime) {
		this->paralyzeDamageTime = paralyzeDamageTime;
	}
	int springai::StubDamage::GetParalyzeDamageTime() {
		return paralyzeDamageTime;
	}

	void springai::StubDamage::SetImpulseFactor(float impulseFactor) {
		this->impulseFactor = impulseFactor;
	}
	float springai::StubDamage::GetImpulseFactor() {
		return impulseFactor;
	}

	void springai::StubDamage::SetImpulseBoost(float impulseBoost) {
		this->impulseBoost = impulseBoost;
	}
	float springai::StubDamage::GetImpulseBoost() {
		return impulseBoost;
	}

	void springai::StubDamage::SetCraterMult(float craterMult) {
		this->craterMult = craterMult;
	}
	float springai::StubDamage::GetCraterMult() {
		return craterMult;
	}

	void springai::StubDamage::SetCraterBoost(float craterBoost) {
		this->craterBoost = craterBoost;
	}
	float springai::StubDamage::GetCraterBoost() {
		return craterBoost;
	}

	void springai::StubDamage::SetTypes(std::vector<float> types) {
		this->types = types;
	}
	std::vector<float> springai::StubDamage::GetTypes() {
		return types;
	}

