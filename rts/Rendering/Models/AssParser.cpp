// AssParser.cpp: implementation of the CAssParser class. Reads 3D formats supported by Assimp lib.
//
//////////////////////////////////////////////////////////////////////
//#include "StdAfx.h"

#include "Util.h"
#include "LogOutput.h"
#include "Platform/errorhandler.h"
#include "System/Exceptions.h"
#include "Sim/Misc/CollisionVolume.h"
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "3DModel.h"
#include "S3OParser.h"
#include "AssIO.h"
#include "AssParser.h"

#include "assimp.hpp"
#include "aiDefines.h"
#include "aiTypes.h"
#include "aiScene.h"
#include "aiPostProcess.h"
#include "DefaultLogger.h"
#include "Rendering/Textures/S3OTextureHandler.h"

#define IS_QNAN(f) (f != f)
//static const float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
//static const float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
#define ASS_POSTPROCESS_OPTIONS \
	aiProcess_FindInvalidData				| \
	aiProcess_CalcTangentSpace				| \
	aiProcess_GenSmoothNormals				| \
	aiProcess_SplitLargeMeshes				| \
	aiProcess_Triangulate					| \
	aiProcess_GenUVCoords             		| \
	aiProcess_SortByPType					| \
	aiProcess_JoinIdenticalVertices

//aiProcess_ImproveCacheLocality

// Enabling the "Model" log subsystem will provide general model output. "Model-detail" is extremely verbose.
static CLogSubsystem LOG_MODEL("Model");
static CLogSubsystem LOG_MODEL_DETAIL("Model-detail");
static CLogSubsystem LOG_PIECE_DETAIL("Piece-detail");

class AssLogStream : public Assimp::LogStream
{
public:
	AssLogStream() {}
	~AssLogStream() {}
	void write(const char* message)
	{
		logOutput.Print (LOG_MODEL_DETAIL, "Assimp: %s", message);
	}
};



S3DModel* CAssParser::Load(const std::string& modelFileName)
{
	logOutput.Print (LOG_MODEL, "AssParser: Loading model: %s\n", modelFileName.c_str() );

	// Load the lua metafile. This contains properties unique to Spring models and must return a table
	std::string metaFileName = modelFileName.substr(0, modelFileName.find_last_of('.')) + ".lua";
	CFileHandler metaFile(metaFileName);
	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);

	if (!metaFileParser.Execute()) {
		if (!metaFile.FileExists()) {
			logOutput.Print(LOG_MODEL, "AssParser: No meta-file '%s'. Using defaults.", metaFileName.c_str());
		} else {
			logOutput.Print(LOG_MODEL, "AssParser: ERROR in '%s': %s. Using defaults.", metaFileName.c_str(), metaFileParser.GetErrorLog().c_str());
		}
	} else {
		logOutput.Print(LOG_MODEL, "AssParser: Using model meta-file '%s'", metaFileName.c_str());
	}
	
	// Get the (root-level) model table
	const LuaTable modelTable = metaFileParser.GetRoot();

	// Create a model importer instance
 	Assimp::Importer importer;

	// Give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler( new AssVFSSystem() );

	// Create a logger for debugging model loading issues
	Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);
	const unsigned int severity = Assimp::Logger::DEBUGGING|Assimp::Logger::INFO|Assimp::Logger::ERR|Assimp::Logger::WARN;
	Assimp::DefaultLogger::get()->attachStream( new AssLogStream(), severity );

	// Read the model file to build a scene object
	logOutput.Print(LOG_MODEL, "AssParser: Importing model file: %s\n", modelFileName.c_str() );
	const aiScene* scene = importer.ReadFile( modelFileName, ASS_POSTPROCESS_OPTIONS );

	// Convert to Spring model format
	S3DModel* model = new S3DModel;
	if (scene != NULL) {
		logOutput.Print(LOG_MODEL, "AssParser: Processing scene for model: %s (%d meshes / %d materials / %d textures)", modelFileName.c_str(), scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures );
		model->name = modelFileName;
		model->type = MODELTYPE_ASS;
		model->scene = scene;
		model->numobjects = 0;

		// Simplified dimensions used for rough calculations
		model->radius = modelTable.GetFloat("radius", 1.0f);
		model->height = modelTable.GetFloat("height", 1.0f);
		model->relMidPos = modelTable.GetFloat3("midpos", float3(0.0f,0.0f,0.0f));
		model->mins = modelTable.GetFloat3("mins", float3(0.0f,0.0f,0.0f));
		model->maxs = modelTable.GetFloat3("maxs", float3(1.0f,1.0f,1.0f));

		// Assign textures
		// The S3O texture handler uses two textures.
		// The first contains diffuse color (RGB) and teamcolor (A)
		// The second contains glow (R), reflectivity (G) and 1-bit Alpha (A).
		model->tex1 = modelTable.GetString("tex1", "default.png");
		model->tex2 = modelTable.GetString("tex2", "");
		model->invertAlpha = modelTable.GetBool("invertteamcolor", true);
		
		// Load textures
		logOutput.Print(LOG_MODEL, "AssParser: Loading textures. Tex1: '%s' Tex2: '%s'", model->tex1.c_str(), model->tex2.c_str());
		texturehandlerS3O->LoadS3OTexture(model);
		
		// Identify and load the root object in the model.
		//const aiString hitboxName( std::string("hitbox") );

		logOutput.Print(LOG_MODEL, "AssParser: Loading pieces from root node '%s'", scene->mRootNode->mName.data);
		SAssPiece* rootPiece = LoadPiece( scene->mRootNode, model );
		model->rootobject = rootPiece;

		// Load size defaults from 'hitbox' object in model (if it exists).
		// If it doesn't exist loop of all pieces
		// These values can be overridden in unitDef
		//const aiString hitboxName( std::string("hitbox") );
		//aiNode* hitbox = scene->mRootNode->FindNode( hitboxName );
		//if (!hitbox) {
		//	hitbox = scene->mRootNode;
		//}
		
		// Verbose logging of model properties
		logOutput.Print(LOG_MODEL_DETAIL, "AssParser: model->name: %s", model->name.c_str());
		logOutput.Print(LOG_MODEL_DETAIL, "AssParser: model->numobjects: %d", model->numobjects);
		logOutput.Print(LOG_MODEL_DETAIL, "AssParser: model->radius: %f", model->radius);
		logOutput.Print(LOG_MODEL_DETAIL, "AssParser: model->height: %f", model->height);
		logOutput.Print(LOG_MODEL_DETAIL, "AssParser: model->mins: (%f,%f,%f)", model->mins[0], model->mins[1], model->mins[2]);
		logOutput.Print(LOG_MODEL_DETAIL, "AssParser: model->maxs: (%f,%f,%f)", model->maxs[0], model->maxs[1], model->maxs[2]);
				
		logOutput.Print (LOG_MODEL, "AssParser: Model %s Imported.", model->name.c_str());
	} else {
		logOutput.Print (LOG_MODEL, "AssParser: Model Import Error: %s\n",  importer.GetErrorString());
	}
	return model;
}

