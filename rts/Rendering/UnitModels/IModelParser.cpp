#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <algorithm>
#include <cctype>
#include "mmgr.h"

#include "IModelParser.h"
#include "3DModel.h"
#include "3DOParser.h"
#include "s3oParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/FartextureHandler.h"
#include "Util.h"
#include "LogOutput.h"
#include "Exceptions.h"


C3DModelParser* modelParser = NULL;


//////////////////////////////////////////////////////////////////////
// C3DModelParser
//

C3DModelParser::C3DModelParser(void)
{
	C3DOParser* unit3doparser = new C3DOParser();
	CS3OParser* units3oparser = new CS3OParser();

	AddParser("3do", unit3doparser);
	AddParser("s3o", units3oparser);
}


C3DModelParser::~C3DModelParser(void)
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


void C3DModelParser::AddParser(const std::string ext, IModelParser* parser)
{
	parsers[ext] = parser;
}


S3DModel* C3DModelParser::Load3DModel(std::string name, const float3& centerOffset)
{
	GML_STDMUTEX_LOCK(model); // Load3DModel

	StringToLowerInPlace(name);

	// search in cache first
	std::map<std::string, S3DModel*>::iterator ci;
	if ((ci = cache.find(name)) != cache.end()) {
		return ci->second;
	}

	std::string fileExt = GetFileExt(name);

	std::map<std::string, IModelParser*>::iterator pi;
	if ((pi = parsers.find(fileExt)) != parsers.end()) {
		IModelParser* p = pi->second;
		S3DModel* model = p->Load(name, centerOffset);

		CreateLists(p, model->rootobject);
		fartextureHandler->CreateFarTexture(model);

		cache[name] = model; // cache model
		return model;
	}

	// throw content_error("couldn't find a model parser for " + name);
	logOutput.Print("couldn't find a model parser for " + name);
	return NULL;
}

void C3DModelParser::Update() {
#if defined(USE_GML) && GML_ENABLE_SIM
	GML_STDMUTEX_LOCK(model); // Update

	for (std::vector<ModelParserPair>::iterator i = createLists.begin(); i != createLists.end(); ++i) {
		CreateListsNow(i->parser, i->model);
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


void C3DModelParser::DeleteChilds(S3DModelPiece* o)
{
	for (std::vector<S3DModelPiece*>::iterator di = o->childs.begin(); di != o->childs.end(); di++) {
		delete (*di);
	}
	o->childs.clear();
	delete o;
}


void C3DModelParser::DeleteLocalModel(CUnit* unit)
{
#if defined(USE_GML) && GML_ENABLE_SIM
	GML_STDMUTEX_LOCK(model); // DeleteLocalModel

	fixLocalModels.erase(unit);
	deleteLocalModels.push_back(unit->localmodel);
#else
	delete unit->localmodel;
#endif
}


void C3DModelParser::CreateLocalModel(CUnit* unit)
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


LocalModel* C3DModelParser::CreateLocalModel(S3DModel* model)
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


void C3DModelParser::CreateLocalModelPieces(S3DModelPiece* piece, LocalModel* lmodel, int* piecenum)
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


void C3DModelParser::FixLocalModel(CUnit* unit)
{
	int piecenum = 0;
	FixLocalModel(unit->model->rootobject, unit->localmodel, &piecenum);
}


void C3DModelParser::FixLocalModel(S3DModelPiece* model, LocalModel* lmodel, int* piecenum)
{
	lmodel->pieces[*piecenum]->displist = model->displist;

	for (unsigned int i = 0; i < model->childs.size(); i++) {
		(*piecenum)++;
		FixLocalModel(model->childs[i], lmodel, piecenum);
	}
}


void C3DModelParser::CreateListsNow(IModelParser* parser, S3DModelPiece* o)
{
	o->displist = glGenLists(1);
	glNewList(o->displist, GL_COMPILE);
	parser->Draw(o);
	glEndList();

	for (std::vector<S3DModelPiece*>::iterator bs = o->childs.begin(); bs != o->childs.end(); bs++) {
		CreateListsNow(parser, *bs);
	}
}


void C3DModelParser::CreateLists(IModelParser* parser, S3DModelPiece* o) {
#if defined(USE_GML) && GML_ENABLE_SIM
	createLists.push_back(ModelParserPair(o, parser));
#else
	CreateListsNow(parser, o);
#endif
}

/******************************************************************************/
/******************************************************************************/
