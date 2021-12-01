#include "CustomMaterial.h"
#include "System/HashSpec.h"

std::size_t CustomMaterial::GetHash() const {
	if (hash > 0)
		return hash;

	hash = 1337;
	for (const auto& rpd : renderPassesData) {
		hash = spring::hash_combine(rpd.GetHash());
	}

	return hash;
}

bool CustomMaterial::operator==(const CustomMaterial& rhs) const {
	if (GetHash() != rhs.GetHash())
		return false;

	for (int i = 0; i < RenderPassType::RPT_TYPE_COUNT; ++i) {
		if (renderPassesData[i] != rhs.renderPassesData[i])
			return false;
	}

	return true;
}