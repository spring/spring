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

/** @file Implementation of the VertexTriangleAdjacency helper class
 */

#include "AssimpPCH.h"

// internal headers
#include "VertexTriangleAdjacency.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
VertexTriangleAdjacency::VertexTriangleAdjacency(aiFace *pcFaces,
	unsigned int iNumFaces,
	unsigned int iNumVertices /*= 0*/,
	bool bComputeNumTriangles /*= false*/)
{
	// compute the number of referenced vertices if it wasn't specified by the caller
	const aiFace* const pcFaceEnd = pcFaces + iNumFaces;
	if (!iNumVertices)
	{
		for (aiFace* pcFace = pcFaces; pcFace != pcFaceEnd; ++pcFace)
		{
			ai_assert(3 == pcFace->mNumIndices);
			iNumVertices = std::max(iNumVertices,pcFace->mIndices[0]);
			iNumVertices = std::max(iNumVertices,pcFace->mIndices[1]);
			iNumVertices = std::max(iNumVertices,pcFace->mIndices[2]);
		}
	}
	unsigned int* pi;

	// allocate storage
	if (bComputeNumTriangles)
	{
		pi = this->mLiveTriangles = new unsigned int[iNumVertices+1];
		::memset(this->mLiveTriangles,0,sizeof(unsigned int)*(iNumVertices+1));
		this->mOffsetTable = new unsigned int[iNumVertices+2]+1;
	}
	else 
	{
		pi = this->mOffsetTable = new unsigned int[iNumVertices+2]+1;
		::memset(this->mOffsetTable,0,sizeof(unsigned int)*(iNumVertices+1));
		this->mLiveTriangles = NULL; // important, otherwise the d'tor would crash
	}

	// get a pointer to the end of the buffer
	unsigned int* piEnd = pi+iNumVertices;
	*piEnd++ = 0u;

	// first pass: compute the number of faces referencing each vertex
	for (aiFace* pcFace = pcFaces; pcFace != pcFaceEnd; ++pcFace)
	{
		pi[pcFace->mIndices[0]]++;	
		pi[pcFace->mIndices[1]]++;	
		pi[pcFace->mIndices[2]]++;	
	}

	// second pass: compute the final offset table
	unsigned int iSum = 0;
	unsigned int* piCurOut = this->mOffsetTable;
	for (unsigned int* piCur = pi; piCur != piEnd;++piCur,++piCurOut)
	{
		unsigned int iLastSum = iSum;
		iSum += *piCur; 
		*piCurOut = iLastSum;
	}
	pi = this->mOffsetTable;

	// third pass: compute the final table
	this->mAdjacencyTable = new unsigned int[iSum];
	iSum = 0;
	for (aiFace* pcFace = pcFaces; pcFace != pcFaceEnd; ++pcFace,++iSum)
	{
		unsigned int idx = pcFace->mIndices[0];
		this->mAdjacencyTable[pi[idx]++] = iSum;

		idx = pcFace->mIndices[1];
		this->mAdjacencyTable[pi[idx]++] = iSum;

		idx = pcFace->mIndices[2];
		this->mAdjacencyTable[pi[idx]++] = iSum;
	}
	// fourth pass: undo the offset computations made during the third pass
	// We could do this in a separate buffer, but this would be TIMES slower.
	--this->mOffsetTable;
	*this->mOffsetTable = 0u;
}
// ------------------------------------------------------------------------------------------------
VertexTriangleAdjacency::~VertexTriangleAdjacency()
{
	// delete all allocated storage
	delete[] this->mOffsetTable;
	delete[] this->mAdjacencyTable;
	delete[] this->mLiveTriangles;
}
