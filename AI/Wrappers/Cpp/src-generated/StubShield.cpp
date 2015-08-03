/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubShield.h"

#include "IncludesSources.h"

	springai::StubShield::~StubShield() {}
	void springai::StubShield::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubShield::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubShield::SetWeaponDefId(int weaponDefId) {
		this->weaponDefId = weaponDefId;
	}
	int springai::StubShield::GetWeaponDefId() const {
		return weaponDefId;
	}

	float springai::StubShield::GetResourceUse(Resource* resource) {
		return 0;
	}

	void springai::StubShield::SetRadius(float radius) {
		this->radius = radius;
	}
	float springai::StubShield::GetRadius() {
		return radius;
	}

	void springai::StubShield::SetForce(float force) {
		this->force = force;
	}
	float springai::StubShield::GetForce() {
		return force;
	}

	void springai::StubShield::SetMaxSpeed(float maxSpeed) {
		this->maxSpeed = maxSpeed;
	}
	float springai::StubShield::GetMaxSpeed() {
		return maxSpeed;
	}

	void springai::StubShield::SetPower(float power) {
		this->power = power;
	}
	float springai::StubShield::GetPower() {
		return power;
	}

	void springai::StubShield::SetPowerRegen(float powerRegen) {
		this->powerRegen = powerRegen;
	}
	float springai::StubShield::GetPowerRegen() {
		return powerRegen;
	}

	float springai::StubShield::GetPowerRegenResource(Resource* resource) {
		return 0;
	}

	void springai::StubShield::SetStartingPower(float startingPower) {
		this->startingPower = startingPower;
	}
	float springai::StubShield::GetStartingPower() {
		return startingPower;
	}

	void springai::StubShield::SetRechargeDelay(int rechargeDelay) {
		this->rechargeDelay = rechargeDelay;
	}
	int springai::StubShield::GetRechargeDelay() {
		return rechargeDelay;
	}

	void springai::StubShield::SetGoodColor(springai::AIColor goodColor) {
		this->goodColor = goodColor;
	}
	springai::AIColor springai::StubShield::GetGoodColor() {
		return goodColor;
	}

	void springai::StubShield::SetBadColor(springai::AIColor badColor) {
		this->badColor = badColor;
	}
	springai::AIColor springai::StubShield::GetBadColor() {
		return badColor;
	}

	void springai::StubShield::SetAlpha(short alpha) {
		this->alpha = alpha;
	}
	short springai::StubShield::GetAlpha() {
		return alpha;
	}

	void springai::StubShield::SetInterceptType(int interceptType) {
		this->interceptType = interceptType;
	}
	int springai::StubShield::GetInterceptType() {
		return interceptType;
	}

