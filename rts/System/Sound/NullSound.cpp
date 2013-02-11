/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NullSound.h"

#include "SoundLog.h"
#include "System/float3.h"


NullSound::NullSound() {
}

NullSound::~NullSound() {
}

bool NullSound::HasSoundItem(const std::string& name) const {
	return false;
}

size_t NullSound::GetSoundId(const std::string& name) {
	return 0;
}

CSoundSource* NullSound::GetNextBestSource(bool lock) {
	return NULL;
}

void NullSound::PitchAdjust(const float newPitch) {
}

void NullSound::ConfigNotify(const std::string& key, const std::string& value) {
}

bool NullSound::Mute() {
	return true;
}

bool NullSound::IsMuted() const {
	return true;
}

void NullSound::Iconified(bool state) {
}

void NullSound::UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime) {
}

void NullSound::PrintDebugInfo() {
	LOG("Null Sound System");
}

bool NullSound::LoadSoundDefs(const std::string& fileName) {
	return false;
}

void NullSound::NewFrame() {
}

const float3& NullSound::GetListenerPos() const {
	return ZeroVector;
}
