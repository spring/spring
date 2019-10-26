/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GeometryBuffer.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"

#include <algorithm>
#include <cstring> //memset


CONFIG(bool, AllowDeferredMapRendering).defaultValue(false).safemodeValue(false).description("Deprecated. Set AllowDeferredRendering instead.");
CONFIG(bool, AllowDeferredModelRendering).defaultValue(false).safemodeValue(false).description("Deprecated. Set AllowDeferredRendering instead.");
CONFIG(bool, AllowDeferredRendering).defaultValue(false).safemodeValue(false).description("Render terrain and models into the deferred buffers");

CONFIG(bool, AllowMultiSampledFrameBuffers).defaultValue(false).safemodeValue(false).description("Create multisampled deferred buffers");

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

	enabled =
		configHandler->GetBool("AllowDeferredRendering") ||
		configHandler->GetBool("AllowDeferredMapRendering") ||
		configHandler->GetBool("AllowDeferredModelRendering");

	msaa |= configHandler->GetBool("AllowMultiSampledFrameBuffers");
	msaa &= globalRendering->supportMSAAFrameBuffer;
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
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GL::GeometryBuffer::SetDepthRange(float nearDepth, float farDepth) const {
	#if 0
	if (globalRendering->supportClipSpaceControl) {
		// TODO: need to inform shaders about this, modify PM instead
		glDepthRangef(nearDepth, farDepth);
		glClearDepth(farDepth);
		glDepthFunc((nearDepth <= farDepth)? GL_LEQUAL: GL_GREATER);
	}
	#else
	glClearDepth(std::max(nearDepth, farDepth));
	glDepthFunc(GL_LEQUAL);
	#endif
}

void GL::GeometryBuffer::DetachTextures(const bool init) {
	// nothing to detach yet during init
	if (init)
		return;

	buffer.Bind();

	// detach only actually attached textures, ATI drivers might crash
	for (unsigned int i = 0; i < (ATTACHMENT_COUNT - 1); ++i) {
		buffer.Detach(GL_COLOR_ATTACHMENT0_EXT + i);
	}

	buffer.Detach(GL_DEPTH_ATTACHMENT_EXT);
	buffer.Unbind();

	glDeleteTextures(ATTACHMENT_COUNT, &bufferTextureIDs[0]);

	// return to incomplete state
	memset(&bufferTextureIDs[0], 0, sizeof(bufferTextureIDs));
	memset(&bufferAttachments[0], 0, sizeof(bufferAttachments));

	valid = false;
}

void GL::GeometryBuffer::DrawDebug(const unsigned int texID, const float2 texMins, const float2 texMaxs) const {
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glActiveTexture(GL_TEXTURE0);
	glEnable(GetTextureTarget());
	glBindTexture(GetTextureTarget(), texID);
	glBegin(GL_QUADS);
	glTexCoord2f(texMins.x, texMins.y); glNormal3fv(&UpVector.x); glVertex2f(texMins.x, texMins.y);
	glTexCoord2f(texMaxs.x, texMins.y); glNormal3fv(&UpVector.x); glVertex2f(texMaxs.x, texMins.y);
	glTexCoord2f(texMaxs.x, texMaxs.y); glNormal3fv(&UpVector.x); glVertex2f(texMaxs.x, texMaxs.y);
	glTexCoord2f(texMins.x, texMaxs.y); glNormal3fv(&UpVector.x); glVertex2f(texMins.x, texMaxs.y);
	glEnd();
	glBindTexture(GetTextureTarget(), 0);
	glDisable(GetTextureTarget());

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
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
			glTexParameteri(texTarget, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

			if (texTarget == GL_TEXTURE_2D)
				glTexImage2D(texTarget, 0, GL_DEPTH_COMPONENT32F, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			else
				glTexImage2DMultisample(texTarget, globalRendering->msaaLevel, GL_DEPTH_COMPONENT32F, size.x, size.y, GL_TRUE);

			bufferAttachments[n] = GL_DEPTH_ATTACHMENT_EXT;
		} else {
			if (texTarget == GL_TEXTURE_2D)
				glTexImage2D(texTarget, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			else
				glTexImage2DMultisample(texTarget, globalRendering->msaaLevel, GL_RGBA8, size.x, size.y, GL_TRUE);

			bufferAttachments[n] = GL_COLOR_ATTACHMENT0_EXT + n;
		}
	}

	// sic; Mesa complains about an incomplete FBO if calling Bind before TexImage (?)
	buffer.Bind();
	buffer.AttachTextures(bufferTextureIDs, bufferAttachments, texTarget, ATTACHMENT_COUNT);

	glBindTexture(GetTextureTarget(), 0);
	// define the attachments we are going to draw into
	// note: the depth-texture attachment does not count
	// here and will be GL_NONE implicitly!
	glDrawBuffers(ATTACHMENT_COUNT - 1, &bufferAttachments[0]);

	// FBO must have been valid from point of construction
	// if we reached CreateGeometryBuffer, but CheckStatus
	// can still invalidate it
	assert(buffer.IsValid());

	valid = buffer.IsValid() && buffer.CheckStatus(name);

	buffer.Unbind();
	return valid;
}

bool GL::GeometryBuffer::Update(const bool init) {
	currBufferSize = GetWantedSize(true);

	// FBO must be valid from point of construction
	if (!buffer.IsValid())
		return false;

	// buffer isn't bound by calling context, can not call
	// GetStatus to check for GL_FRAMEBUFFER_COMPLETE_EXT
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

GL::GeometryBuffer* GL::GeometryBufferUni::geomBuffer = nullptr;