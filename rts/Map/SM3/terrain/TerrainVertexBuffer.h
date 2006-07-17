/*---------------------------------------------------------------------
 Terrain Renderer using texture splatting and geomipmapping

 Copyright (2006) Jelmer Cnossen
 This code is released under GPL license (See LICENSE.html for info)
---------------------------------------------------------------------*/

#ifndef JC_TERRAIN_VERTEX_BUFFER_H
#define JC_TERRAIN_VERTEX_BUFFER_H

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
		void *data;
		uint id;
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

#endif
