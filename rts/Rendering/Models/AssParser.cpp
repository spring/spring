/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AssParser.h"

#include "3DModel.h"
#include "3DModelLog.h"
#include "S3OParser.h"
#include "AssIO.h"

#include "Lua/LuaParser.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Platform/errorhandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

#include "lib/assimp/include/assimp/config.h"
#include "lib/assimp/include/assimp/defs.h"
#include "lib/assimp/include/assimp/types.h"
#include "lib/assimp/include/assimp/scene.h"
#include "lib/assimp/include/assimp/postprocess.h"
#include "lib/assimp/include/assimp/Importer.hpp"
#include "lib/assimp/include/assimp/DefaultLogger.hpp"
#include "Rendering/Textures/S3OTextureHandler.h"
#ifndef BITMAP_NO_OPENGL
	#include "Rendering/GL/myGL.h"
#endif


#define IS_QNAN(f) (f != f)
static const float DEGTORAD = PI / 180.f;
static const float RADTODEG = 180.f / PI;

//! triangulate guarantees the most complex mesh is a triangle
//! sortbytype ensure only 1 type of primitive type per mesh is used
static const int ASS_POSTPROCESS_OPTIONS =
	  aiProcess_RemoveComponent
	| aiProcess_FindInvalidData
	| aiProcess_CalcTangentSpace
	| aiProcess_GenSmoothNormals
	| aiProcess_Triangulate
	| aiProcess_GenUVCoords
	| aiProcess_SortByPType
	| aiProcess_JoinIdenticalVertices
	//| aiProcess_ImproveCacheLocality FIXME crashes in an assert in VertexTriangleAdjancency.h (date 04/2011)
	| aiProcess_SplitLargeMeshes;


//! Convert Assimp quaternion to radians around x, y and z
static float3 QuaternionToRadianAngles(aiQuaternion q1)
{
	float sqw = q1.w*q1.w;
	float sqx = q1.x*q1.x;
	float sqy = q1.y*q1.y;
	float sqz = q1.z*q1.z;
	float unit = sqx + sqy + sqz + sqw; //! if normalised is one, otherwise is correction factor
	float test = q1.x*q1.y + q1.z*q1.w;

	float3 result;

	if (test > 0.499f * unit) { //! singularity at north pole
		result.x = 2 * math::atan2(q1.x,q1.w);
		result.y = PI/2;
	} else if (test < -0.499f * unit) { //! singularity at south pole
		result.x = -2 * math::atan2(q1.x,q1.w);
		result.y = -PI/2;
	} else {
		result.x = math::atan2(2*q1.y*q1.w-2*q1.x*q1.z , sqx - sqy - sqz + sqw);
		result.y = math::asin(2*test/unit);
		result.z = math::atan2(2*q1.x*q1.w-2*q1.y*q1.z , -sqx + sqy - sqz + sqw);
	}
	return result;
}


class AssLogStream : public Assimp::LogStream
{
public:
	AssLogStream() {}
	~AssLogStream() {}
	void write(const char* message)
	{
		LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "Assimp: %s", message);
	}
};



S3DModel* CAssParser::Load(const std::string& modelFilePath)
{
	LOG_S(LOG_SECTION_MODEL, "Loading model: %s", modelFilePath.c_str() );
	const std::string modelPath  = FileSystem::GetDirectory(modelFilePath);
	const std::string modelName  = FileSystem::GetBasename(modelFilePath);

	//! LOAD METADATA
	//! Load the lua metafile. This contains properties unique to Spring models and must return a table
	std::string metaFileName = modelFilePath + ".lua";
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		//! Try again without the model file extension
		metaFileName = modelPath + '/' + modelName + ".lua";
	}
	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!CFileHandler::FileExists(metaFileName, SPRING_VFS_ZIP)) {
		LOG_S(LOG_SECTION_MODEL, "No meta-file '%s'. Using defaults.", metaFileName.c_str());
	} else if (!metaFileParser.Execute()) {
		LOG_SL(LOG_SECTION_MODEL, L_ERROR, "'%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());
	}

	//! Get the (root-level) model table
	const LuaTable& metaTable = metaFileParser.GetRoot();
	if (metaTable.IsValid()) {
		LOG_S(LOG_SECTION_MODEL, "Found valid model metadata in '%s'", metaFileName.c_str());
	}


	//! LOAD MODEL DATA
	//! Create a model importer instance
	Assimp::Importer importer;

	//! Create a logger for debugging model loading issues
	Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);
	const unsigned int severity = Assimp::Logger::Debugging|Assimp::Logger::Info|Assimp::Logger::Err|Assimp::Logger::Warn;
	Assimp::DefaultLogger::get()->attachStream( new AssLogStream(), severity );

	//! Give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler( new AssVFSSystem() );

	//! Speed-up processing by skipping things we don't need
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS|aiComponent_LIGHTS|aiComponent_TEXTURES|aiComponent_ANIMATIONS);

