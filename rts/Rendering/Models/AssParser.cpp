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

// triangulate guarantees the most complex mesh is a triangle
// sortbytype ensure only 1 type of primitive type per mesh is used
#define ASS_POSTPROCESS_OPTIONS \
	aiProcess_FindInvalidData				| \
	aiProcess_CalcTangentSpace				| \
	aiProcess_GenSmoothNormals				| \
	aiProcess_LimitBoneWeights				| \
	aiProcess_RemoveRedundantMaterials		| \
	aiProcess_SplitLargeMeshes				| \
	aiProcess_Triangulate					| \
	aiProcess_GenUVCoords             		| \
	aiProcess_SortByPType					| \
	aiProcess_JoinIdenticalVertices

//aiProcess_ImproveCacheLocality

class AssLogStream : public Assimp::LogStream
{
public:
	AssLogStream() {}
	~AssLogStream() {}
	void write(const char* message)
	{
		logOutput.Print ("%s", message);
	}
};



S3DModel* CAssParser::Load(const std::string& name)
{
	logOutput.Print ("Loading model: %s\n", name.c_str() );

	// Check if the file exists
	CFileHandler file(name);
	if (!file.FileExists()) {
		throw content_error("File not found: "+name);
	}

 	Assimp::Importer importer;

	// give the importer an IO class that handles Spring's VFS
	importer.SetIOHandler( new AssVFSSystem() );

	// Select the kinds of messages you want to receive on this log stream
	Assimp::DefaultLogger::create("",Assimp::Logger::VERBOSE);
	const unsigned int severity = Assimp::Logger::DEBUGGING|Assimp::Logger::INFO|Assimp::Logger::ERR|Assimp::Logger::WARN;
	Assimp::DefaultLogger::get()->attachStream( new AssLogStream(), severity );

	// read the model and texture files to build an assimp scene
	logOutput.Print("Reading model file: %s\n", name.c_str() );
	const aiScene* scene = importer.ReadFile( name, ASS_POSTPROCESS_OPTIONS );

	// convert to Spring model format
	S3DModel* model = new S3DModel;
	if (scene != NULL) {
		logOutput.Print("Processing scene for model: %s (%d meshes / %d materials / %d textures)", name.c_str(), scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures );
		model->name = name;
		model->type = MODELTYPE_ASS;
		model->scene = scene;
		model->numobjects = 0;
		model->tex1 = "weapons1.png";
		model->tex2 = "faketex2.png";
		texturehandlerS3O->LoadS3OTexture(model);

		logOutput.Print("Loading root node '%s'", scene->mRootNode->mName.data);
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

		// Default collision volume
		model->radius = 1.0f;
		model->height = 1.0f;
		model->relMidPos = float3(0.0f,0.0f,0.0f);

		model->mins = float3(0.0f,0.0f,0.0f);
		model->maxs = float3(1.0f,1.0f,1.0f);

		logOutput.Print ("Model Imported.\n");
	} else {
		logOutput.Print ("Model Import Error: %s\n",  importer.GetErrorString());
	}
	return model;
}

#define IS_QNAN(f) (f != f)

