/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
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

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

/** @file  IFC.cpp
 *  @brief Implementation of the Industry Foundation Classes loader.
 */

#ifndef INCLUDED_IFCUTIL_H
#define INCLUDED_IFCUTIL_H

#include "IFCReaderGen.h"
#include "IFCLoader.h"

namespace Assimp {
namespace IFC {

	typedef float IfcFloat;

	// IfcFloat-precision math data types
	typedef aiVector2t<IfcFloat> IfcVector2;
	typedef aiVector3t<IfcFloat> IfcVector3;
	typedef aiMatrix4x4t<IfcFloat> IfcMatrix4;
	typedef aiMatrix3x3t<IfcFloat> IfcMatrix3;
	typedef aiColor4t<IfcFloat> IfcColor4; 


// helper for std::for_each to delete all heap-allocated items in a container
template<typename T>
struct delete_fun
{
	void operator()(T* del) {
		delete del;
	}
};

// ------------------------------------------------------------------------------------------------
// Temporary representation of an opening in a wall or a floor
// ------------------------------------------------------------------------------------------------
struct TempMesh;
struct TempOpening 
{
	const IFC::IfcExtrudedAreaSolid* solid;
	IfcVector3 extrusionDir;
	boost::shared_ptr<TempMesh> profileMesh;

	// ------------------------------------------------------------------------------
	TempOpening(const IFC::IfcExtrudedAreaSolid* solid,IfcVector3 extrusionDir,boost::shared_ptr<TempMesh> profileMesh)
		: solid(solid)
		, extrusionDir(extrusionDir)
		, profileMesh(profileMesh)
	{
	}

	// ------------------------------------------------------------------------------
	void Transform(const IfcMatrix4& mat); // defined later since TempMesh is not complete yet
};


// ------------------------------------------------------------------------------------------------
// Intermediate data storage during conversion. Keeps everything and a bit more.
// ------------------------------------------------------------------------------------------------
struct ConversionData 
{
	ConversionData(const STEP::DB& db, const IFC::IfcProject& proj, aiScene* out,const IFCImporter::Settings& settings)
		: len_scale(1.0)
		, angle_scale(1.0)
		, db(db)
		, proj(proj)
		, out(out)
		, settings(settings)
		, apply_openings()
		, collect_openings()
	{}

	~ConversionData() {
		std::for_each(meshes.begin(),meshes.end(),delete_fun<aiMesh>());
		std::for_each(materials.begin(),materials.end(),delete_fun<aiMaterial>());
	}

	IfcFloat len_scale, angle_scale;
	bool plane_angle_in_radians;

	const STEP::DB& db;
	const IFC::IfcProject& proj;
	aiScene* out;

	IfcMatrix4 wcs;
	std::vector<aiMesh*> meshes;
	std::vector<aiMaterial*> materials;

	typedef std::map<const IFC::IfcRepresentationItem*, std::vector<unsigned int> > MeshCache;
	MeshCache cached_meshes;

	const IFCImporter::Settings& settings;

	// Intermediate arrays used to resolve openings in walls: only one of them
	// can be given at a time. apply_openings if present if the current element
	// is a wall and needs its openings to be poured into its geometry while
	// collect_openings is present only if the current element is an 
	// IfcOpeningElement, for which all the geometry needs to be preserved
	// for later processing by a parent, which is a wall. 
	std::vector<TempOpening>* apply_openings;
	std::vector<TempOpening>* collect_openings;
};

// ------------------------------------------------------------------------------------------------
// Binary predicate to compare vectors with a given, quadratic epsilon.
// ------------------------------------------------------------------------------------------------
struct FuzzyVectorCompare {

	FuzzyVectorCompare(IfcFloat epsilon) : epsilon(epsilon) {}
	bool operator()(const IfcVector3& a, const IfcVector3& b) {
		return math::fabs((a-b).SquareLength()) < epsilon;
	}

	const IfcFloat epsilon;
};


// ------------------------------------------------------------------------------------------------
// Helper used during mesh construction. Aids at creating aiMesh'es out of relatively few polygons.
// ------------------------------------------------------------------------------------------------
struct TempMesh
{
	std::vector<IfcVector3> verts;
	std::vector<unsigned int> vertcnt;

