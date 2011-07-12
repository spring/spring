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

/** @file  IFCLoad.cpp
 *  @brief Implementation of the Industry Foundation Classes loader.
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_IFC_IMPORTER

#include <iterator>
#include <boost/tuple/tuple.hpp>


#include "IFCLoader.h"
#include "STEPFileReader.h"

#include "IFCUtil.h"

#include "StreamReader.h"
#include "MemoryIOWrapper.h"


using namespace Assimp;
using namespace Assimp::Formatter;
using namespace Assimp::IFC;

template<> const std::string LogFunctions<IFCImporter>::log_prefix = "IFC: ";


/* DO NOT REMOVE this comment block. The genentitylist.sh script
 * just looks for names adhering to the IfcSomething naming scheme
 * and includes all matches in the whitelist for code-generation. Thus,
 * all entity classes that are only indirectly referenced need to be
 * mentioned explicitly.

  IfcRepresentationMap
  IfcProductRepresentation
  IfcUnitAssignment
  IfcClosedShell
  IfcDoor

 */

namespace {


// forward declarations
void SetUnits(ConversionData& conv);
void SetCoordinateSpace(ConversionData& conv);
void ProcessSpatialStructures(ConversionData& conv);
aiNode* ProcessSpatialStructure(aiNode* parent, const IfcProduct& el ,ConversionData& conv);
void ProcessProductRepresentation(const IfcProduct& el, aiNode* nd, ConversionData& conv);
void MakeTreeRelative(ConversionData& conv);
void ConvertUnit(const EXPRESS::DataType& dt,ConversionData& conv);

} // anon

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
IFCImporter::IFCImporter()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
IFCImporter::~IFCImporter()
{
}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool IFCImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	const std::string& extension = GetExtension(pFile);
	if (extension == "ifc") {
		return true;
	}

	else if ((!extension.length() || checkSig) && pIOHandler)	{
		// note: this is the common identification for STEP-encoded files, so
		// it is only unambiguous as long as we don't support any further
		// file formats with STEP as their encoding.
		const char* tokens[] = {"ISO-10303-21"};
		return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// List all extensions handled by this loader
void IFCImporter::GetExtensionList(std::set<std::string>& app) 
{
	app.insert("ifc");
}


// ------------------------------------------------------------------------------------------------
// Setup configuration properties for the loader
void IFCImporter::SetupProperties(const Importer* pImp)
{
	settings.skipSpaceRepresentations = pImp->GetPropertyBool(AI_CONFIG_IMPORT_IFC_SKIP_SPACE_REPRESENTATIONS,true);
	settings.skipCurveRepresentations = pImp->GetPropertyBool(AI_CONFIG_IMPORT_IFC_SKIP_CURVE_REPRESENTATIONS,true);
	settings.useCustomTriangulation = pImp->GetPropertyBool(AI_CONFIG_IMPORT_IFC_CUSTOM_TRIANGULATION,true);

	settings.skipAnnotations = true;
}


// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void IFCImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	boost::shared_ptr<IOStream> stream(pIOHandler->Open(pFile));
	if (!stream) {
		ThrowException("Could not open file for reading");
	}

	boost::scoped_ptr<STEP::DB> db(STEP::ReadFileHeader(stream));
	const STEP::HeaderInfo& head = const_cast<const STEP::DB&>(*db).GetHeader();

	if(!head.fileSchema.size() || head.fileSchema.substr(0,3) != "IFC") {
		ThrowException("Unrecognized file schema: " + head.fileSchema);
	}

	if (!DefaultLogger::isNullLogger()) {
		DefaultLogger::get()->debug("File schema is \'" + head.fileSchema + '\'');
		if (head.timestamp.length()) {
			LogDebug("Timestamp \'" + head.timestamp + '\'');
		}
		if (head.app.length()) {
			DefaultLogger::get()->debug("Application/Exporter identline is \'" + head.app  + '\'');
		}
	}

	// obtain a copy of the machine-generated IFC scheme
	EXPRESS::ConversionSchema schema;
	GetSchema(schema);

	// tell the reader which entity types to track with special care
	static const char* const types_to_track[] = {
		"ifcsite", "ifcbuilding", "ifcproject"
	};

	// tell the reader for which types we need to simulate STEPs reverse indices
	static const char* const inverse_indices_to_track[] = {
		"ifcrelcontainedinspatialstructure", "ifcrelaggregates", "ifcrelvoidselement", "ifcstyleditem"
	};

	// feed the IFC schema into the reader and pre-parse all lines
	STEP::ReadFile(*db, schema, types_to_track, inverse_indices_to_track);

	const STEP::LazyObject* proj =  db->GetObject("ifcproject");
	if (!proj) {
		ThrowException("missing IfcProject entity");
	}

	ConversionData conv(*db,proj->To<IfcProject>(),pScene,settings);
	SetUnits(conv);
	SetCoordinateSpace(conv);
	ProcessSpatialStructures(conv);
	MakeTreeRelative(conv);

	// NOTE - this is a stress test for the importer, but it works only
	// in a build with no entities disabled. See 
	//     scripts/IFCImporter/CPPGenerator.py
	// for more information.
#ifdef ASSIMP_IFC_TEST
	db->EvaluateAll();
#endif

	// do final data copying
	if (conv.meshes.size()) {
		pScene->mNumMeshes = static_cast<unsigned int>(conv.meshes.size());
		pScene->mMeshes = new aiMesh*[pScene->mNumMeshes]();
		std::copy(conv.meshes.begin(),conv.meshes.end(),pScene->mMeshes);

		// needed to keep the d'tor from burning us
		conv.meshes.clear();
	}

	if (conv.materials.size()) {
		pScene->mNumMaterials = static_cast<unsigned int>(conv.materials.size());
		pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials]();
		std::copy(conv.materials.begin(),conv.materials.end(),pScene->mMaterials);

		// needed to keep the d'tor from burning us
		conv.materials.clear();
	}

	// apply world coordinate system (which includes the scaling to convert to meters and a -90 degrees rotation around x)
	aiMatrix4x4 scale, rot;
	aiMatrix4x4::Scaling(aiVector3D(conv.len_scale,conv.len_scale,conv.len_scale),scale);
	aiMatrix4x4::RotationX(-AI_MATH_HALF_PI_F,rot);

	pScene->mRootNode->mTransformation = rot * scale * conv.wcs * pScene->mRootNode->mTransformation;

	// this must be last because objects are evaluated lazily as we process them
	if ( !DefaultLogger::isNullLogger() ){
		DefaultLogger::get()->debug((Formatter::format(),"STEP: evaluated ",db->GetEvaluatedObjectCount()," object records"));
	}
}

