/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

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

----------------------------------------------------------------------
*/

/** @file  IFCProfile.cpp
 *  @brief Read profile and curves entities from IFC files
 */

#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER
#include "IFCUtil.h"

namespace Assimp {
	namespace IFC {

// ------------------------------------------------------------------------------------------------
void ProcessPolyLine(const IfcPolyline& def, TempMesh& meshout, ConversionData& /*conv*/)
{
	// this won't produce a valid mesh, it just spits out a list of vertices
	aiVector3D t;
	BOOST_FOREACH(const IfcCartesianPoint& cp, def.Points) {
		ConvertCartesianPoint(t,cp);
		meshout.verts.push_back(t);
	}
	meshout.vertcnt.push_back(meshout.verts.size());
}

// ------------------------------------------------------------------------------------------------
bool ProcessCurve(const IfcCurve& curve,  TempMesh& meshout, ConversionData& conv)
{
	boost::scoped_ptr<const Curve> cv(Curve::Convert(curve,conv));
	if (!cv) {
		IFCImporter::LogWarn("skipping unknown IfcCurve entity, type is " + curve.GetClassName());
		return false;
	}

	// we must have a bounded curve at this point
	if (const BoundedCurve* bc = dynamic_cast<const BoundedCurve*>(cv.get())) {
		try {
			bc->SampleDiscrete(meshout);
		}
		catch(const  CurveError& cv) {
			IFCImporter::LogError(cv.s+ " (error occurred while processing curve)");
			return false;
		}
		meshout.vertcnt.push_back(meshout.verts.size());
		return true;
	}

	IFCImporter::LogError("cannot use unbounded curve as profile");
	return false;
}

// ------------------------------------------------------------------------------------------------
void ProcessClosedProfile(const IfcArbitraryClosedProfileDef& def, TempMesh& meshout, ConversionData& conv)
{
	ProcessCurve(def.OuterCurve,meshout,conv);
}

// ------------------------------------------------------------------------------------------------
void ProcessOpenProfile(const IfcArbitraryOpenProfileDef& def, TempMesh& meshout, ConversionData& conv)
{
	ProcessCurve(def.Curve,meshout,conv);
}

// ------------------------------------------------------------------------------------------------
void ProcessParametrizedProfile(const IfcParameterizedProfileDef& def, TempMesh& meshout, ConversionData& conv)
{
	if(const IfcRectangleProfileDef* const cprofile = def.ToPtr<IfcRectangleProfileDef>()) {
		const float x = cprofile->XDim*0.5f, y = cprofile->YDim*0.5f;

		meshout.verts.reserve(meshout.verts.size()+4);
		meshout.verts.push_back( aiVector3D( x, y, 0.f ));
		meshout.verts.push_back( aiVector3D(-x, y, 0.f ));
		meshout.verts.push_back( aiVector3D(-x,-y, 0.f ));
		meshout.verts.push_back( aiVector3D( x,-y, 0.f ));
		meshout.vertcnt.push_back(4);
	}
	else if( const IfcCircleProfileDef* const circle = def.ToPtr<IfcCircleProfileDef>()) {
		if( const IfcCircleHollowProfileDef* const hollow = def.ToPtr<IfcCircleHollowProfileDef>()) {
			// TODO
		}
		const size_t segments = 32;
		const float delta = AI_MATH_TWO_PI_F/segments, radius = circle->Radius;

		meshout.verts.reserve(segments);

		float angle = 0.f;
		for(size_t i = 0; i < segments; ++i, angle += delta) {
			meshout.verts.push_back( aiVector3D( cos(angle)*radius, sin(angle)*radius, 0.f ));
		}

		meshout.vertcnt.push_back(segments);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcParameterizedProfileDef entity, type is " + def.GetClassName());
		return;
	}

	aiMatrix4x4 trafo;
	ConvertAxisPlacement(trafo, *def.Position);
	meshout.Transform(trafo);
}

// ------------------------------------------------------------------------------------------------
bool ProcessProfile(const IfcProfileDef& prof, TempMesh& meshout, ConversionData& conv) 
{
	if(const IfcArbitraryClosedProfileDef* const cprofile = prof.ToPtr<IfcArbitraryClosedProfileDef>()) {
		ProcessClosedProfile(*cprofile,meshout,conv);
	}
	else if(const IfcArbitraryOpenProfileDef* const copen = prof.ToPtr<IfcArbitraryOpenProfileDef>()) {
		ProcessOpenProfile(*copen,meshout,conv);
	}
	else if(const IfcParameterizedProfileDef* const cparam = prof.ToPtr<IfcParameterizedProfileDef>()) {
		ProcessParametrizedProfile(*cparam,meshout,conv);
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcProfileDef entity, type is " + prof.GetClassName());
		return false;
	}
	meshout.RemoveAdjacentDuplicates();
	if (!meshout.vertcnt.size() || meshout.vertcnt.front() <= 1) {
		return false;
	}
	return true;
}

} // ! IFC
} // ! Assimp

#endif
