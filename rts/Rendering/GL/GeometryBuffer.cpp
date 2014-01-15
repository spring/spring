/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GeometryBuffer.h"
#include "Rendering/GlobalRendering.h"
#include <cstring> //memset

void GL::GeometryBuffer::Init() {
	memset(&bufferTextureIDs[0], 0, sizeof(bufferTextureIDs));
	memset(&bufferAttachments[0], 0, sizeof(bufferAttachments));

	// NOTE:
	//   Lua can toggle drawDeferred and might be the
	//   first to call us --> initial buffer size must
	//   be (0, 0) so prevSize != currSize (when !init)
	prevBufferSize = GetWantedSize(false);
	currBufferSize = GetWantedSize(true);
}

void GL::GeometryBuffer::Kill() {
	if (buffer.IsValid()) {
		DetachTextures(false);
	}
}

void GL::GeometryBuffer::DetachTextures(const bool init) {
	if (init) {
		// nothing to detach yet during init
		return;
	}

	buffer.Bind();

	// ATI drivers crash when detaching non-attached textures (?)
	for (unsigned int i = 0; i < (ATTACHMENT_COUNT - 1); ++i) {
		buffer.Detach(GL_COLOR_ATTACHMENT0_EXT + i);
	}

	buffer.Detach(GL_DEPTH_ATTACHMENT_EXT);
	buffer.Unbind();

	glDeleteTextures(ATTACHMENT_COUNT, &bufferTextureIDs[0]);
}

void GL::GeometryBuffer::DrawDebug(unsigned int texID) {
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texID);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glNormal3f(0.0f, 1.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glNormal3f(0.0f, 1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glNormal3f(0.0f, 1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f); glNormal3f(0.0f, 1.0f, 0.0f); glVertex2f(0.0f, 1.0f);

	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

bool GL::GeometryBuffer::Create(const int2 size) {
	buffer.Bind();

	for (unsigned int n = 0; n < ATTACHMENT_COUNT; n++) {
		glGenTextures(1, &bufferTextureIDs[n]);
		glBindTexture(GL_TEXTURE_2D, bufferTextureIDs[n]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (n == ATTACHMENT_ZVALTEX) {
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			bufferAttachments[n] = GL_DEPTH_ATTACHMENT_EXT;
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			bufferAttachments[n] = GL_COLOR_ATTACHMENT0_EXT + n;
		}

		buffer.AttachTexture(bufferTextureIDs[n], GL_TEXTURE_2D, bufferAttachments[n]);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	// define the attachments we are going to draw into
	// note: the depth-texture attachment does not count
	// here and will be GL_NONE implicitly!
	glDrawBuffers(ATTACHMENT_COUNT - 1, &bufferAttachments[0]);

	// FBO must have been valid from point of construction
	// if we reached CreateGeometryBuffer, but CheckStatus
	// can still invalidate it
	assert(buffer.IsValid());

	const bool ret = buffer.CheckStatus(bufferName);

	buffer.Unbind();
	return ret;
}

bool GL::GeometryBuffer::Update(const bool init) {
	currBufferSize = GetWantedSize(true);

	// FBO must be valid from point of construction
	if (!buffer.IsValid())
		return false;

	if (buffer.GetStatus() == GL_FRAMEBUFFER_COMPLETE_EXT) {
		// FBO cannot be complete yet during init!
		//
		// however GL spec says that FBO's with only
		// empty attachments are complete by default
		// --> extend FBO class to test for this?
		// assert(!init);

		// FBO was already initialized (during init
		// or from Lua) so it will have attachments
		// --> check if they need to be regenerated
		// (eg. if a window resize event happened)
		if (prevBufferSize == currBufferSize)
			return true;

		DetachTextures(init);
	}

	return (Create(prevBufferSize = currBufferSize));
}

int2 GL::GeometryBuffer::GetWantedSize(bool allowed) const {
	if (allowed)
		return (int2(globalRendering->viewSizeX, globalRendering->viewSizeY));

	return (int2(0, 0));
}

