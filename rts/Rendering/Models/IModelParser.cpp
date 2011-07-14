/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <algorithm>
#include <cctype>
#include "System/mmgr.h"

#include "IModelParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "OBJParser.h"
#include "AssParser.h"
#include "assimp.hpp"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Units/Unit.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"

C3DModelLoader* modelParser = NULL;



static inline ModelType ModelExtToModelType(const C3DModelLoader::ParserMap& parsers, const std::string& ext) {
	if (ext == "3do") { return MODELTYPE_3DO; }
	if (ext == "s3o") { return MODELTYPE_S3O; }
	if (ext == "obj") { return MODELTYPE_OBJ; }

	if (parsers.find(ext) != parsers.end()) {
		return MODELTYPE_ASS;
	}

	// FIXME: this should be a content error?
	return MODELTYPE_OTHER;
}

static inline S3DModelPiece* ModelTypeToModelPiece(const ModelType& type) {
	if (type == MODELTYPE_3DO) { return (new S3DOPiece()); }
	if (type == MODELTYPE_S3O) { return (new SS3OPiece()); }
	if (type == MODELTYPE_OBJ) { return (new SOBJPiece()); }
	// FIXME: SAssPiece is not yet fully implemented
	// if (type == MODELTYPE_ASS) { return (new SAssPiece()); }
	return NULL;
}

static void RegisterAssimpModelParsers(C3DModelLoader::ParserMap& parsers, CAssParser* assParser) {
	std::string extension;
	std::string extensions;
	Assimp::Importer importer;

	// get a ";" separated list of format extensions ("*.3ds;*.lwo;*.mesh.xml;...")
	importer.GetExtensionList(extensions);

	// do not ignore the last extension
	extensions += ";";

	logOutput.Print("[%s] supported Assimp model formats: %s", __FUNCTION__, extensions.c_str());

	size_t i = 0;
	size_t j = 0;

	// split the list, strip off the "*." extension prefixes
	while ((j = extensions.find(";", i)) != std::string::npos) {
		extension = extensions.substr(i, j - i);
		extension = extension.substr(extension.find("*.") + 2);

		StringToLowerInPlace(extension);

		if (parsers.find(extension) == parsers.end()) {
			parsers[extension] = assParser;
		}

		i = j + 1;
	}
}



C3DModelLoader::C3DModelLoader()
{
	// file-extension should be lowercase
	parsers["3do"] = new C3DOParser();
	parsers["s3o"] = new CS3OParser();
	parsers["obj"] = new COBJParser();

	// FIXME: unify the metadata formats of CAssParser and COBJParser
	RegisterAssimpModelParsers(parsers, new CAssParser());
}


C3DModelLoader::~C3DModelLoader()
{
	// delete model cache
	ModelMap::iterator ci;
	for (ci = cache.begin(); ci != cache.end(); ++ci) {
		S3DModel* model = ci->second;

		DeleteChilds(model->GetRootPiece());
		delete model;
	}
	cache.clear();

	// get rid of Spring's native parsers
	delete parsers["3do"]; parsers.erase("3do");
	delete parsers["s3o"]; parsers.erase("s3o");
	delete parsers["obj"]; parsers.erase("obj");

	if (!parsers.empty()) {
		// delete the shared Assimp parser
		ParserMap::iterator pi = parsers.begin();
		delete pi->second;
	}

	parsers.clear();

#if defined(USE_GML) && GML_ENABLE_SIM && !GML_SHARE_LISTS
	createLists.clear();
	fixLocalModels.clear();
	Update(); // delete remaining local models
#endif
}



