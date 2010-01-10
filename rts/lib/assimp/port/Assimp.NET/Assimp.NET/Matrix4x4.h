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

#include "Matrix3x3.h"
#include "Vector3D.h"
#include "Quaternion.h"

//native includes
#include "aiVector3D.h"
#include "aiMatrix4x4.h"

namespace AssimpNET
{
	public ref class Matrix4x4
	{
	public:
		Matrix4x4(void);
		Matrix4x4(Matrix3x3^ other);
		Matrix4x4(	float _a1, float _a2, float _a3, float _a4,
					float _b1, float _b2, float _b3, float _b4,
					float _c1, float _c2, float _c3, float _c4,
					float _d1, float _d2, float _d3, float _d4);
		Matrix4x4(aiMatrix4x4* native);
		~Matrix4x4(void);

		void Decompose(Vector3D^ scaling, Quaternion^ rotation, Vector3D^ position);
		void DecomposeNoScaling(Quaternion^ rotation, Vector3D^ position);
		float Determinant();
		void FromEulerAnglesXYZ(const Vector3D^ euler);
		void FromEulerAnglesXYZ(float x, float y, float z);
		Matrix4x4^ Inverse();
		Matrix4x4^ Transpose();
		bool IsIdentity();
		bool operator != (const Matrix4x4^ m);
		bool operator == (const Matrix4x4^ m);
		Matrix4x4^ operator* (const Matrix4x4^ m);
		Matrix4x4^ operator*= (const Matrix4x4^ m);
		float operator[](unsigned int i);

		static Matrix4x4^ FromToMatrix (const Vector3D^ from, const Vector3D^ to, Matrix4x4^ out);
		static Matrix4x4^ Rotation (float a, const Vector3D^ axis, Matrix4x4^ out);	 	
		static Matrix4x4^ RotationX (float a, Matrix4x4^ out);
		static Matrix4x4^ RotationY (float a, Matrix4x4^ out);		 	
		static Matrix4x4^ RotationZ (float a, Matrix4x4^ out);		 	
		static Matrix4x4^ Scaling (const Vector3D^ v, Matrix4x4^ out);
		static Matrix4x4^ Translation (const Vector3D^ v, Matrix4x4^ out);



		property float a1
		{
			float get()
			{
				return this->p_native->a1;
			}
			void set(float value)
			{
				this->p_native->a1 = value;
			}
		}

		property float a2
		{
			float get()
			{
				return this->p_native->a2;
			}
			void set(float value)
			{
				this->p_native->a2 = value;
			}
		}

		property float a3
		{
			float get()
			{
				return this->p_native->a3;
			}
			void set(float value)
			{
				this->p_native->a3 = value;
			}
		}

		property float a4
		{
			float get()
			{
				return this->p_native->a4;
			}
			void set(float value)
			{
				this->p_native->a4 = value;
			}
		}

		property float b1
		{
			float get()
			{
				return this->p_native->b1;
			}
			void set(float value)
			{
				this->p_native->b1 = value;
			}
		}

		property float b2
		{
			float get()
			{
				return this->p_native->b2;
			}
			void set(float value)
			{
				this->p_native->b2 = value;
			}
		}

		property float b3
		{
			float get()
			{
				return this->p_native->b3;
			}
			void set(float value)
			{
				this->p_native->b3 = value;
			}
		}

		property float b4
		{
			float get()
			{
				return this->p_native->b4;
			}
			void set(float value)
			{
				this->p_native->b4 = value;
			}
		}

		property float c1
		{
			float get()
			{
				return this->p_native->c1;
			}
			void set(float value)
			{
				this->p_native->c1 = value;
			}
		}

		property float c2
		{
			float get()
			{
				return this->p_native->c2;
			}
			void set(float value)
			{
				this->p_native->c2 = value;
			}
		}

		property float c3
		{
			float get()
			{
				return this->p_native->c3;
			}
			void set(float value)
			{
				this->p_native->c3 = value;
			}
		}

		property float c4
		{
			float get()
			{
				return this->p_native->c4;
			}
			void set(float value)
			{
				this->p_native->c4 = value;
			}
		}

		property float d1
		{
			float get()
			{
				return this->p_native->d1;
			}
			void set(float value)
			{
				this->p_native->d1 = value;
			}
		}

		property float d2
		{
			float get()
			{
				return this->p_native->d2;
			}
			void set(float value)
			{
				this->p_native->d2 = value;
			}
		}

		property float d3
		{
			float get()
			{
				return this->p_native->d3;
			}
			void set(float value)
			{
				this->p_native->d3 = value;
			}
		}

		property float d4
		{
			float get()
			{
				return this->p_native->d4;
			}
			void set(float value)
			{
				this->p_native->d4 = value;
			}
		}

		aiMatrix4x4* getNative();
		private:
		aiMatrix4x4 *p_native;

	};
}//namespace