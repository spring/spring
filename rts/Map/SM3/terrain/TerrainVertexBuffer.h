/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TERRAIN_VERTEX_BUFFER_H_
#define _TERRAIN_VERTEX_BUFFER_H_

#include "TerrainBase.h"
#include "Rendering/GL/myGL.h"

namespace terrain {

	class VertexBuffer
	{
	public:
		VertexBuffer();
		~VertexBuffer();

		/// Allocate the buffer
		void Init(int byteSize);
		/// Free the buffer, called by destructor
		void Free();
		/// returns the vertex buffer size in bytes
		uint GetSize() const { return size; }

		/// returns the pointer that should be passed to glVertexPointer
		void* Bind();
		/// unbind it, so it can be locked again
		void Unbind();

		/// returns a pointer to the data, write-only
		void* LockData();
		/// unlocks the data, so it can be used for rendering
		void UnlockData();

		/// returns total buffer memory size used by all VertexBuffer instances
		static int TotalSize() { return totalBufferSize; }

	protected:
		char* data;
		GLuint id;
		uint size;
		uint type;

		static int totalBufferSize;
	};

	class IndexBuffer : public VertexBuffer
	{
	public:
		IndexBuffer();
	};
}

#endif // _TERRAIN_VERTEX_BUFFER_H_