S3DModel* C3DModelLoader::Load3DModel(std::string name, const float3& centerOffset)
{
	GML_RECMUTEX_LOCK(model); // Load3DModel

	StringToLowerInPlace(name);

	//! search in cache first
	ModelMap::iterator ci;
	if ((ci = cache.find(name)) != cache.end()) {
		return ci->second;
	}

	//! not found in cache, create the model and cache it
	const std::string& fileExt = filesystem.GetExtension(name);
	const ParserMap::iterator pi = parsers.find(fileExt);

	if (pi != parsers.end()) {
		IModelParser* p = pi->second;
		S3DModel* model = NULL;
		S3DModelPiece* root = NULL;

		try {
			model = p->Load(name);
			model->relMidPos += centerOffset;
		} catch (const content_error& e) {
			// crash-dummy
			model = new S3DModel();
			model->type = ModelExtToModelType(parsers, StringToLower(fileExt));
			model->numPieces = 1;
			model->SetRootPiece(ModelTypeToModelPiece(model->type));
			model->GetRootPiece()->SetCollisionVolume(new CollisionVolume("box", UpVector * -1.0f, ZeroVector, CollisionVolume::COLVOL_HITTEST_CONT));

			logOutput.Print("WARNING: could not load model \"" + name + "\" (reason: " + e.what() + ")");
		}

		if ((root = model->GetRootPiece()) != NULL) {
			CreateLists(root);
		}

		cache[name] = model;    //! cache the model
		model->id = cache.size(); //! IDs start with 1
		return model;
	}

	logOutput.Print("ERROR: could not find a parser for model \"" + name + "\" (unknown format?)");
	return NULL;
}

void C3DModelLoader::Update() {
#if defined(USE_GML) && GML_ENABLE_SIM && !GML_SHARE_LISTS
	GML_RECMUTEX_LOCK(model); // Update

	for (std::vector<S3DModelPiece*>::iterator it = createLists.begin(); it != createLists.end(); ++it) {
		CreateListsNow(*it);
	}
	createLists.clear();

	for (std::set<CUnit*>::iterator i = fixLocalModels.begin(); i != fixLocalModels.end(); ++i) {
		SetLocalModelPieceDisplayLists(*i);
	}
	fixLocalModels.clear();

	for (std::vector<LocalModel*>::iterator i = deleteLocalModels.begin(); i != deleteLocalModels.end(); ++i) {
		delete *i;
	}
	deleteLocalModels.clear();
#endif
}


void C3DModelLoader::DeleteChilds(S3DModelPiece* o)
{
	for (std::vector<S3DModelPiece*>::iterator di = o->childs.begin(); di != o->childs.end(); ++di) {
		DeleteChilds(*di);
	}

	o->childs.clear();
	delete o;
}


void C3DModelLoader::DeleteLocalModel(CUnit* unit)
{
#if defined(USE_GML) && GML_ENABLE_SIM && !GML_SHARE_LISTS
	GML_RECMUTEX_LOCK(model); // DeleteLocalModel

	fixLocalModels.erase(unit);
	deleteLocalModels.push_back(unit->localmodel);
#else
	delete unit->localmodel;
#endif
}

void C3DModelLoader::CreateLocalModel(CUnit* unit)
{
#if defined(USE_GML) && GML_ENABLE_SIM && !GML_SHARE_LISTS
	GML_RECMUTEX_LOCK(model); // CreateLocalModel

	unit->localmodel = new LocalModel(unit->model);
	fixLocalModels.insert(unit);
#else
	unit->localmodel = new LocalModel(unit->model);
	// SetLocalModelPieceDisplayLists(unit);
#endif
}


void C3DModelLoader::SetLocalModelPieceDisplayLists(CUnit* unit)
{
	int piecenum = 0;
	SetLocalModelPieceDisplayLists(unit->model->GetRootPiece(), unit->localmodel, &piecenum);
}

void C3DModelLoader::SetLocalModelPieceDisplayLists(S3DModelPiece* model, LocalModel* lmodel, int* piecenum)
{
	lmodel->pieces[*piecenum]->dispListID = model->dispListID;

	for (unsigned int i = 0; i < model->childs.size(); i++) {
		(*piecenum)++;
		SetLocalModelPieceDisplayLists(model->childs[i], lmodel, piecenum);
	}
}


void C3DModelLoader::CreateListsNow(S3DModelPiece* o)
{
	o->dispListID = glGenLists(1);
	glNewList(o->dispListID, GL_COMPILE);
		o->DrawForList();
	glEndList();

	for (std::vector<S3DModelPiece*>::iterator bs = o->childs.begin(); bs != o->childs.end(); ++bs) {
		CreateListsNow(*bs);
	}
}


void C3DModelLoader::CreateLists(S3DModelPiece* o) {
#if defined(USE_GML) && GML_ENABLE_SIM && !GML_SHARE_LISTS
	createLists.push_back(o);
#else
	CreateListsNow(o);
#endif
}

/******************************************************************************/
/******************************************************************************/
