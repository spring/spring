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

//managed includes
#include "Matrix4x4.h"
#include "VertexWeight.h"


//native includes
#include "aiMesh.h"

using namespace System;

namespace AssimpNET
{
	public ref class Bone
	{
	public:
		Bone(void);
		Bone(Bone% other);
		Bone(aiBone* native);
		~Bone(void);

		property String^ mName
		{
			String^ get()
			{
				return gcnew String(this->p_native->mName.data);
			}
			void set(String^ value)
			{
				throw gcnew System::NotImplementedException();
				this->p_native->mName.Set((char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(value).ToPointer());
			}
		}
		property unsigned int mNumWeights
		{
			unsigned int get()
			{
				return this->p_native->mNumWeights;
			}
			void set(unsigned int value)
			{
				this->p_native->mNumWeights = value;
			}
		}
		property Matrix4x4^ mOffsetMatrix
		{
			Matrix4x4^ get()
			{
				return gcnew Matrix4x4(&this->p_native->mOffsetMatrix);
			}
			void set(Matrix4x4^ value)
			{
				this->p_native->mOffsetMatrix = aiMatrix4x4(*value->getNative());
			}
		}
		property array<VertexWeight^>^ mWeights
		{
			array<VertexWeight^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<VertexWeight^>^ value){throw gcnew System::NotImplementedException();}
		}

		aiBone* getNative();
		private:
		aiBone *p_native;
	};
}//namespace