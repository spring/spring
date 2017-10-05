/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <cstring> // memcpy

#include "myGL.h"
#include "RenderDataBuffer.hpp"

void GL::RenderDataBuffer::EnableAttribs(size_t numAttrs, const Shader::ShaderInput* rawAttrs) const {
	for (size_t n = 0; n < numAttrs; n++) {
		const Shader::ShaderInput& a = rawAttrs[n];

		glEnableVertexAttribArray(a.index);
		glVertexAttribPointer(a.index, a.count, a.type, false, a.stride, a.data);
	}
}

void GL::RenderDataBuffer::DisableAttribs(size_t numAttrs, const Shader::ShaderInput* rawAttrs) const {
	for (size_t n = 0; n < numAttrs; n++) {
		glDisableVertexAttribArray(rawAttrs[n].index);
	}
}


void GL::RenderDataBuffer::CreateShader(
	size_t numObjects,
	size_t numUniforms,
	Shader::GLSLShaderObject* objects,
	const Shader::ShaderInput* uniforms
) {
	for (size_t n = 0; n < numObjects; n++) {
		shader.AttachShaderObject(&objects[n]);
	}

	shader.ReloadShaderObjects();
	shader.CreateAndLink();
	shader.RecalculateShaderHash();

	for (size_t n = 0; n < numUniforms; n++) {
		shader.SetUniform(uniforms[n]);
	}
}


void GL::RenderDataBuffer::Upload(
	size_t numElems,
	size_t numIndcs,
	size_t numAttrs,
	const uint8_t* rawElems,
	const uint8_t* rawIndcs,
	const Shader::ShaderInput* rawAttrs
) {
	array.Bind();
	elems.Bind();
	indcs.Bind();
	elems.New(numElems * sizeof(uint8_t), GL_STATIC_DRAW, rawElems);
	indcs.New(numIndcs * sizeof(uint8_t), GL_STATIC_DRAW, rawIndcs);

	EnableAttribs(numAttrs, rawAttrs);

	elems.UnmapBuffer();
	indcs.UnmapBuffer();
	array.Unbind();

	DisableAttribs(numAttrs, rawAttrs);
}


void GL::RenderDataBuffer::Submit(uint32_t primType, uint32_t dataSize, uint32_t dataType) {
	array.Bind();

	if (indcs.bufSize == 0) {
		// dataSize := numElems (unique verts)
		glDrawArrays(primType, 0, dataSize);
	} else {
		// dataSize := numIndcs, dataType := GL_UNSIGNED_INT
		assert(dataType == GL_UNSIGNED_INT);
		glDrawElements(primType, dataSize, dataType, nullptr);
	}

	array.Unbind();
}




#if 0
void GL::RenderDataBuffer::UploadC(
	size_t numElems,
	size_t numIndcs,
	const VA_TYPE_C* rawElems,
	const uint32_t* rawIndcs
) {
	uploadBuffer.clear();
	uploadBuffer.reserve(numElems * (VA_SIZE_C + 3));

	for (size_t n = 0; n < numElems; n++) {
		const VA_TYPE_C& e = rawElems[n];

		uploadBuffer.push_back(e.p.x);
		uploadBuffer.push_back(e.p.y);
		uploadBuffer.push_back(e.p.z);
		uploadBuffer.push_back(e.c.r); // turn SColor uint32 into 4 floats
		uploadBuffer.push_back(e.c.g);
		uploadBuffer.push_back(e.c.b);
		uploadBuffer.push_back(e.c.a);
	}

	Upload(uploadBuffer.size(), numIndcs, NUM_VA_TYPE_C_ATTRS, uploadBuffer.data(), rawIndcs, VA_TYPE_C_ATTRS);
}
#endif

