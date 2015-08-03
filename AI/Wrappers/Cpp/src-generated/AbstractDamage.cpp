/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractDamage.h"

#include "IncludesSources.h"

	springai::AbstractDamage::~AbstractDamage() {}
	int springai::AbstractDamage::CompareTo(const Damage& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Damage*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetWeaponDefId() < other.GetWeaponDefId()) return BEFORE;
		if (this->GetWeaponDefId() > other.GetWeaponDefId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractDamage::Equals(const Damage& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetWeaponDefId() != other.GetWeaponDefId()) return false;
	return true;
}

int springai::AbstractDamage::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetWeaponDefId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractDamage::ToString() {

	std::string internal_ret = "Damage";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "weaponDefId=%i, ", this->GetWeaponDefId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

