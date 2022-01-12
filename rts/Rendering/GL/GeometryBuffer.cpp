/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GeometryBuffer.h"
#include "RenderDataBuffer.hpp"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"

#include <algorithm>
#include <cstring> // memset

void GL::GeometryBuffer::Init(bool ctor) {
	// if dead, this must be a non-ctor reload
	assert(!dead || !ctor);

	memset(&bufferTextureIDs[0], 0, sizeof(bufferTextureIDs));
	memset(&bufferAttachments[0], 0, sizeof(bufferAttachments));

	// NOTE:
	//   initial buffer size must be 0 s.t. prevSize != currSize when !init
	//   (Lua can toggle drawDeferred and might be the first to cause a call
	//   to Create)
	prevBufferSize = GetWantedSize(false);
	currBufferSize = GetWantedSize(true);

	dead = false;
	bound = false;
	msaa = configHandler->GetBool("AllowMultiSampledFrameBuffers");
}

void GL::GeometryBuffer::Kill(bool dtor) {
	if (dead) {
		// if already dead, this must be final cleanup
		assert(dtor);
		return;
	}

	if (buffer.IsValid())
		DetachTextures(false);

	dead = true;
}

void GL::GeometryBuffer::Clear() const {
	assert(bound);
	glAttribStatePtr->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glAttribStatePtr->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GL::GeometryBuffer::SetDepthRange(float nearDepth, float farDepth) const {
	#if 0
	if (globalRendering->supportClipSpaceControl) {
		// TODO: need to inform shaders about this, modify PM instead
		glAttribStatePtr->DepthRange(nearDepth, farDepth);
		glAttribStatePtr->ClearDepth(farDepth);
		glAttribStatePtr->DepthFunc((nearDepth <= farDepth)? GL_LEQUAL: GL_GREATER);
	}
	#else
	glAttribStatePtr->ClearDepth(std::max(nearDepth, farDepth));
	glAttribStatePtr->DepthFunc(GL_LEQUAL);
	#endif
}

void GL::GeometryBuffer::DetachTextures(const bool init) {
	// nothing to detach yet during init
	if (init)
		return;

	buffer.Bind();

	// detach only actually attached textures, ATI drivers might crash
	for (unsigned int i = 0; i < (ATTACHMENT_COUNT - 1); ++i) {
		buffer.Detach(GL_COLOR_ATTACHMENT0 + i);
	}

	buffer.Detach(GL_DEPTH_ATTACHMENT);
	buffer.Unbind();

	glDeleteTextures(ATTACHMENT_COUNT, &bufferTextureIDs[0]);

	// return to incomplete state
	memset(&bufferTextureIDs[0], 0, sizeof(bufferTextureIDs));
	memset(&bufferAttachments[0], 0, sizeof(bufferAttachments));
}

void GL::GeometryBuffer::DrawDebug(const unsigned int texID, const float2 texMins, const float2 texMaxs) const {
	GL::RenderDataBuffer2DT* buffer = GL::GetRenderBuffer2DT();
	Shader::IProgramObject* shader = buffer->GetShader();

	// FIXME: needs resolve or different shader if msaa
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GetTextureTarget(), texID);

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::Identity());

	buffer->SafeAppend({texMins.x, texMins.y,  texMins.x, texMins.y}); // tl
	buffer->SafeAppend({texMaxs.x, texMins.y,  texMaxs.x, texMins.y}); // tr
	buffer->SafeAppend({texMaxs.x, texMaxs.y,  texMaxs.x, texMaxs.y}); // br

	buffer->SafeAppend({texMaxs.x, texMaxs.y,  texMaxs.x, texMaxs.y}); // br
	buffer->SafeAppend({texMins.x, texMaxs.y,  texMins.x, texMaxs.y}); // bl
	buffer->SafeAppend({texMins.x, texMins.y,  texMins.x, texMins.y}); // tl

	buffer->Submit(GL_TRIANGLES);
	shader->Disable();

	glBindTexture(GetTextureTarget(), 0);
}

bool GL::GeometryBuffer::Create(const int2 size) {
	const unsigned int texTarget = GetTextureTarget();

	for (unsigned int n = 0; n < ATTACHMENT_COUNT; n++) {
		glGenTextures(1, &bufferTextureIDs[n]);
		glBindTexture(texTarget, bufferTextureIDs[n]);

		glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (n == ATTACHMENT_ZVALTEX) {
			if (texTarget == GL_TEXTURE_2D)
				glTexImage2D(texTarget, 0, GL_DEPTH_COMPONENT32F, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			else
				glTexImage2DMultisample(texTarget, globalRendering->msaaLevel, GL_DEPTH_COMPONENT32F, size.x, size.y, GL_TRUE);

			bufferAttachments[n] = GL_DEPTH_ATTACHMENT;
		} else {
			if (texTarget == GL_TEXTURE_2D)
				glTexImage2D(texTarget, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			else
				glTexImage2DMultisample(texTarget, globalRendering->msaaLevel, GL_RGBA8, size.x, size.y, GL_TRUE);

			bufferAttachments[n] = GL_COLOR_ATTACHMENT0 + n;
		}
	}

	// sic; Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
	buffer.Bind();
	buffer.AttachTextures(bufferTextureIDs, bufferAttachments, texTarget, ATTACHMENT_COUNT);

	glBindTexture(texTarget, 0);
	// define the attachments we are going to draw into
	// note: the depth-texture attachment does not count
	// here and will be GL_NONE implicitly!
	glDrawBuffers(ATTACHMENT_COUNT - 1, &bufferAttachments[0]);

	// FBO must have been valid from point of construction
	// if we reached CreateGeometryBuffer, but CheckStatus
	// can still invalidate it
	assert(buffer.IsValid());

	const bool ret = buffer.CheckStatus(name);

	buffer.Unbind();
	return ret;
}

bool GL::GeometryBuffer::Update(const bool init) {
	currBufferSize = GetWantedSize(true);

	// FBO must be valid from point of construction
	if (!buffer.IsValid())
		return false;

	// buffer isn't bound by calling context, can not call
	// GetStatus to check for GL_FRAMEBUFFER_COMPLETE here
	//
	if (HasAttachments()) {
		// technically a buffer can not be complete yet during
		// initialization, however the GL spec says that FBO's
		// with only empty attachments are complete by default
		// assert(!init);

		// FBO was already initialized (during init or from Lua)
		// so it will have attachments -> check if they need to
		// be regenerated, eg. if a window resize event happened
		if (prevBufferSize == currBufferSize)
			return true;

		DetachTextures(init);
	}

	return (Create(prevBufferSize = currBufferSize));
}

int2 GL::GeometryBuffer::GetWantedSize(bool allowed) const {
	return {globalRendering->viewSizeX * allowed, globalRendering->viewSizeY * allowed};
}