#ifndef BITMAP_NO_OPENGL
	//! Optimize VBO-Mesh sizes/ranges
	GLint maxIndices  = 1024;
	GLint maxVertices = 1024;
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES,  &maxIndices);
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &maxVertices); //FIXME returns not optimal data, at best compute it ourself! (pre-TL cache size!)
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT,   maxVertices);
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, maxIndices/3);
#endif

	//! Read the model file to build a scene object
	LOG_S(LOG_SECTION_MODEL, "Importing model file: %s", modelFilePath.c_str() );
	const aiScene* scene = importer.ReadFile( modelFilePath, ASS_POSTPROCESS_OPTIONS );
	if (scene != NULL) {
		LOG_S(LOG_SECTION_MODEL,
				"Processing scene for model: %s (%d meshes / %d materials / %d textures)",
				modelFilePath.c_str(), scene->mNumMeshes, scene->mNumMaterials,
				scene->mNumTextures );
	} else {
		LOG_SL(LOG_SECTION_MODEL, L_ERROR, "Model Import: %s",
				importer.GetErrorString());
	}

	SAssModel* model = new SAssModel;
	model->name = modelFilePath;
	model->type = MODELTYPE_ASS;
	model->scene = scene;
	//model->meta = &metaTable;

	//! Gather per mesh info
	CalculatePerMeshMinMax(model);

	//! Assign textures
	//! The S3O texture handler uses two textures.
	//! The first contains diffuse color (RGB) and teamcolor (A)
	//! The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).
	if (metaTable.KeyExists("tex1")) {
		model->tex1 = metaTable.GetString("tex1", "default.png");
	} else {
		//! Search for a texture
		std::vector<std::string> files = CFileHandler::FindFiles("unittextures/", modelName + ".*");
		for(std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
			model->tex1 = FileSystem::GetFilename(*fi);
			break; //! there can be only one!
		}
	}
	if (metaTable.KeyExists("tex2")) {
		model->tex2 = metaTable.GetString("tex2", "");
	} else {
		//! Search for a texture
		std::vector<std::string> files = CFileHandler::FindFiles("unittextures/", modelName + "2.*");
		for(std::vector<std::string>::iterator fi = files.begin(); fi != files.end(); ++fi) {
			model->tex2 = FileSystem::GetFilename(*fi);
			break; //! there can be only one!
		}
	}
	model->flipTexY = metaTable.GetBool("fliptextures", true); //! Flip texture upside down
	model->invertTexAlpha = metaTable.GetBool("invertteamcolor", true); //! Reverse teamcolor levels

	//! Load textures
	LOG_S(LOG_SECTION_MODEL, "Loading textures. Tex1: '%s' Tex2: '%s'",
			model->tex1.c_str(), model->tex2.c_str());
	texturehandlerS3O->LoadS3OTexture(model);

	//! Load all pieces in the model
	LOG_S(LOG_SECTION_MODEL, "Loading pieces from root node '%s'",
			scene->mRootNode->mName.data);
	LoadPiece(model, scene->mRootNode, metaTable);

	//! Update piece hierarchy based on metadata
	BuildPieceHierarchy( model );

	//! Simplified dimensions used for rough calculations
	model->radius = metaTable.GetFloat("radius", model->radius);
	model->height = metaTable.GetFloat("height", model->height);
	model->drawRadius = metaTable.GetFloat("drawRadius", model->drawRadius);
	model->relMidPos = metaTable.GetFloat3("midpos", model->relMidPos);
	model->mins = metaTable.GetFloat3("mins", model->mins);
	model->maxs = metaTable.GetFloat3("maxs", model->maxs);

	//! Calculate model dimensions if not set
	if (!metaTable.KeyExists("mins") || !metaTable.KeyExists("maxs")) CalculateMinMax( model->rootPiece );
	if (model->radius < 0.0001f) CalculateRadius( model );
	if (model->height < 0.0001f) CalculateHeight( model );

	//! Verbose logging of model properties
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->name: %s", model->name.c_str());
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->numobjects: %d", model->numPieces);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->radius: %f", model->radius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->height: %f", model->height);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->drawRadius: %f", model->drawRadius);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->mins: (%f,%f,%f)", model->mins[0], model->mins[1], model->mins[2]);
	LOG_SL(LOG_SECTION_MODEL, L_DEBUG, "model->maxs: (%f,%f,%f)", model->maxs[0], model->maxs[1], model->maxs[2]);

	LOG_S(LOG_SECTION_MODEL, "Model %s Imported.", model->name.c_str());
	return model;
}