SAssPiece* CAssParser::LoadPiece(aiNode* node, S3DModel* model)
{
	logOutput.Print("Converting node '%s' to a piece (%d meshes).", node->mName.data, node->mNumMeshes);

	model->numobjects++;

	SAssPiece* piece = new SAssPiece;
	piece->name = std::string(node->mName.data);
	piece->type = MODELTYPE_ASS;
	piece->node = node;
	piece->isEmpty = node->mNumMeshes == 0;

	aiVector3D scale, position;
 	aiQuaternion rotation;
	node->mTransformation.Decompose(scale,rotation,position);

	piece->mins = float3(0.0f,0.0f,0.0f);
	piece->maxs = float3(1.0f,1.0f,1.0f);

	piece->offset.x = position.x;
	piece->offset.y = position.y;
	piece->offset.z = position.z;

	// for all meshes
	for ( unsigned meshListIndex = 0; meshListIndex < node->mNumMeshes; meshListIndex++ ) {
		unsigned int meshIndex = node->mMeshes[meshListIndex];
		logOutput.Print("Fetching mesh %d from scene", meshIndex );
		aiMesh* mesh = model->scene->mMeshes[meshIndex];
		std::vector<unsigned> mesh_vertex_mapping;
		// extract vertex data
		logOutput.Print("Processing vertices for mesh %d (%d vertices)", meshIndex, mesh->mNumVertices );
		logOutput.Print("Normals: %s Tangents/Bitangents: %s TexCoords: %s",
				(mesh->HasNormals())?"Y":"N",
				(mesh->HasTangentsAndBitangents())?"Y":"N",
				(mesh->HasTextureCoords(0)?"Y":"N")
		);
		for ( unsigned vertexIndex= 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			SAssVertex vertex;
			// vertex coordinates
			//logOutput.Print("Fetching vertex %d from mesh", vertexIndex );
			aiVector3D& aiVertex = mesh->mVertices[vertexIndex];
			vertex.pos.x = aiVertex.x;
			vertex.pos.y = aiVertex.y;
			vertex.pos.z = aiVertex.z;
			//logOutput.Print("vertex %d: %f %f %f",vertexIndex, vertex.pos.x, vertex.pos.y,vertex.pos.z );
			// vertex normal
			//logOutput.Print("Fetching normal for vertex %d", vertexIndex );
			aiVector3D& aiNormal = mesh->mNormals[vertexIndex];
			vertex.hasNormal = !IS_QNAN(aiNormal);
			if ( vertex.hasNormal ) {
				vertex.normal.x = aiNormal.x;
				vertex.normal.y = aiNormal.y;
				vertex.normal.z = aiNormal.z;
				//logOutput.Print("vertex normal %d: %f %f %f",vertexIndex, vertex.normal.x, vertex.normal.y,vertex.normal.z );
			}
			// vertex tangent, x is positive in texture axis
			if (mesh->HasTangentsAndBitangents()) {
				//logOutput.Print("Fetching tangent for vertex %d", vertexIndex );
				aiVector3D& aiTangent = mesh->mTangents[vertexIndex];
				vertex.hasTangent = !IS_QNAN(aiTangent);
				if ( vertex.hasTangent ) {
					float3 tangent;
					tangent.x = aiTangent.x;
					tangent.y = aiTangent.y;
					tangent.z = aiTangent.z;
					//logOutput.Print("vertex tangent %d: %f %f %f",vertexIndex, tangent.x, tangent.y,tangent.z );
					piece->sTangents.push_back(tangent);
					// bitangent is cross product of tangent and normal
					float3 bitangent;
					if ( vertex.hasNormal ) {
						bitangent = tangent.cross(vertex.normal);
						//logOutput.Print("vertex bitangent %d: %f %f %f",vertexIndex, bitangent.x, bitangent.y,bitangent.z );
						piece->tTangents.push_back(bitangent);
					}
				}
			} else {
				vertex.hasTangent = false;
			}
			mesh_vertex_mapping.push_back(piece->vertices.size());
			piece->vertices.push_back(vertex);
			logOutput.Print("Finished vertex %d", vertexIndex );
		}
		// extract face data
		logOutput.Print("Processing faces for mesh %d (%d faces)", meshIndex, mesh->mNumFaces);
		if ( mesh->HasFaces() ) {
			for ( unsigned faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++ ) {
					aiFace& face = mesh->mFaces[faceIndex];
					// get the vertex belonging to the mesh
					for ( unsigned vertexListID = 0; vertexListID < face.mNumIndices; vertexListID++ ) {
						unsigned int vertexID = mesh_vertex_mapping[face.mIndices[vertexListID]];
						//logOutput.Print("face %d vertex %d", faceIndex, vertexID );
						piece->vertexDrawOrder.push_back(vertexID);
					}
			}
		}
	}


	// collision volume for piece
	const float3 cvScales(1.0f, 1.0f, 1.0f);
	const float3 cvOffset(piece->offset.x, piece->offset.y, piece->offset.z);
	piece->colvol = new CollisionVolume("box", cvScales, cvOffset, CollisionVolume::COLVOL_HITTEST_CONT);

	// Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		SAssPiece* childPiece = LoadPiece(node->mChildren[i], model);
		piece->childs.push_back(childPiece);
	}

	logOutput.Print ("Loaded model piece: %s with %d meshes\n", piece->name.c_str(), node->mNumMeshes );
	return piece;
}


/* Walks a nodes children retreiving all transformed child meshes as an array
void GetNodeVertices( aiNode node, Matrix4x4 accTransform)
{
  SceneObject parent;
  Matrix4x4 transform;

  // if node has meshes, create a new scene object for it
  if( node.mNumMeshes > 0)
  {
    SceneObjekt newObject = new SceneObject;
    targetParent.addChild( newObject);
    // copy the meshes
    CopyMeshes( node, newObject);

    // the new object is the parent for all child nodes
    parent = newObject;
    transform.SetUnity();
  } else
  {
    // if no meshes, skip the node, but keep its transformation
    parent = targetParent;
    transform = node.mTransformation * accTransform;
  }

  // continue for all child nodes
  for( all node.mChildren)
    CopyNodesWithMeshes( node.mChildren[a], parent, transform);
}
*/

void DrawPiecePrimitive( const S3DModelPiece* o)
{
	if (o->isEmpty) {
		return;
	}
	logOutput.Print("Compiling piece %s", o->name.c_str());
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

	logOutput.Print("Completed piece %s", o->name.c_str());
}

void CAssParser::Draw( const S3DModelPiece* o) const
{
	DrawPiecePrimitive( o );
}

void SAssPiece::DrawList() const
{
	DrawPiecePrimitive(this);
}