namespace {


// ------------------------------------------------------------------------------------------------
void ConvertUnit(const IfcNamedUnit& unit,ConversionData& conv)
{
	if(const IfcSIUnit* const si = unit.ToPtr<IfcSIUnit>()) {

		if(si->UnitType == "LENGTHUNIT") { 
			conv.len_scale = si->Prefix ? ConvertSIPrefix(si->Prefix) : 1.f;
			DefaultLogger::get()->debug("got units used for lengths");
		}
		if(si->UnitType == "PLANEANGLEUNIT") { 
			if (si->Name != "RADIAN") {
				DefaultLogger::get()->warn("expected base unit for angles to be radian");
			}
		}
	}
	else if(const IfcConversionBasedUnit* const convu = unit.ToPtr<IfcConversionBasedUnit>()) {

		if(convu->UnitType == "PLANEANGLEUNIT") { 
			try {
				conv.angle_scale = convu->ConversionFactor->ValueComponent->To<EXPRESS::REAL>();
				ConvertUnit(*convu->ConversionFactor->UnitComponent,conv);
				DefaultLogger::get()->debug("got units used for angles");
			}
			catch(std::bad_cast&) {
				DefaultLogger::get()->error("skipping unknown IfcConversionBasedUnit.ValueComponent entry - expected REAL");
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
void ConvertUnit(const EXPRESS::DataType& dt,ConversionData& conv)
{
	try {
		const EXPRESS::ENTITY& e = dt.To<ENTITY>();

		const IfcNamedUnit& unit = e.ResolveSelect<IfcNamedUnit>(conv.db);
		if(unit.UnitType != "LENGTHUNIT" && unit.UnitType != "PLANEANGLEUNIT") {
			return;
		}

		ConvertUnit(unit,conv);
	}
	catch(std::bad_cast&) {
		// not entity, somehow
		DefaultLogger::get()->error("skipping unknown IfcUnit entry - expected entity");
	}
}

// ------------------------------------------------------------------------------------------------
void SetUnits(ConversionData& conv)
{
	// see if we can determine the coordinate space used to express. 
	for(size_t i = 0; i <  conv.proj.UnitsInContext->Units.size(); ++i ) {
		ConvertUnit(*conv.proj.UnitsInContext->Units[i],conv);
	}
}


// ------------------------------------------------------------------------------------------------
void SetCoordinateSpace(ConversionData& conv)
{
	const IfcRepresentationContext* fav = NULL;
	BOOST_FOREACH(const IfcRepresentationContext& v, conv.proj.RepresentationContexts) {
		fav = &v;
		// Model should be the most suitable type of context, hence ignore the others 
		if (v.ContextType && v.ContextType.Get() == "Model") { 
			break;
		}
	}
	if (fav) {
		if(const IfcGeometricRepresentationContext* const geo = fav->ToPtr<IfcGeometricRepresentationContext>()) {
			ConvertAxisPlacement(conv.wcs, *geo->WorldCoordinateSystem, conv);
			DefaultLogger::get()->debug("got world coordinate system");
		}
	}
}


// ------------------------------------------------------------------------------------------------
void ResolveObjectPlacement(aiMatrix4x4& m, const IfcObjectPlacement& place, ConversionData& conv)
{
	if (const IfcLocalPlacement* const local = place.ToPtr<IfcLocalPlacement>()){
		ConvertAxisPlacement(m, *local->RelativePlacement, conv);

		if (local->PlacementRelTo) {
			aiMatrix4x4 tmp;
			ResolveObjectPlacement(tmp,local->PlacementRelTo.Get(),conv);
			m = tmp * m;
		}
	}
	else {
		IFCImporter::LogWarn("skipping unknown IfcObjectPlacement entity, type is " + place.GetClassName());
	}
}

// ------------------------------------------------------------------------------------------------
void GetAbsTransform(aiMatrix4x4& out, const aiNode* nd, ConversionData& conv)
{
	aiMatrix4x4 t;
	if (nd->mParent) {
		GetAbsTransform(t,nd->mParent,conv);
	}
	out = t*nd->mTransformation;
}

// ------------------------------------------------------------------------------------------------
bool ProcessMappedItem(const IfcMappedItem& mapped, aiNode* nd_src, std::vector< aiNode* >& subnodes_src, ConversionData& conv)
{
	// insert a custom node here, the cartesian transform operator is simply a conventional transformation matrix
	std::auto_ptr<aiNode> nd(new aiNode());
	nd->mName.Set("IfcMappedItem");
		
	// handle the cartesian operator
	aiMatrix4x4 m;
	ConvertTransformOperator(m, *mapped.MappingTarget);

	aiMatrix4x4 msrc;
	ConvertAxisPlacement(msrc,*mapped.MappingSource->MappingOrigin,conv);

	msrc = m*msrc;

	std::vector<unsigned int> meshes;
	const size_t old_openings = conv.collect_openings ? conv.collect_openings->size() : 0;
	if (conv.apply_openings) {
		aiMatrix4x4 minv = msrc;
		minv.Inverse();
		BOOST_FOREACH(TempOpening& open,*conv.apply_openings){
			open.Transform(minv);
		}
	}

	const IfcRepresentation& repr = mapped.MappingSource->MappedRepresentation;

	bool got = false;
	BOOST_FOREACH(const IfcRepresentationItem& item, repr.Items) {
		if(!ProcessRepresentationItem(item,meshes,conv)) {
			IFCImporter::LogWarn("skipping unknown mapped entity, type is " + item.GetClassName());
		}
		else got = true;
	}

	if (!got) {
		return false;
	}

	AssignAddedMeshes(meshes,nd.get(),conv);
	if (conv.collect_openings) {

		// if this pass serves us only to collect opening geometry,
		// make sure we transform the TempMesh's which we need to
		// preserve as well.
		if(const size_t diff = conv.collect_openings->size() - old_openings) {
			for(size_t i = 0; i < diff; ++i) {
				(*conv.collect_openings)[old_openings+i].Transform(msrc);
			}
		}
	}

	nd->mTransformation =  nd_src->mTransformation * msrc;
	subnodes_src.push_back(nd.release());

	return true;
}

// ------------------------------------------------------------------------------------------------
struct RateRepresentationPredicate {

	int Rate(const IfcRepresentation* r) const {
		// the smaller, the better

		if (! r->RepresentationIdentifier) {
			// neutral choice if no extra information is specified
			return 0;
		}

		
		const std::string& name = r->RepresentationIdentifier.Get();
		if (name == "MappedRepresentation") {
			if (!r->Items.empty()) {
				// take the first item and base our choice on it
				const IfcMappedItem* const m = r->Items.front()->ToPtr<IfcMappedItem>();
				if (m) {
					return Rate(m->MappingSource->MappedRepresentation);
				}
			}
			return 100;
		}

		return Rate(name);
	}

	int Rate(const std::string& r) const {


		if (r == "SolidModel") {
			return -3;
		}
		
		// give strong preference to extruded geometry
		if (r == "SweptSolid") {
			return -10;
		}
		
		if (r == "Clipping") {
			return -5;
		}

		// 'Brep' is difficult to get right due to possible voids in the
		// polygon boundaries, so take it only if we are forced to (i.e.
		// if the only alternative is (non-clipping) boolean operations, 
		// which are not supported at all).
		if (r == "Brep") {
			return -2;
		}
		
		// Curves, bounding boxes - those will most likely not be loaded
		// as we can't make any use out of this data. So consider them
		// last.
		if (r == "BoundingBox" || r == "Curve2D") {
			return 100;
		}
		return 0;
	}

	bool operator() (const IfcRepresentation* a, const IfcRepresentation* b) const {
		return Rate(a) <= Rate(b);
	}
};

// ------------------------------------------------------------------------------------------------
void ProcessProductRepresentation(const IfcProduct& el, aiNode* nd, std::vector< aiNode* >& subnodes, ConversionData& conv)
{
	if(!el.Representation) {
		return;
	}


	std::vector<unsigned int> meshes;
	
	// we want only one representation type, so bring them in a suitable order (i.e try those
	// that look as if we could read them quickly at first). This way of reading
	// representation is relatively generic and allows the concrete implementations
	// for the different representation types to make some sensible choices what
	// to load and what not to load.
	const STEP::ListOf< STEP::Lazy< IfcRepresentation >, 1, 0 >& src = el.Representation.Get()->Representations;

	std::vector<const IfcRepresentation*> repr_ordered(src.size());
	std::copy(src.begin(),src.end(),repr_ordered.begin());
	std::sort(repr_ordered.begin(),repr_ordered.end(),RateRepresentationPredicate());

	BOOST_FOREACH(const IfcRepresentation* repr, repr_ordered) {
		bool res = false;
		BOOST_FOREACH(const IfcRepresentationItem& item, repr->Items) {
			if(const IfcMappedItem* const geo = item.ToPtr<IfcMappedItem>()) {
				res = ProcessMappedItem(*geo,nd,subnodes,conv) || res;
			}
			else {
				res = ProcessRepresentationItem(item,meshes,conv) || res;
			}
		}
		// if we got something meaningful at this point, skip any further representations
		if(res) {
			break;
		}
	}

	AssignAddedMeshes(meshes,nd,conv);
}

// ------------------------------------------------------------------------------------------------
aiNode* ProcessSpatialStructure(aiNode* parent, const IfcProduct& el, ConversionData& conv, std::vector<TempOpening>* collect_openings = NULL)
{
	const STEP::DB::RefMap& refs = conv.db.GetRefs();

	// skip over space and annotation nodes - usually, these have no meaning in Assimp's context
	if(conv.settings.skipSpaceRepresentations) {
		if(const IfcSpace* const space = el.ToPtr<IfcSpace>()) {
			DefaultLogger::get()->debug("skipping IfcSpace entity due to importer settings");
			return NULL;
		}
	}

	if(conv.settings.skipAnnotations) {
		if(const IfcAnnotation* const ann = el.ToPtr<IfcAnnotation>()) {
			DefaultLogger::get()->debug("skipping IfcAnnotation entity due to importer settings");
			return NULL;
		}
	}

	// add an output node for this spatial structure
	std::auto_ptr<aiNode> nd(new aiNode());
	nd->mName.Set(el.GetClassName()+"_"+(el.Name?el.Name:el.GlobalId));
	nd->mParent = parent;

	if(el.ObjectPlacement) {
		ResolveObjectPlacement(nd->mTransformation,el.ObjectPlacement.Get(),conv);
	}

	std::vector<TempOpening> openings;

	aiMatrix4x4 myInv;
	bool didinv = false;

	// convert everything contained directly within this structure,
	// this may result in more nodes.
	std::vector< aiNode* > subnodes;
	try {
		// locate aggregates and 'contained-in-here'-elements of this spatial structure and add them in recursively
		// on our way, collect openings in *this* element
		STEP::DB::RefMapRange range = refs.equal_range(el.GetID());

		for(STEP::DB::RefMapRange range2 = range; range2.first != range.second; ++range2.first) {
			const STEP::LazyObject& obj = conv.db.MustGetObject((*range2.first).second);

			// handle regularly-contained elements
			if(const IfcRelContainedInSpatialStructure* const cont = obj->ToPtr<IfcRelContainedInSpatialStructure>()) {
				BOOST_FOREACH(const IfcProduct& pro, cont->RelatedElements) {		
					if(const IfcOpeningElement* const open = pro.ToPtr<IfcOpeningElement>()) {
						// IfcOpeningElement is handled below. Sadly we can't use it here as is:
						// The docs say that opening elements are USUALLY attached to building storeys
						// but we want them for the building elements to which they belong to.
						continue;
					}
					
					aiNode* const ndnew = ProcessSpatialStructure(nd.get(),pro,conv,NULL);
					if(ndnew) {
						subnodes.push_back( ndnew );
					}
				}
			}
			// handle openings, which we collect in a list rather than adding them to the node graph
			else if(const IfcRelVoidsElement* const fills = obj->ToPtr<IfcRelVoidsElement>()) {
				if(fills->RelatingBuildingElement->GetID() == el.GetID()) {
					const IfcFeatureElementSubtraction& open = fills->RelatedOpeningElement;

					// move opening elements to a separate node since they are semantically different than elements that are just 'contained'
					std::auto_ptr<aiNode> nd_aggr(new aiNode());
					nd_aggr->mName.Set("$RelVoidsElement");
					nd_aggr->mParent = nd.get();

					nd_aggr->mTransformation = nd->mTransformation;

					std::vector<TempOpening> openings_local;
					aiNode* const ndnew = ProcessSpatialStructure( nd_aggr.get(),open, conv,&openings_local);
					if (ndnew) {

						nd_aggr->mNumChildren = 1;
						nd_aggr->mChildren = new aiNode*[1]();

						
						nd_aggr->mChildren[0] = ndnew;
						
						if(openings_local.size()) {
							if (!didinv) {
								myInv = aiMatrix4x4(nd->mTransformation ).Inverse();
								didinv = true;
							}

							// we need all openings to be in the local space of *this* node, so transform them
							BOOST_FOREACH(TempOpening& op,openings_local) {
								op.Transform( myInv*nd_aggr->mChildren[0]->mTransformation);
								openings.push_back(op);
							}
						}
						subnodes.push_back( nd_aggr.release() );
					}
				}
			}
		}

		for(;range.first != range.second; ++range.first) {
			if(const IfcRelAggregates* const aggr = conv.db.GetObject((*range.first).second)->ToPtr<IfcRelAggregates>()) {

				// move aggregate elements to a separate node since they are semantically different than elements that are just 'contained'
				std::auto_ptr<aiNode> nd_aggr(new aiNode());
				nd_aggr->mName.Set("$RelAggregates");
				nd_aggr->mParent = nd.get();

				nd_aggr->mTransformation = nd->mTransformation;

				nd_aggr->mChildren = new aiNode*[aggr->RelatedObjects.size()]();
				BOOST_FOREACH(const IfcObjectDefinition& def, aggr->RelatedObjects) {
					if(const IfcProduct* const prod = def.ToPtr<IfcProduct>()) {

						aiNode* const ndnew = ProcessSpatialStructure(nd_aggr.get(),*prod,conv,NULL);
						if(ndnew) {
							nd_aggr->mChildren[nd_aggr->mNumChildren++] = ndnew;
						}
					}
				}
			
				subnodes.push_back( nd_aggr.release() );
			}
		}

		conv.collect_openings = collect_openings;
		if(!conv.collect_openings) {
			conv.apply_openings = &openings;
		}

		ProcessProductRepresentation(el,nd.get(),subnodes,conv);
		conv.apply_openings = conv.collect_openings = NULL;

		if (subnodes.size()) {
			nd->mChildren = new aiNode*[subnodes.size()]();
			BOOST_FOREACH(aiNode* nd2, subnodes) {
				nd->mChildren[nd->mNumChildren++] = nd2;
				nd2->mParent = nd.get();
			}
		}
	}
	catch(...) {
		// it hurts, but I don't want to pull boost::ptr_vector into -noboost only for these few spots here
		std::for_each(subnodes.begin(),subnodes.end(),delete_fun<aiNode>());
		throw;
	}

	return nd.release();
}

// ------------------------------------------------------------------------------------------------
void ProcessSpatialStructures(ConversionData& conv)
{
	// XXX add support for multiple sites (i.e. IfcSpatialStructureElements with composition == COMPLEX)


	// process all products in the file. it is reasonable to assume that a
	// file that is relevant for us contains at least a site or a building.
	const STEP::DB::ObjectMapByType& map = conv.db.GetObjectsByType();

	ai_assert(map.find("ifcsite") != map.end());
	const STEP::DB::ObjectSet* range = &map.find("ifcsite")->second;

	if (range->empty()) {
		ai_assert(map.find("ifcbuilding") != map.end());
		range = &map.find("ifcbuilding")->second;
		if (range->empty()) {
			// no site, no building -  fail;
			IFCImporter::ThrowException("no root element found (expected IfcBuilding or preferably IfcSite)");
		}
	}

	
	BOOST_FOREACH(const STEP::LazyObject* lz, *range) {
		const IfcSpatialStructureElement* const prod = lz->ToPtr<IfcSpatialStructureElement>();
		if(!prod) {
			continue;
		}
		IFCImporter::LogDebug("looking at spatial structure `" + (prod->Name ? prod->Name.Get() : "unnamed") + "`" + (prod->ObjectType? " which is of type " + prod->ObjectType.Get():""));
	
		// the primary site is referenced by an IFCRELAGGREGATES element which assigns it to the IFCPRODUCT
		const STEP::DB::RefMap& refs = conv.db.GetRefs();
		STEP::DB::RefMapRange range = refs.equal_range(conv.proj.GetID());
		for(;range.first != range.second; ++range.first) {
			if(const IfcRelAggregates* const aggr = conv.db.GetObject((*range.first).second)->ToPtr<IfcRelAggregates>()) {
			
				BOOST_FOREACH(const IfcObjectDefinition& def, aggr->RelatedObjects) {
					// comparing pointer values is not sufficient, we would need to cast them to the same type first
					// as there is multiple inheritance in the game.
					if (def.GetID() == prod->GetID()) { 
						DefaultLogger::get()->debug("selecting this spatial structure as root structure");
						// got it, this is the primary site.
						conv.out->mRootNode = ProcessSpatialStructure(NULL,*prod,conv,NULL);
						return;
					}
				}

			}
		}
	}

	
	DefaultLogger::get()->warn("failed to determine primary site element, taking the first IfcSite");
	BOOST_FOREACH(const STEP::LazyObject* lz, *range) {
		const IfcSpatialStructureElement* const prod = lz->ToPtr<IfcSpatialStructureElement>();
		if(!prod) {
			continue;
		}

		conv.out->mRootNode = ProcessSpatialStructure(NULL,*prod,conv,NULL);
		return;
	}

	IFCImporter::ThrowException("failed to determine primary site element");
}

// ------------------------------------------------------------------------------------------------
void MakeTreeRelative(aiNode* start, const aiMatrix4x4& combined)
{
	// combined is the parent's absolute transformation matrix
	aiMatrix4x4 old = start->mTransformation;

	if (!combined.IsIdentity()) {
		start->mTransformation = aiMatrix4x4(combined).Inverse() * start->mTransformation;
	}

	// All nodes store absolute transformations right now, so we need to make them relative
	for (unsigned int i = 0; i < start->mNumChildren; ++i) {
		MakeTreeRelative(start->mChildren[i],old);
	}
}

// ------------------------------------------------------------------------------------------------
void MakeTreeRelative(ConversionData& conv)
{
	MakeTreeRelative(conv.out->mRootNode,aiMatrix4x4());
}

} // !anon



#endif