void CAssParser::CalculatePerMeshMinMax(SAssModel* model)
{
	const aiScene* scene = model->scene;

	model->mesh_minmax.resize(scene->mNumMeshes);
	for (size_t i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh& mesh = *scene->mMeshes[i];

		SAssModel::MinMax& minmax = model->mesh_minmax[i];
		for (size_t vertexIndex= 0; vertexIndex < mesh.mNumVertices; vertexIndex++) {
			const aiVector3D& aiVertex = mesh.mVertices[vertexIndex];
			minmax.mins.x = std::min(minmax.mins.x, aiVertex.x);
			minmax.mins.y = std::min(minmax.mins.y, aiVertex.y);
			minmax.mins.z = std::min(minmax.mins.z, aiVertex.z);
			minmax.maxs.x = std::max(minmax.maxs.x, aiVertex.x);
			minmax.maxs.y = std::max(minmax.maxs.y, aiVertex.y);
			minmax.maxs.z = std::max(minmax.maxs.z, aiVertex.z);
		}
	}
}


void CAssParser::LoadPieceTransformations(SAssPiece* piece, const LuaTable& pieceMetaTable)
{
	//! Process transforms
	float3 rotate, scale, offset;
	aiVector3D _scale, _offset;
	aiQuaternion _rotate;
	piece->node->mTransformation.Decompose(_scale,_rotate,_offset);

	LOG_S(LOG_SECTION_PIECE,
			"(%d:%s) Assimp offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)",
			piece->model->numPieces, piece->name.c_str(),
			_offset.x, _offset.y, _offset.z,
			_rotate.x, _rotate.y, _rotate.z,
			_scale.x, _scale.y, _scale.z
			);

	offset   = pieceMetaTable.GetFloat3("offset", float3(_offset.x, _offset.y, _offset.z));
	offset.x = pieceMetaTable.GetFloat("offsetx", offset.x);
	offset.y = pieceMetaTable.GetFloat("offsety", offset.y);
	offset.z = pieceMetaTable.GetFloat("offsetz", offset.z);

	rotate   = QuaternionToRadianAngles(_rotate);
	rotate   = float3(rotate.z, rotate.x, rotate.y); //! swizzle
	rotate   = pieceMetaTable.GetFloat3("rotate", rotate * RADTODEG);
	rotate.x = pieceMetaTable.GetFloat("rotatex", rotate.x);
	rotate.y = pieceMetaTable.GetFloat("rotatey", rotate.y);
	rotate.z = pieceMetaTable.GetFloat("rotatez", rotate.z);
	rotate  *= DEGTORAD;

	scale   = pieceMetaTable.GetFloat3("scale", float3(_scale.x, _scale.z, _scale.y));
	scale.x = pieceMetaTable.GetFloat("scalex", scale.x);
	scale.y = pieceMetaTable.GetFloat("scaley", scale.y);
	scale.z = pieceMetaTable.GetFloat("scalez", scale.z);

	LOG_S(LOG_SECTION_PIECE,
			"(%d:%s) Relative offset (%f,%f,%f), rotate (%f,%f,%f), scale (%f,%f,%f)",
			piece->model->numPieces, piece->name.c_str(),
			offset.x, offset.y, offset.z,
			rotate.x, rotate.y, rotate.z,
			scale.x, scale.y, scale.z
			);
	piece->offset = offset;
	piece->rot = rotate;
	piece->scale = scale;
}


