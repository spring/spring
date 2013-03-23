/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"
#include <algorithm>
#include <cctype>

#include "IModelParser.h"
#include "3DModel.h"
#include "3DModelLog.h"
#include "3DOParser.h"
#include "S3OParser.h"
#include "OBJParser.h"
#include "AssParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "lib/gml/gml_base.h"
#include "lib/assimp/include/assimp/Importer.hpp"

C3DModelLoader* modelParser = NULL;


static inline S3DModelPiece* ModelTypeToModelPiece(const ModelType& type) {
	if (type == MODELTYPE_3DO) { return (new S3DOPiece()); }
	if (type == MODELTYPE_S3O) { return (new SS3OPiece()); }
	if (type == MODELTYPE_OBJ) { return (new SOBJPiece()); }
	// FIXME: SAssPiece is not yet fully implemented
	// if (type == MODELTYPE_ASS) { return (new SAssPiece()); }
	return NULL;
}

static void RegisterAssimpModelFormats(C3DModelLoader::FormatMap& formats) {
	std::string extension;
	std::string extensions;
	Assimp::Importer importer;

	// get a ";" separated list of format extensions ("*.3ds;*.lwo;*.mesh.xml;...")
	importer.GetExtensionList(extensions);

	// do not ignore the last extension
	extensions += ";";

	LOG("[%s] supported Assimp model formats: %s",
			__FUNCTION__, extensions.c_str());

	size_t curIdx = 0;
	size_t nxtIdx = 0;

	// split the list, strip off the "*." extension prefixes
	while ((nxtIdx = extensions.find(";", curIdx)) != std::string::npos) {
		extension = extensions.substr(curIdx, nxtIdx - curIdx);
		extension = extension.substr(extension.find("*.") + 2);
		extension = StringToLower(extension);

		if (formats.find(extension) == formats.end()) {
			formats[extension] = MODELTYPE_ASS;
		}

		curIdx = nxtIdx + 1;
	}
}



C3DModelLoader::C3DModelLoader()
{
	// file-extension should be lowercase
	formats["3do"] = MODELTYPE_3DO;
	formats["s3o"] = MODELTYPE_S3O;
	formats["obj"] = MODELTYPE_OBJ;

	parsers[MODELTYPE_3DO] = new C3DOParser();
	parsers[MODELTYPE_S3O] = new CS3OParser();
	parsers[MODELTYPE_OBJ] = new COBJParser();
	parsers[MODELTYPE_ASS] = new CAssParser();

	// FIXME: unify the metadata formats of CAssParser and COBJParser
	RegisterAssimpModelFormats(formats);

	// dummy first model, model IDs start at 1
	models.reserve(32);
	models.push_back(NULL);
}


C3DModelLoader::~C3DModelLoader()
{
	// delete model cache
	for (unsigned int n = 1; n < models.size(); n++) {
		S3DModel* model = models[n];

		assert(model != NULL);
		assert(model->GetRootPiece() != NULL);

		model->DeletePieces(model->GetRootPiece());
		model->SetRootPiece(NULL);

		delete model;
	}

	for (ParserMap::const_iterator it = parsers.begin(); it != parsers.end(); ++it) {
		delete (it->second);
	}

	models.clear();
	cache.clear();
	parsers.clear();

	if (GML::SimEnabled() && !GML::ShareLists()) {
		createLists.clear();
		fixLocalModels.clear();
		Update(); // delete remaining local models
	}
}



void CheckModelNormals(const S3DModel* model) {
	const char* modelName = model->name.c_str();
	const char* formatStr =
		"[%s] piece \"%s\" of model \"%s\" has %u (of %u) null-normals!"
		"It will either be rendered fully black or with black splotches!";

	// Warn about models with null normals (they break lighting and appear black)
	for (ModelPieceMap::const_iterator it = model->pieces.begin(); it != model->pieces.end(); ++it) {
		const S3DModelPiece* modelPiece = it->second;
		const char* pieceName = it->first.c_str();

		if (modelPiece->GetVertexCount() == 0)
			continue;

		unsigned int numNullNormals = 0;

		for (unsigned int n = 0; n < modelPiece->GetVertexCount(); n++) {
			numNullNormals += (modelPiece->GetNormal(n).SqLength() < 0.5f);
		}

		if (numNullNormals > 0) {
			LOG_L(L_WARNING, formatStr, __FUNCTION__, pieceName, modelName, numNullNormals, modelPiece->GetVertexCount());
		}
	}
}


std::string C3DModelLoader::FindModelPath(std::string name) const
{
	const std::string& fileExt = FileSystem::GetExtension(name);

	if (fileExt.empty()) {
		for (FormatMap::const_iterator it = formats.begin(); it != formats.end(); ++it) {
			const std::string& formatExt = it->first;

			if (CFileHandler::FileExists(name + "." + formatExt, SPRING_VFS_ZIP)) {
				name += ("." + formatExt); break;
			}
		}
	}

	if (!CFileHandler::FileExists(name, SPRING_VFS_ZIP)) {
		if (name.find("objects3d/") == std::string::npos) {
			return FindModelPath("objects3d/" + name);
		}
	}

	return name;
}


