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
#include "aiMaterial.h"

using namespace System;

namespace AssimpNET
{
	public enum PropertyTypeInfo
	{
		PTI_Float,
		PTI_String,
		PTI_Integer,
		PTI_Buffer,
		_PTI_Force32Bit
	};

	public ref class MaterialProperty
	{
		public:
			MaterialProperty(void);
			MaterialProperty(aiMaterialProperty* native);
			~MaterialProperty(void);

		property array<char>^ mData
		{
			array<char>^ get()
			{
				array<char>^ tmp = gcnew array<char>(this->p_native->mDataLength);
				System::Runtime::InteropServices::Marshal::Copy((System::IntPtr)this->p_native->mData, (array<unsigned char>^)tmp, 0, this->p_native->mDataLength);
				return tmp;				
			}
			void set(array<char>^ value)
			{
				System::Runtime::InteropServices::Marshal::Copy((array<unsigned char>^)value, 0, (System::IntPtr)this->p_native->mData, value->Length);
			}
		}

		property unsigned int mDataLength
		{
			unsigned int get()
			{
				return this->p_native->mDataLength;
			}
			void set(unsigned int value)
			{
				this->p_native->mDataLength = value;
			}
		}

		property unsigned int mIndex
		{
			unsigned int get()
			{
				return this->p_native->mIndex;
			}
			void set(unsigned int value)
			{
				this->p_native->mIndex = value;
			}
		}

		property String^ mKey
		{
			String^ get()
			{
				return gcnew String(this->p_native->mKey.data);
			}
			void set(String^ value)
			{
				this->p_native->mKey.Set((char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(value).ToPointer());
			}
		}

		property unsigned int mSemantic
		{
			unsigned int get()
			{
				return this->p_native->mSemantic;
			}
			void set(unsigned int value)
			{
				this->p_native->mSemantic = value;
			}
		}

		property PropertyTypeInfo mType
		{
			PropertyTypeInfo get(){throw gcnew System::NotImplementedException();}
			void set(PropertyTypeInfo value){throw gcnew System::NotImplementedException();}
		}

		aiMaterialProperty* getNative();
		private:
		aiMaterialProperty *p_native;
	};
}//namespace