SAssPiece* CAssParser::LoadPiece(SAssModel* model, aiNode* node, const LuaTable& metaTable)
{
	//! Create new piece
	++model->numPieces;
	SAssPiece* piece = new SAssPiece;
	piece->type = MODELTYPE_OTHER;
	piece->node = node;
	piece->model = model;
	piece->isEmpty = (node->mNumMeshes == 0);

	if (node->mParent) {
		piece->name = std::string(node->mName.data);
	} else {
		//FIXME is this really smart?
		piece->name = "root"; //! The real model root
	}

	//! find a new name if none given or if a piece with the same name already exists
	if (piece->name.empty()) {
		piece->name = "piece";
	}
	ModelPieceMap::const_iterator it = model->pieces.find(piece->name);
	if (it != model->pieces.end()) {
		char buf[64];
		int i = 0;
		while (it != model->pieces.end()) {
			SNPRINTF(buf, 64, "%s%02i", piece->name.c_str(), i++);
			it = model->pieces.find(buf);
		}
		piece->name = buf;
	}
	LOG_S(LOG_SECTION_PIECE, "Converting node '%s' to piece '%s' (%d meshes).",
			node->mName.data, piece->name.c_str(), node->mNumMeshes);

	//! Load additional piece properties from metadata
	const LuaTable& pieceTable = metaTable.SubTable("pieces").SubTable(piece->name);
	if (pieceTable.IsValid()) {
		LOG_S(LOG_SECTION_PIECE, "Found metadata for piece '%s'",
				piece->name.c_str());
	}

	//! Load transforms
	LoadPieceTransformations(piece, pieceTable);

	//! Update piece min/max extents
	for (unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; meshListIndex++) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		SAssModel::MinMax& minmax = model->mesh_minmax[meshIndex];
		piece->mins.x = std::min(piece->mins.x, minmax.mins.x);
		piece->mins.y = std::min(piece->mins.y, minmax.mins.y);
		piece->mins.z = std::min(piece->mins.z, minmax.mins.z);
		piece->maxs.x = std::max(piece->maxs.x, minmax.maxs.x);
		piece->maxs.y = std::max(piece->maxs.y, minmax.maxs.y);
		piece->maxs.z = std::max(piece->maxs.z, minmax.maxs.z);
	}


	//! Check if piece is special (ie, used to set Spring model properties)
	if (strcmp(node->mName.data, "SpringHeight") == 0) {
		//! Set the model height to this nodes Z value
		if (!metaTable.KeyExists("height")) {
			model->height = piece->offset.z;
			LOG_S(LOG_SECTION_MODEL,
					"Model height of %f set by special node 'SpringHeight'",
					model->height);
		}
		--model->numPieces;
		delete piece;
		return NULL;
	}
	if (strcmp(node->mName.data, "SpringRadius") == 0) {
		if (!metaTable.KeyExists("midpos")) {
			model->relMidPos = float3(piece->offset.x, piece->offset.z, piece->offset.y); //! Y and Z are swapped because this piece isn't rotated
			LOG_S(LOG_SECTION_MODEL,
					"Model midpos of (%f,%f,%f) set by special node 'SpringRadius'",
					model->relMidPos.x, model->relMidPos.y, model->relMidPos.z);
		}
		if (!metaTable.KeyExists("radius")) {
			if (piece->maxs.x <= 0.00001f) {
				model->radius = piece->scale.x; //! the blender import script only sets the scale property
			} else {
				model->radius = piece->maxs.x; //! use the transformed mesh extents
			}
			LOG_S(LOG_SECTION_MODEL,
					"Model radius of %f set by special node 'SpringRadius'",
					model->radius);
		}
		--model->numPieces;
		delete piece;
		return NULL;
	}


	//! Get vertex data from node meshes
	for (unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; ++meshListIndex) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching mesh %d from scene",
				meshIndex);
		aiMesh* mesh = model->scene->mMeshes[meshIndex];
		std::vector<unsigned> mesh_vertex_mapping;
		//! extract vertex data
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Processing vertices for mesh %d (%d vertices)",
				meshIndex, mesh->mNumVertices);
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals() ? "Y" : "N"),
				(mesh->HasTangentsAndBitangents() ? "Y" : "N"),
				(mesh->HasTextureCoords(0) ? "Y" : "N"));

		// FIXME add piece->vertices.reserve()
		for (unsigned vertexIndex= 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			SAssVertex vertex;

			//! vertex coordinates
			//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching vertex %d from mesh", vertexIndex);
			const aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			vertex.pos.x = aiVertex.x;
			vertex.pos.y = aiVertex.y;
			vertex.pos.z = aiVertex.z;

			//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "vertex position %d: %f %f %f", vertexIndex, vertex.pos.x, vertex.pos.y, vertex.pos.z);

			//! vertex normal
			LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Fetching normal for vertex %d",
					vertexIndex);
			const aiVector3D& aiNormal = mesh->mNormals[vertexIndex];
			vertex.hasNormal = !IS_QNAN(aiNormal);
			if (vertex.hasNormal) {
				vertex.normal.x = aiNormal.x;
				vertex.normal.y = aiNormal.y;
				vertex.normal.z = aiNormal.z;
				//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "vertex normal %d: %f %f %f",vertexIndex, vertex.normal.x, vertex.normal.y,vertex.normal.z);
			}

			//! vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
						"Fetching tangent for vertex %d", vertexIndex );
				const aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				const aiVector3D& aiBitangent = mesh->mBitangents[vertexIndex];
				vertex.hasTangent = !IS_QNAN(aiBitangent) && !IS_QNAN(aiTangent);

				const float3 tangent(aiTangent.x, aiTangent.y, aiTangent.z);
				//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "vertex tangent %d: %f %f %f",vertexIndex, tangent.x, tangent.y,tangent.z);
				piece->sTangents.push_back(tangent);

				const float3 bitangent(aiBitangent.x, aiBitangent.y, aiBitangent.z);
				//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "vertex bitangent %d: %f %f %f",vertexIndex, bitangent.x, bitangent.y,bitangent.z);
				piece->tTangents.push_back(bitangent);
			}

			//! vertex texcoords
			if (mesh->HasTextureCoords(0)) {
				vertex.textureX = mesh->mTextureCoords[0][vertexIndex].x;
				vertex.textureY = mesh->mTextureCoords[0][vertexIndex].y;
				//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "vertex texcoords %d: %f %f", vertexIndex, vertex.textureX, vertex.textureY);
			}

			mesh_vertex_mapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		//! extract face data
		// FIXME add piece->vertexDrawOrder.reserve()
		LOG_SL(LOG_SECTION_PIECE, L_DEBUG,
				"Processing faces for mesh %d (%d faces)",
				meshIndex, mesh->mNumFaces);
		for (unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			const aiFace& face = mesh->mFaces[faceIndex];
			//! get the vertex belonging to the mesh
			for (unsigned vertexListID = 0; vertexListID < face.mNumIndices; ++vertexListID) {
				unsigned int vertexID = mesh_vertex_mapping[face.mIndices[vertexListID]];
				//LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "face %d vertex %d", faceIndex, vertexID);
				piece->vertexDrawOrder.push_back(vertexID);
			}
		}
	}

	//! collision volume for piece (not sure about these coords)
	// FIXME add metatable tags for this!!!!
	const float3 cvScales = piece->maxs - piece->mins;
	const float3 cvOffset = (piece->maxs - piece->offset) + (piece->mins - piece->offset);

	piece->colvol = new CollisionVolume("box", cvScales, cvOffset);

	//! Get parent name from metadata or model
	if (pieceTable.KeyExists("parent")) {
		piece->parentName = pieceTable.GetString("parent", "");
	} else if (node->mParent) {
		if (node->mParent->mParent) {
			piece->parentName = std::string(node->mParent->mName.data);
		} else { //! my parent is the root, which gets renamed
			piece->parentName = "root";
		}
	} else {
		piece->parentName = "";
	}

	LOG_S(LOG_SECTION_PIECE, "Loaded model piece: %s with %d meshes",
			piece->name.c_str(), node->mNumMeshes);

	//! Verbose logging of piece properties
	LOG_S(LOG_SECTION_PIECE, "piece->name: %s", piece->name.c_str());
	LOG_S(LOG_SECTION_PIECE, "piece->parent: %s", piece->parentName.c_str());

	//! Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		LoadPiece(model, node->mChildren[i], metaTable);
	}

	model->pieces[piece->name] = piece;
	return piece;
}