SAssPiece* CAssParser::LoadPiece(aiNode* node, S3DModel* model)
{
	logOutput.Print(LOG_MODEL_DETAIL, "AssParser: Converting node '%s' to a piece (%d meshes).", node->mName.data, node->mNumMeshes);

	model->numobjects++;

	SAssPiece* piece = new SAssPiece;
	piece->name = std::string(node->mName.data);
	piece->type = MODELTYPE_ASS;
	piece->node = node;
	piece->isEmpty = node->mNumMeshes == 0;

	aiVector3D scale, position;
 	aiQuaternion rotation;
	node->mTransformation.Decompose(scale,rotation,position); // convert local to global transform

	piece->mins = float3(0.0f,0.0f,0.0f);
	piece->maxs = float3(1.0f,1.0f,1.0f);

	piece->offset.x = position.x;
	piece->offset.y = position.y;
	piece->offset.z = position.z;

	// for all meshes
	for ( unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; meshListIndex++ ) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		logOutput.Print(LOG_PIECE_DETAIL, "Fetching mesh %d from scene", meshIndex );
		aiMesh* mesh = model->scene->mMeshes[meshIndex];
		std::vector<unsigned> mesh_vertex_mapping;
		// extract vertex data
		logOutput.Print(LOG_PIECE_DETAIL, "Processing vertices for mesh %d (%d vertices)", meshIndex, mesh->mNumVertices );
		logOutput.Print(LOG_PIECE_DETAIL, "Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals())?"Y":"N",
				(mesh->HasTangentsAndBitangents())?"Y":"N",
				(mesh->HasTextureCoords(0)?"Y":"N")
		);
		for ( unsigned vertexIndex= 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			SAssVertex vertex;

			// vertex coordinates
			logOutput.Print(LOG_PIECE_DETAIL, "Fetching vertex %d from mesh", vertexIndex );
			aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			vertex.pos.x = aiVertex.x;
			vertex.pos.y = aiVertex.y;
			vertex.pos.z = aiVertex.z;

			// update piece min/max extents
			piece->mins.x = std::min(piece->mins.x, aiVertex.x);
			piece->mins.y = std::min(piece->mins.y, aiVertex.y);
			piece->mins.z = std::min(piece->mins.z, aiVertex.z);
			piece->maxs.x = std::max(piece->maxs.x, aiVertex.x);
			piece->maxs.y = std::max(piece->maxs.y, aiVertex.y);
			piece->maxs.z = std::max(piece->maxs.z, aiVertex.z);

			logOutput.Print(LOG_PIECE_DETAIL, "vertex position %d: %f %f %f", vertexIndex, vertex.pos.x, vertex.pos.y, vertex.pos.z );

			// vertex normal
			logOutput.Print(LOG_PIECE_DETAIL, "Fetching normal for vertex %d", vertexIndex );
			aiVector3D& aiNormal = mesh->mNormals[vertexIndex];
			vertex.hasNormal = !IS_QNAN(aiNormal);
			if ( vertex.hasNormal ) {
				vertex.normal.x = aiNormal.x;
				vertex.normal.y = aiNormal.y;
				vertex.normal.z = aiNormal.z;
				logOutput.Print(LOG_PIECE_DETAIL, "vertex normal %d: %f %f %f",vertexIndex, vertex.normal.x, vertex.normal.y,vertex.normal.z );
			}

			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				logOutput.Print(LOG_PIECE_DETAIL, "Fetching tangent for vertex %d", vertexIndex );
				aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				vertex.hasTangent = !IS_QNAN(aiTangent);
				if ( vertex.hasTangent ) {
					float3 tangent;
					tangent.x = aiTangent.x;
					tangent.y = aiTangent.y;
					tangent.z = aiTangent.z;
					logOutput.Print(LOG_PIECE_DETAIL, "vertex tangent %d: %f %f %f",vertexIndex, tangent.x, tangent.y,tangent.z );
					piece->sTangents.push_back(tangent);
					// bitangent is cross product of tangent and normal
					float3 bitangent;
					if ( vertex.hasNormal ) {
						bitangent = tangent.cross(vertex.normal);
						logOutput.Print(LOG_PIECE_DETAIL, "vertex bitangent %d: %f %f %f",vertexIndex, bitangent.x, bitangent.y,bitangent.z );
						piece->tTangents.push_back(bitangent);
					}
				}
			} else {
				vertex.hasTangent = false;
			}

			// vertex texcoords
			if (mesh->HasTextureCoords(0)) {
				vertex.textureX = mesh->mTextureCoords[0][vertexIndex].x;
				vertex.textureY = mesh->mTextureCoords[0][vertexIndex].y;
				logOutput.Print(LOG_PIECE_DETAIL, "vertex texcoords %d: %f %f", vertexIndex, vertex.textureX, vertex.textureY );
			}

			mesh_vertex_mapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
		}

		// extract face data
		if ( mesh->HasFaces() ) {
			logOutput.Print(LOG_PIECE_DETAIL, "Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);
			for ( unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++ ) {
					aiFace& face = mesh->mFaces[faceIndex];
					// get the vertex belonging to the mesh
					for ( unsigned vertexListID = 0; vertexListID < face.mNumIndices; vertexListID++ ) {
						unsigned int vertexID = mesh_vertex_mapping[face.mIndices[vertexListID]];
						logOutput.Print(LOG_PIECE_DETAIL, "face %d vertex %d", faceIndex, vertexID );
						piece->vertexDrawOrder.push_back(vertexID);
					}
			}
		}
	}

	// update model min/max extents
	model->mins.x = std::min(piece->mins.x, model->mins.x);
	model->mins.y = std::min(piece->mins.y, model->mins.y);
	model->mins.z = std::min(piece->mins.z, model->mins.z);
	model->maxs.x = std::max(piece->maxs.x, model->maxs.x);
	model->maxs.y = std::max(piece->maxs.y, model->maxs.y);
	model->maxs.z = std::max(piece->maxs.z, model->maxs.z);

	// collision volume for piece (not sure about these coords)
	const float3 cvScales = piece->maxs - piece->mins;
	const float3 cvOffset = (piece->maxs - piece->offset) + (piece->mins - piece->offset);
	//const float3 cvOffset(piece->offset.x, piece->offset.y, piece->offset.z);
	piece->colvol = new CollisionVolume("box", cvScales, cvOffset, CollisionVolume::COLVOL_HITTEST_CONT);

	// Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		SAssPiece* childPiece = LoadPiece(node->mChildren[i], model);
		piece->childs.push_back(childPiece);
	}

	logOutput.Print (LOG_PIECE_DETAIL, "AssParser: Loaded model piece: %s with %d meshes\n", piece->name.c_str(), node->mNumMeshes );
	return piece;
}