S3DModel* C3DModelLoader::Load3DModel(std::string modelName)
{
	GML_RECMUTEX_LOCK(model); // Load3DModel

	StringToLowerInPlace(modelName);

	// search in cache first
	ModelMap::iterator ci;
	FormatMap::iterator fi;

	if ((ci = cache.find(modelName)) != cache.end()) {
		return models[ci->second];
	}

	const std::string modelPath = FindModelPath(modelName);

	if ((ci = cache.find(modelPath)) != cache.end()) {
		return models[ci->second];
	}


	// not found in cache, create the model and cache it
	const std::string& fileExt = StringToLower(FileSystem::GetExtension(modelPath));

	if ((fi = formats.find(fileExt)) != formats.end()) {
		IModelParser* p = parsers[fi->second];
		S3DModel* model = NULL;
		S3DModelPiece* root = NULL;

		try {
			model = p->Load(modelPath);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "could not load model \"%s\" (reason: %s)", modelName.c_str(), ex.what());
			goto dummy;
		}

		if ((root = model->GetRootPiece()) != NULL) {
			CreateLists(root);
		}

		AddModelToCache(model, modelName, modelPath);
		CheckModelNormals(model);
		return model;
	}

	LOG_L(L_ERROR, "could not find a parser for model \"%s\" (unknown format?)", modelName.c_str());

dummy:
	// crash-dummy
	S3DModel* model = new S3DModel();
	model->type = MODELTYPE_3DO;
	model->numPieces = 1;
	// give it one dummy piece
	model->SetRootPiece(ModelTypeToModelPiece(MODELTYPE_3DO));
	model->GetRootPiece()->SetCollisionVolume(new CollisionVolume("box", -UpVector, ZeroVector));

	if (model->GetRootPiece() != NULL) {
		CreateLists(model->GetRootPiece());
	}

	AddModelToCache(model, modelName, modelPath);
	return model;
}

void C3DModelLoader::AddModelToCache(S3DModel* model, const std::string& modelName, const std::string& modelPath) {
	model->id = models.size(); // IDs start at 1
	models.push_back(model);

	assert(models[model->id] == model);

	cache[modelName] = model->id;
	cache[modelPath] = model->id;
}

void C3DModelLoader::Update() {
	if (GML::SimEnabled() && !GML::ShareLists()) {
		GML_RECMUTEX_LOCK(model); // Update

		for (std::list<S3DModelPiece*>::iterator it = createLists.begin(); it != createLists.end(); ++it) {
			CreateListsNow(*it);
		}
		createLists.clear();

		for (std::list<LocalModel*>::iterator i = fixLocalModels.begin(); i != fixLocalModels.end(); ++i) {
			(*i)->ReloadDisplayLists();
		}
		fixLocalModels.clear();

		for (std::list<LocalModel*>::iterator i = deleteLocalModels.begin(); i != deleteLocalModels.end(); ++i) {
			delete *i;
		}
		deleteLocalModels.clear();
	}
}



void C3DModelLoader::CreateLocalModel(LocalModel* localModel)
{
	const S3DModel* model = localModel->original;
	const S3DModelPiece* root = model->GetRootPiece();

	const bool dlistLoaded = (root->GetDisplayListID() != 0);

	if (!dlistLoaded && GML::SimEnabled() && !GML::ShareLists()) {
		GML_RECMUTEX_LOCK(model); // CreateLocalModel

		fixLocalModels.push_back(localModel);
	}
}

void C3DModelLoader::DeleteLocalModel(LocalModel* localModel)
{
	if (GML::SimEnabled() && !GML::ShareLists()) {
		GML_RECMUTEX_LOCK(model); // DeleteLocalModel

		std::list<LocalModel*>::iterator it = find(fixLocalModels.begin(), fixLocalModels.end(), localModel);
		if (it != fixLocalModels.end())
			fixLocalModels.erase(it);
		deleteLocalModels.push_back(localModel);
	} else {
		delete localModel;
	}
}


void C3DModelLoader::CreateListsNow(S3DModelPiece* o)
{
	o->UploadGeometryVBOs();

	const unsigned int dlistID = o->CreateDrawForList();

	for (unsigned int n = 0; n < o->GetChildCount(); n++) {
		CreateListsNow(o->GetChild(n));
	}

	// bind when everything is ready, should be more safe in multithreaded scenarios
	// TODO: still for 100% safety it should use GL_SYNC
	o->SetDisplayListID(dlistID);
}


void C3DModelLoader::CreateLists(S3DModelPiece* o) {
	if (GML::SimEnabled() && !GML::ShareLists())
		// already mutex'ed via ::Load3DModel()
		createLists.push_back(o);
	else
		CreateListsNow(o);
}

/******************************************************************************/
/******************************************************************************/
