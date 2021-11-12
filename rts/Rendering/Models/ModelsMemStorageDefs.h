#pragma once

#include <array>

#include "System/MemPoolTypes.h"
#include "System/float4.h"
#include "Sim/Misc/GlobalConstants.h"

constexpr static int MAX_MODEL_UD_UNIFORMS = 20;
struct alignas(4) ModelUniformData {
	union {
		uint32_t composite;
		struct {
			uint8_t drawFlag;
			uint8_t unused1;
			uint16_t id;
		};
	};

	uint32_t unused2;
	uint32_t unused3;
	uint32_t unused4;

	float maxHealth;
	float health;
	float unused5;
	float unused6;

	float4 speed;

	std::array<float, MAX_MODEL_UD_UNIFORMS> userDefined;
};

static_assert(sizeof(ModelUniformData) == 128, "");
static_assert(alignof(ModelUniformData) == 4, "");
static_assert(sizeof(ModelUniformData::userDefined) % 4 == 0, ""); //due to GLSL std140 userDefined must be a mutiple of 4