	// utilities
	aiMesh* ToMesh();
	void Clear();
	void Transform(const IfcMatrix4& mat);
	IfcVector3 Center() const;
	void Append(const TempMesh& other);
	void RemoveAdjacentDuplicates();
};





// conversion routines for common IFC entities, implemented in IFCUtil.cpp
void ConvertColor(aiColor4D& out, const IfcColourRgb& in);
void ConvertColor(aiColor4D& out, const IfcColourOrFactor& in,ConversionData& conv,const aiColor4D* base);
void ConvertCartesianPoint(IfcVector3& out, const IfcCartesianPoint& in);
void ConvertDirection(IfcVector3& out, const IfcDirection& in);
void ConvertVector(IfcVector3& out, const IfcVector& in);
void AssignMatrixAxes(IfcMatrix4& out, const IfcVector3& x, const IfcVector3& y, const IfcVector3& z);
void ConvertAxisPlacement(IfcMatrix4& out, const IfcAxis2Placement3D& in);
void ConvertAxisPlacement(IfcMatrix4& out, const IfcAxis2Placement2D& in);
void ConvertAxisPlacement(IfcVector3& axis, IfcVector3& pos, const IFC::IfcAxis1Placement& in);
void ConvertAxisPlacement(IfcMatrix4& out, const IfcAxis2Placement& in, ConversionData& conv);
void ConvertTransformOperator(IfcMatrix4& out, const IfcCartesianTransformationOperator& op);
bool IsTrue(const EXPRESS::BOOLEAN& in);
IfcFloat ConvertSIPrefix(const std::string& prefix);


// IFCProfile.cpp
bool ProcessProfile(const IfcProfileDef& prof, TempMesh& meshout, ConversionData& conv);

// IFCMaterial.cpp
unsigned int ProcessMaterials(const IFC::IfcRepresentationItem& item, ConversionData& conv);

// IFCGeometry.cpp
bool ProcessRepresentationItem(const IfcRepresentationItem& item, std::vector<unsigned int>& mesh_indices, ConversionData& conv);
void AssignAddedMeshes(std::vector<unsigned int>& mesh_indices,aiNode* nd,ConversionData& /*conv*/);


// IFCCurve.cpp

// ------------------------------------------------------------------------------------------------
// Custom exception for use by members of the Curve class
// ------------------------------------------------------------------------------------------------
class CurveError 
{
public:
	CurveError(const std::string& s)
		: s(s)
	{
	}

	std::string s;
};


// ------------------------------------------------------------------------------------------------
// Temporary representation for an arbitrary sub-class of IfcCurve. Used to sample the curves
// to obtain a list of line segments.
// ------------------------------------------------------------------------------------------------
class Curve
{
protected:

	Curve(const IfcCurve& base_entity, ConversionData& conv)
		: base_entity(base_entity)
		, conv(conv)
	{}

public:

	typedef std::pair<IfcFloat, IfcFloat> ParamRange;

public:

	// check if a curve is closed 
	virtual bool IsClosed() const = 0;

	// evaluate the curve at the given parametric position
	virtual IfcVector3 Eval(IfcFloat p) const = 0;

	// try to match a point on the curve to a given parameter
	// for self-intersecting curves, the result is not ambiguous and
	// it is undefined which parameter is returned. 
	virtual bool ReverseEval(const IfcVector3& val, IfcFloat& paramOut) const;

	// get the range of the curve (both inclusive).
	// +inf and -inf are valid return values, the curve is not bounded in such a case.
	virtual std::pair<IfcFloat,IfcFloat> GetParametricRange() const = 0;
	IfcFloat GetParametricRangeDelta() const;

	// estimate the number of sample points that this curve will require
	virtual size_t EstimateSampleCount(IfcFloat start,IfcFloat end) const;

	// intelligently sample the curve based on the current settings
	// and append the result to the mesh
	virtual void SampleDiscrete(TempMesh& out,IfcFloat start,IfcFloat end) const;

#ifdef _DEBUG
	// check if a particular parameter value lies within the well-defined range
	bool InRange(IfcFloat) const;
#endif 

public:

	static Curve* Convert(const IFC::IfcCurve&,ConversionData& conv);

protected:

	const IfcCurve& base_entity;
	ConversionData& conv;
};


// --------------------------------------------------------------------------------
// A BoundedCurve always holds the invariant that GetParametricRange()
// never returns infinite values.
// --------------------------------------------------------------------------------
class BoundedCurve : public Curve 
{
public:

	BoundedCurve(const IfcBoundedCurve& entity, ConversionData& conv)
		: Curve(entity,conv)
	{}

public:

	bool IsClosed() const;

public:

	// sample the entire curve
	void SampleDiscrete(TempMesh& out) const;
	using Curve::SampleDiscrete;
};



}
}

#endif 
