/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <algorithm>
#include <cctype>
#include "mmgr.h"

#include "IModelParser.h"
#include "3DModel.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "OBJParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/COB/CobInstance.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"


C3DModelLoader* modelParser = NULL;


//////////////////////////////////////////////////////////////////////
// C3DModelLoader
//

C3DModelLoader::C3DModelLoader(void)
{
	// file-extension should be lowercase
	AddParser("3do", new C3DOParser());
	AddParser("s3o", new CS3OParser());
	AddParser("obj", new COBJParser());
}


C3DModelLoader::~C3DModelLoader(void)
{
	// delete model cache
	std::map<std::string, S3DModel*>::iterator ci;
	for (ci = cache.begin(); ci != cache.end(); ++ci) {
		DeleteChilds(ci->second->rootobject);
		delete ci->second;
	}
	cache.clear();

	// delete parsers
	std::map<std::string, IModelParser*>::iterator pi;
	for (pi = parsers.begin(); pi != parsers.end(); ++pi) {
		delete pi->second;
	}
	parsers.clear();

#if defined(USE_GML) && GML_ENABLE_SIM
	createLists.clear();
	fixLocalModels.clear();
	Update(); // delete remaining local models
#endif
}


void C3DModelLoader::AddParser(const std::string& ext, IModelParser* parser)
{
	parsers[ext] = parser;
}


S3DModel* C3DModelLoader::Load3DModel(std::string name, const float3& centerOffset)
{
	GML_STDMUTEX_LOCK(model); // Load3DModel

	StringToLowerInPlace(name);

	//! search in cache first
	std::map<std::string, S3DModel*>::iterator ci;
	if ((ci = cache.find(name)) != cache.end()) {
		return ci->second;
	}

	//! not found in cache, create the model and cache it
	const std::string fileExt = filesystem.GetExtension(name);
	std::map<std::string, IModelParser*>::iterator pi = parsers.find(fileExt);
	if (pi != parsers.end()) {
		IModelParser* p = pi->second;
		S3DModel* model = p->Load(name);

		model->relMidPos += centerOffset;

		CreateLists(model->rootobject);

		cache[name] = model;    //! cache model
		model->id = cache.size(); //! IDs start with 1
		return model;
	}

	logOutput.Print("couldn't find a parser for model named \"" + name + "\"");
	return NULL;
}

void C3DModelLoader::Update() {
#if defined(USE_GML) && GML_ENABLE_SIM
	GML_STDMUTEX_LOCK(model); // Update

	for (std::vector<S3DModelPiece*>::iterator it = createLists.begin(); it != createLists.end(); ++it) {
		CreateListsNow(*it);
	}
	createLists.clear();

	for (std::set<CUnit*>::iterator i = fixLocalModels.begin(); i != fixLocalModels.end(); ++i) {
		FixLocalModel(*i);
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
	for (std::vector<S3DModelPiece*>::iterator di = o->childs.begin(); di != o->childs.end(); di++) {
		DeleteChilds(*di);
	}

	o->childs.clear();
	delete o;
}


void C3DModelLoader::DeleteLocalModel(CUnit* unit)
{
#if defined(USE_GML) && GML_ENABLE_SIM
	GML_STDMUTEX_LOCK(model); // DeleteLocalModel

	fixLocalModels.erase(unit);
	deleteLocalModels.push_back(unit->localmodel);
#else
	delete unit->localmodel;
#endif
}


void C3DModelLoader::CreateLocalModel(CUnit* unit)
{
#if defined(USE_GML) && GML_ENABLE_SIM
	GML_STDMUTEX_LOCK(model); // CreateLocalModel

	unit->localmodel = CreateLocalModel(unit->model);
	fixLocalModels.insert(unit);
#else
	unit->localmodel = CreateLocalModel(unit->model);
	// FixLocalModel(unit);
#endif
}


LocalModel* C3DModelLoader::CreateLocalModel(S3DModel* model)
{
	LocalModel* lmodel = new LocalModel;
	lmodel->type = model->type;
	lmodel->pieces.reserve(model->numobjects);

	for (unsigned int i = 0; i < model->numobjects; i++) {
		lmodel->pieces.push_back(new LocalModelPiece);
	}
	lmodel->pieces[0]->parent = NULL;

	int piecenum = 0;
	CreateLocalModelPieces(model->rootobject, lmodel, &piecenum);
	return lmodel;
}


void C3DModelLoader::CreateLocalModelPieces(S3DModelPiece* piece, LocalModel* lmodel, int* piecenum)
{
	LocalModelPiece& lmp = *lmodel->pieces[*piecenum];
	lmp.original  =  piece;
	lmp.name      =  piece->name;
	lmp.type      =  piece->type;
	lmp.displist  =  piece->displist;
	lmp.visible   = !piece->isEmpty;
	lmp.updated   =  false;
	lmp.pos       =  piece->offset;
	lmp.rot       =  float3(0.0f, 0.0f, 0.0f);
	lmp.colvol    = new CollisionVolume(piece->colvol);

	lmp.childs.reserve(piece->childs.size());
	for (unsigned int i = 0; i < piece->childs.size(); i++) {
		(*piecenum)++;
		lmp.childs.push_back(lmodel->pieces[*piecenum]);
		lmodel->pieces[*piecenum]->parent = &lmp;
		CreateLocalModelPieces(piece->childs[i], lmodel, piecenum);
	}
}


void C3DModelLoader::FixLocalModel(CUnit* unit)
{
	int piecenum = 0;
	FixLocalModel(unit->model->rootobject, unit->localmodel, &piecenum);
}


void C3DModelLoader::FixLocalModel(S3DModelPiece* model, LocalModel* lmodel, int* piecenum)
{
	lmodel->pieces[*piecenum]->displist = model->displist;

	for (unsigned int i = 0; i < model->childs.size(); i++) {
		(*piecenum)++;
		FixLocalModel(model->childs[i], lmodel, piecenum);
	}
}


void C3DModelLoader::CreateListsNow(S3DModelPiece* o)
{
	o->displist = glGenLists(1);
	glNewList(o->displist, GL_COMPILE);
	o->DrawList();
	glEndList();

	for (std::vector<S3DModelPiece*>::iterator bs = o->childs.begin(); bs != o->childs.end(); bs++) {
		CreateListsNow(*bs);
	}
}


void C3DModelLoader::CreateLists(S3DModelPiece* o) {
#if defined(USE_GML) && GML_ENABLE_SIM
	createLists.push_back(o);
#else
	CreateListsNow(o);
#endif
}

/******************************************************************************/
/******************************************************************************/