void DrawPiecePrimitive( const S3DModelPiece* o)
{
	if (o->isEmpty) {
		return;
	}
	logOutput.Print(LOG_PIECE_DETAIL, "Compiling piece %s", o->name.c_str());
	// Add GL commands to the pieces displaylist

	const SAssPiece* so = static_cast<const SAssPiece*>(o);
	const SAssVertex* sAssV = static_cast<const SAssVertex*>(&so->vertices[0]);


	// pass the tangents as 3D texture coordinates
	// (array elements are float3's, which are 12
	// bytes in size and each represent a single
	// xyz triple)
	// TODO: test if we have this many texunits
	// (if not, could only send the s-tangents)?

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->sTangents[0].x);
	}
	if (!so->tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SAssVertex), &sAssV->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SAssVertex), &sAssV->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SAssVertex), &sAssV->normal.x);

	// since aiProcess_SortByPType is being used, we're sure we'll get only 1 type here, so combination check isn't needed, also anything more complex than triangles is being split thanks to aiProcess_Triangulate
	glDrawElements(GL_TRIANGLES, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if (!so->tTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	logOutput.Print(LOG_PIECE_DETAIL, "Completed compiling piece %s", o->name.c_str());
}

void CAssParser::Draw( const S3DModelPiece* o) const
{
	DrawPiecePrimitive( o );
}

void SAssPiece::DrawList() const
{
	DrawPiecePrimitive(this);
}
