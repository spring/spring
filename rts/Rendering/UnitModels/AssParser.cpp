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
#include "s3oParser.h"
#include "AssIO.h"
#include "AssParser.h"

#include "assimp.hpp"
#include "aiDefines.h"
#include "aiTypes.h"
#include "aiScene.h"
#include "aiPostProcess.h"

#define ASS_POSTPROCESS_OPTIONS \
	aiProcess_CalcTangentSpace				|  \
	aiProcess_GenSmoothNormals				|  \
	aiProcess_JoinIdenticalVertices			|  \
	aiProcess_ImproveCacheLocality			|  \
	aiProcess_LimitBoneWeights				|  \
	aiProcess_RemoveRedundantMaterials      |  \
	aiProcess_SplitLargeMeshes				|  \
	aiProcess_Triangulate					|  \
	aiProcess_GenUVCoords

S3DModel* CAssParser::Load(std::string name)
{
	logOutput.Print ("Loading model: %s\n", name.c_str() );

	// Check if the file exists
	CFileHandler file(name);
	if (!file.FileExists()) {
		throw content_error("File not found: "+name);
	}

 	Assimp::Importer importer;

	// give the importer an IO class that handles Spring's VFS
	Assimp::IOSystem* iohandler = new AssVFSSystem();
	importer.SetIOHandler( iohandler );

	// read the model and texture files to build an assimp scene
	logOutput.Print("Reading model file: %s\n", name.c_str() );
	const aiScene* scene = importer.ReadFile( name, ASS_POSTPROCESS_OPTIONS );

	// convert to Spring model format
	S3DModel* model = new S3DModel;
	if (scene != NULL) {
		logOutput.Print("Processing scene for model: %s\n", name.c_str() );
		model->name = name;
		model->type = MODELTYPE_OTHER;
		model->scene = scene;
		model->numobjects = scene->mNumMeshes;
		model->rootobject = LoadPiece( scene->mRootNode );

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
		model->relMidPos.x = 0.0f;
		model->relMidPos.y = 0.0f;
		model->relMidPos.z = 0.0f;

		model->minx = 0.0f;
		model->miny = 0.0f;
		model->minz = 0.0f;

		model->maxx = 1.0f;
		model->maxy = 1.0f;
		model->maxz = 1.0f;
	} else {
		//logOutput.Print ("Model Import Error: %s\n",  importer.GetErrorString());
		handleerror(0,importer.GetErrorString(),"Model Import Error",0);
	}
	return model;
}

SAssPiece* CAssParser::LoadPiece(aiNode* node)
{
	SAssPiece* piece = new SAssPiece;
	piece->name = std::string(node->mName.data);
	piece->pos = float3(); // FIXME:Implement GetVertexPos properly
	piece->type = MODELTYPE_OTHER;
	piece->node = node;
	piece->isEmpty = node->mNumMeshes > 0;

	logOutput.Print("Positioning piece");
	aiVector3D scale, position;
 	aiQuaternion rotation;
	node->mTransformation.Decompose(scale,rotation,position);

	piece->offset.x = position.x;
	piece->offset.y = position.y;
	piece->offset.z = position.z;

	// collision volume for piece
	const float3 cvScales(1.0f, 1.0f, 1.0f);
	const float3 cvOffset(0.0f, 0.0f, 0.0f);
	piece->colvol = new CollisionVolume("box", cvScales, cvOffset, COLVOL_TEST_CONT);
	piece->colvol->Enable();

	// Recursively process all child pieces
	for (unsigned int i = 0; i < node->mNumChildren;++i) {
		SAssPiece* childPiece = LoadPiece(node->mChildren[i]);
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

void CAssParser::Draw( const S3DModelPiece* o) const
{
	logOutput.Print("Drawing piece");
	if (o->isEmpty) {
		return;
	}
	// Add GL commands to the pieces displaylist

	const SS3OPiece* so = static_cast<const SS3OPiece*>(o);
	const SS3OVertex* s3ov = static_cast<const SS3OVertex*>(&so->vertices[0]);


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

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), &s3ov->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SS3OVertex), &s3ov->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SS3OVertex), &s3ov->normal.x);

	switch (so->primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES:
			glDrawElements(GL_TRIANGLES, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);
			break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP:
			glDrawElements(GL_TRIANGLE_STRIP, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);
			break;
		case S3O_PRIMTYPE_QUADS:
			glDrawElements(GL_QUADS, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);
			break;
	}

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}
