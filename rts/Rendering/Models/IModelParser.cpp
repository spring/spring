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
#include "lib/assimp/include/assimp/Importer.hpp"

C3DModelLoader* modelParser = nullptr;


static void RegisterAssimpModelFormats(C3DModelLoader::FormatMap& formats) {
	std::set<std::string> whitelist;
	std::string extension;
	std::string extensions;
	std::string enabledExtensions;

	whitelist.insert("3ds"  ); // 3DSMax
	whitelist.insert("dae"  ); // Collada
	whitelist.insert("lwo"  ); // LightWave
	whitelist.insert("blend"); // Blender

	Assimp::Importer importer;
	// get a ";" separated list of format extensions ("*.3ds;*.lwo;*.mesh.xml;...")
	importer.GetExtensionList(extensions);

	// do not ignore the last extension
	extensions += ";";

	size_t curIdx = 0;
	size_t nxtIdx = 0;

	// split the list, strip off the "*." extension prefixes
	while ((nxtIdx = extensions.find(";", curIdx)) != std::string::npos) {
		extension = extensions.substr(curIdx, nxtIdx - curIdx);
		extension = extension.substr(extension.find("*.") + 2);
		extension = StringToLower(extension);

		curIdx = nxtIdx + 1;

		if (whitelist.find(extension) == whitelist.end())
			continue;
		if (formats.find(extension) != formats.end())
			continue;

		formats[extension] = MODELTYPE_ASS;
		enabledExtensions += "*." + extension + ";";
	}

	LOG("[%s] supported Assimp model formats: %s", __FUNCTION__, enabledExtensions.c_str());
}

static S3DModel* CreateDummyModel()
{
	// create a crash-dummy
	S3DModel* model = new S3DModel();
	model->type = MODELTYPE_3DO;
	model->numPieces = 1;
	// give it one empty piece
	model->SetRootPiece(new S3DOPiece());
	model->GetRootPiece()->SetCollisionVolume(CollisionVolume("box", -UpVector, ZeroVector));
	return model;
}

static void CheckPieceNormals(const S3DModel* model, const S3DModelPiece* modelPiece)
{
	if (modelPiece->GetVertexCount() >= 3) {
		// do not check pseudo-pieces
		unsigned int numNullNormals = 0;

		for (unsigned int n = 0; n < modelPiece->GetVertexCount(); n++) {
			numNullNormals += (modelPiece->GetNormal(n).SqLength() < 0.5f);
		}

		if (numNullNormals > 0) {
			const char* formatStr =
				"[%s] piece \"%s\" of model \"%s\" has %u (of %u) null-normals! "
				"It will either be rendered fully black or with black splotches!";

			const char* modelName = model->name.c_str();
			const char* pieceName = modelPiece->name.c_str();

			LOG_L(L_WARNING, formatStr, __FUNCTION__, pieceName, modelName, numNullNormals, modelPiece->GetVertexCount());
		}
	}

	for (const S3DModelPiece* childPiece: modelPiece->children) {
		CheckPieceNormals(model, childPiece);
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
	models.push_back(nullptr);
}


C3DModelLoader::~C3DModelLoader()
{
	// delete model cache
	for (unsigned int n = 1; n < models.size(); n++) {
		S3DModel* model = models[n];

		assert(model != nullptr);
		assert(model->GetRootPiece() != nullptr);

		model->DeletePieces(model->GetRootPiece());
		model->SetRootPiece(nullptr);

		delete model;
	}

	for (ParserMap::const_iterator it = parsers.begin(); it != parsers.end(); ++it) {
		delete (it->second);
	}

	models.clear();
	cache.clear();
	parsers.clear();

}



std::string C3DModelLoader::FindModelPath(std::string name) const
{
	// check for empty string because we can be called
	// from Lua*Defs and certain features have no models
	if (!name.empty()) {
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
	}

	return name;
}


S3DModel* C3DModelLoader::Load3DModel(std::string modelName)
{
	// cannot happen except through SpawnProjectile
	if (modelName.empty())
		return nullptr;

	StringToLowerInPlace(modelName);

	// search in cache first
	ModelMap::iterator ci;
	FormatMap::iterator fi;

	if ((ci = cache.find(modelName)) != cache.end())
		return models[ci->second];

	const std::string& modelPath = FindModelPath(modelName);
	const std::string& fileExt = StringToLower(FileSystem::GetExtension(modelPath));

	if ((ci = cache.find(modelPath)) != cache.end())
		return models[ci->second];

	S3DModel* model = nullptr;
	IModelParser* parser = nullptr;

	// not found in cache, create the model and cache it
	if ((fi = formats.find(fileExt)) == formats.end()) {
		LOG_L(L_ERROR, "could not find a parser for model \"%s\" (unknown format?)", modelName.c_str());
	} else {
		parser = parsers[fi->second];
	}

	if (parser != nullptr) {
		try {
			model = parser->Load(modelPath);
		} catch (const content_error& ex) {
			LOG_L(L_WARNING, "could not load model \"%s\" (reason: %s)", modelName.c_str(), ex.what());
		}
	}

	if (model == nullptr)
		model = CreateDummyModel();

	assert(model->GetRootPiece() != nullptr);

	CreateLists(model->GetRootPiece());
	AddModelToCache(model, modelName, modelPath);

	if (model->type != MODELTYPE_3DO) {
		// warn about models with bad normals (they break lighting)
		// skip for 3DO's, it causes a LARGE amount of warning spam
		CheckPieceNormals(model, model->GetRootPiece());
	}

	return model;
}

void C3DModelLoader::AddModelToCache(S3DModel* model, const std::string& modelName, const std::string& modelPath) {
	model->id = models.size(); // IDs start at 1
	models.push_back(model);

	assert(models[model->id] == model);

	cache[modelName] = model->id;
	cache[modelPath] = model->id;
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
	CreateListsNow(o);
}

/******************************************************************************/
/******************************************************************************/
