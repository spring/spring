/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _TERRAIN_VERTEX_BUFFER_H_
#define _TERRAIN_VERTEX_BUFFER_H_

namespace terrain {

	class VertexBuffer
	{
	public:
		VertexBuffer ();
		~VertexBuffer ();

		void Init(int bytesize); /// Allocate the buffer
		void Free(); /// Free the buffer, called by destructor
		uint GetSize() { return size; } /// returns the vertex buffer size in bytes

		void* Bind (); /// returns the pointer that should be passed to glVertexPointer
		void Unbind (); /// unbind it, so it can be locked again

		void* LockData(); /// returns a pointer to the data, write-only
		void UnlockData(); /// unlocks the data, so it can be used for rendering
		
		static int TotalSize() { return totalBufferSize; } /// returns total buffer memory size used by all VertexBuffer instances

	protected:
		char *data;
		GLuint id;
		uint size;
		uint type;

		static int totalBufferSize;
	};

	class IndexBuffer : public VertexBuffer
	{
	public:
		IndexBuffer ();
	};
};

#endif // _TERRAIN_VERTEX_BUFFER_H_