//! Because of metadata overrides we don't know the true hierarchy until all pieces have been loaded
void CAssParser::BuildPieceHierarchy(S3DModel* model)
{
	model->numPieces = 0;
	
	//! Loop through all pieces and create missing hierarchy info
	for (ModelPieceMap::const_iterator it = model->pieces.begin(); it != model->pieces.end(); ++it)
	{
		S3DModelPiece* piece = it->second;
		if (piece->name == "root") {
			piece->parent = NULL;
			model->SetRootPiece(piece); // FIXME what if called multiple times?
			++model->numPieces;
		} else if (piece->parentName != "") {
			piece->parent = model->FindPiece(piece->parentName);
			if (piece->parent == NULL) {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR,
						"Missing piece '%s' declared as parent of '%s'.",
						piece->parentName.c_str(), piece->name.c_str());
			} else {
				piece->parent->children.push_back(piece);
				++model->numPieces;
			}
		} else {
			//! A piece with no parent that isn't the root (orphan)
			piece->parent = model->FindPiece("root");
			if (piece->parent == NULL) {
				LOG_SL(LOG_SECTION_PIECE, L_ERROR, "Missing root piece");
			} else {
				piece->parent->children.push_back(piece);
				++model->numPieces;
			}
		}
	}
}


//! Iterate over the model and calculate its overall dimensions
void CAssParser::CalculateMinMax(S3DModelPiece* piece)
{
	piece->goffset = piece->parent ? piece->parent->goffset + piece->offset : piece->offset;

	//! update model min/max extents
	piece->model->mins.x = std::min(piece->goffset.x + piece->mins.x, piece->model->mins.x);
	piece->model->mins.y = std::min(piece->goffset.y + piece->mins.y, piece->model->mins.y);
	piece->model->mins.z = std::min(piece->goffset.z + piece->mins.z, piece->model->mins.z);
	piece->model->maxs.x = std::max(piece->goffset.x + piece->maxs.x, piece->model->maxs.x);
	piece->model->maxs.y = std::max(piece->goffset.y + piece->maxs.y, piece->model->maxs.y);
	piece->model->maxs.z = std::max(piece->goffset.z + piece->maxs.z, piece->model->maxs.z);

	//! Repeat with children
	for (unsigned int i = 0; i < piece->children.size(); i++) {
		CalculateMinMax(piece->children[i]);
	}
}


