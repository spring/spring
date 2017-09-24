/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GEOMETRYBUFFER_H
#define GEOMETRYBUFFER_H

#include "Rendering/GL/FBO.h"
#include "System/type2.h"

namespace GL {
	struct GeometryBuffer {
	public:
		enum {
			ATTACHMENT_NORMTEX = 0, // shading (not geometric) normals
			ATTACHMENT_DIFFTEX = 1, // diffuse texture fragments
			ATTACHMENT_SPECTEX = 2, // specular texture fragments
			ATTACHMENT_EMITTEX = 3, // emissive texture fragments
			ATTACHMENT_MISCTEX = 4, // custom data for LuaObjectRendering shaders
			ATTACHMENT_ZVALTEX = 5, // fragment depth-values (must be last)
			ATTACHMENT_COUNT   = 6,
		};

		GeometryBuffer(): name(nullptr), dead(false), bound(false) { Init(true); }
		~GeometryBuffer() { Kill(true); }

		void Init(bool ctor);
		void Kill(bool dtor);
		void Clear() const;

		void DetachTextures(const bool init);
		void DrawDebug(const unsigned int texID, const float2 texMins, const float2 texMaxs) const;
		void DrawDebug(const unsigned int texID) const { DrawDebug(texID, float2(0.0f, 0.0f), float2(1.0f, 1.0f)); }
		void SetName(const char* s) { name = s; }

		bool HasAttachments() const { return (bufferTextureIDs[0] != 0); }
		bool Valid() const { return (buffer.IsValid()); }
		bool Create(const int2 size);
		bool Update(const bool init);

		GLuint GetBufferTexture(unsigned int idx) const { return bufferTextureIDs[idx]; }
		GLuint GetBufferAttachment(unsigned int idx) const { return bufferAttachments[idx]; }

		const FBO& GetObject() const { return buffer; }
		      FBO& GetObject()       { return buffer; }

		void Bind() { assert(!dead && !bound); buffer.Bind(); bound = true; }
		void UnBind() { assert(!dead && bound); buffer.Unbind(); bound = false; }

		void SetDepthRange(float nearDepth, float farDepth) const;

		int2 GetCurrSize() const { return currBufferSize; }
		int2 GetPrevSize() const { return prevBufferSize; }

		int2 GetWantedSize(bool allowed) const;

	private:
		FBO buffer;

		GLuint bufferTextureIDs[ATTACHMENT_COUNT];
		GLenum bufferAttachments[ATTACHMENT_COUNT];

		int2 prevBufferSize;
		int2 currBufferSize;

		const char* name;

		bool dead;
		bool bound;
	};
}

#endif

