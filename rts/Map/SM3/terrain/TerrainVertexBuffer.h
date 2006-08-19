/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/
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
		char *data;
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
