/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

 * Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

 * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

 * Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
 */

#pragma once

//native includes
#include "aiMesh.h"

namespace AssimpNET
{
	public ref class Face
	{
	public:
		Face(void);
		Face(Face% other);
		Face(aiFace* native);
		~Face(void);

		bool operator != (const Face^ other);
		Face^ operator = (const Face^ other);
		bool operator == (const Face^ other);

		property array<unsigned int>^ mIndices
		{
			array<unsigned int>^ get()
			{
				array<unsigned int>^ tmp = gcnew array<unsigned int>(this->p_native->mNumIndices);
				System::Runtime::InteropServices::Marshal::Copy((System::IntPtr)this->p_native->mIndices, (array<int>^)tmp, 0, tmp->Length);
				return tmp;
			}
			void set(array<unsigned int>^ value)
			{
				System::Runtime::InteropServices::Marshal::Copy((array<int>^)value, 0, (System::IntPtr)this->p_native->mIndices, value->Length);
			}
		}

		property unsigned int mNumIndices
		{
			unsigned int get()
			{
				return this->p_native->mNumIndices;
			
			}
			void set(unsigned int value)
			{
				this->p_native->mNumIndices = value;
			}
		}

		aiFace* getNative();
		private:
		aiFace *p_native;
	};
}//namespace