//! Calculate model radius from the min/max extents
void CAssParser::CalculateRadius(S3DModel* model)
{
	model->radius = std::max(std::fabs(model->maxs), std::fabs(model->mins)).Length();
	model->drawRadius = model->radius;
}


//! Calculate model height from the min/max extents
void CAssParser::CalculateHeight(S3DModel* model)
{
	model->height = model->maxs.z;
}


void SAssPiece::DrawForList() const
{
	if (isEmpty) {
		return;
	}
	LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Compiling piece %s", name.c_str());
	//! Add GL commands to the pieces displaylist

	const SAssVertex* sAssV = &vertices[0];

	//! pass the tangents as 3D texture coordinates
	//! (array elements are float3's, which are 12
	//! bytes in size and each represent a single
	//! xyz triple)
	//! TODO: test if we have this many texunits
	//! (if not, could only send the s-tangents)?

	if (!sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &sTangents[0].x);
	}
	if (!tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), &sAssV->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SAssVertex), &sAssV->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SAssVertex), &sAssV->normal.x);

	/*
	 * since aiProcess_SortByPType is being used,
	 * we're sure we'll get only 1 type here,
	 * so combination check isn't needed, also
	 * anything more complex than triangles is
	 * being split thanks to aiProcess_Triangulate
	 */
	glDrawElements(GL_TRIANGLES, vertexDrawOrder.size(), GL_UNSIGNED_INT, &vertexDrawOrder[0]);

	if (!sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (!tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	LOG_SL(LOG_SECTION_PIECE, L_DEBUG, "Completed compiling piece %s",
			name.c_str());
}
