// s3oParser.cpp: implementation of the Cs3oParser class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <locale>
#include <stdexcept>
#include "mmgr.h"

#include "s3oParser.h"
#include "Rendering/GL/myGL.h"
#include "FileSystem/FileHandler.h"
#include "s3o.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Platform/byteorder.h"
#include "Platform/errorhandler.h"
#include "Util.h"
#include "Exceptions.h"
#include "LogOutput.h"


S3DModel* CS3OParser::Load(std::string name)
{
	CFileHandler file(name);
	if(!file.FileExists()){
		throw content_error("File not found: "+name);
	}
	unsigned char* fileBuf=new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());
	S3OHeader header;
	memcpy(&header,fileBuf,sizeof(header));
	header.swap();

	S3DModel *model = new S3DModel;
	model->type=MODELTYPE_S3O;
	model->numobjects=0;
	model->name=name;
	model->tex1=(char*)&fileBuf[header.texture1];
	model->tex2=(char*)&fileBuf[header.texture2];
	texturehandlerS3O->LoadS3OTexture(model);
	SS3OPiece* object=LoadPiece(fileBuf,header.rootPiece,model);
	object->type=MODELTYPE_S3O;

	FindMinMax(object);

	model->rootobject=object;
	model->radius = header.radius;
	model->height = header.height;
	model->relMidPos.x=header.midx;
	model->relMidPos.y=header.midy;
	model->relMidPos.z=header.midz;
	if(model->relMidPos.y<1)
		model->relMidPos.y=1;

	model->maxx=object->maxx;
	model->maxy=object->maxy;
	model->maxz=object->maxz;

	model->minx=object->minx;
	model->miny=object->miny;
	model->minz=object->minz;

	delete[] fileBuf;

	return model;
}

SS3OPiece* CS3OParser::LoadPiece(unsigned char* buf, int offset,S3DModel* model)
{
	model->numobjects++;

	SS3OPiece* piece = new SS3OPiece;
	piece->type = MODELTYPE_S3O;

	Piece* fp = (Piece*)&buf[offset];
	fp->swap(); // Does it matter we mess with the original buffer here? Don't hope so.

	piece->offset.x = fp->xoffset;
	piece->offset.y = fp->yoffset;
	piece->offset.z = fp->zoffset;
	piece->primitiveType = fp->primitiveType;
	piece->name = (char*)&buf[fp->name];

	int vertexPointer=fp->vertices;
	for(int a=0;a<fp->numVertices;++a){
		((Vertex*)&buf[vertexPointer])->swap();  // Does it matter we mess with the original buffer here?
		piece->vertices.push_back(*(SS3OVertex*)&buf[vertexPointer]);
/*		piece->vertices.back().normal.x=piece->vertices.back().pos.x;
		piece->vertices.back().normal.y=piece->vertices.back().pos.y;
		piece->vertices.back().normal.z=piece->vertices.back().pos.z;
		piece->vertices.back().normal.Normalize();*/
		vertexPointer+=sizeof(Vertex);
	}
	int vertexTablePointer=fp->vertexTable;
	for(int a=0;a<fp->vertexTableSize;++a){
		int num = swabdword(*(int*)&buf[vertexTablePointer]);
		piece->vertexDrawOrder.push_back(num);
		vertexTablePointer += sizeof(int);

		if(num==-1 && a!=fp->vertexTableSize-1){		//for triangle strips
			piece->vertexDrawOrder.push_back(num);

			num = swabdword(*(int*)&buf[vertexTablePointer]);
			piece->vertexDrawOrder.push_back(num);
		}
	}

	piece->isEmpty = false;//piece->vertexDrawOrder.empty(); 
	piece->vertexCount = piece->vertices.size();
	int childPointer = fp->childs;
	for(int a=0;a<fp->numChilds;++a){
		piece->childs.push_back(LoadPiece(buf,swabdword(*(int*)&buf[childPointer]),model));
		childPointer += sizeof(int);
	}
	return piece;
}

void CS3OParser::FindMinMax(SS3OPiece *object)
{
	std::vector<S3DModelPiece*>::iterator si;
	for(si=object->childs.begin(); si!=object->childs.end(); ++si){
		FindMinMax(static_cast<SS3OPiece*>(*si));
	}

	float maxx=-1000,maxy=-1000,maxz=-1000;
	float minx=10000,miny=10000,minz=10000;

	std::vector<SS3OVertex>::iterator vi;
	for(vi=object->vertices.begin();vi!=object->vertices.end();++vi){
		maxx=std::max(maxx,vi->pos.x);
		maxy=std::max(maxy,vi->pos.y);
		maxz=std::max(maxz,vi->pos.z);

		minx=std::min(minx,vi->pos.x);
		miny=std::min(miny,vi->pos.y);
		minz=std::min(minz,vi->pos.z);
	}
	for(si=object->childs.begin();si!=object->childs.end();++si){
		maxx=std::max(maxx,(*si)->offset.x+(*si)->maxx);
		maxy=std::max(maxy,(*si)->offset.y+(*si)->maxy);
		maxz=std::max(maxz,(*si)->offset.z+(*si)->maxz);

		minx=std::min(minx,(*si)->offset.x+(*si)->minx);
		miny=std::min(miny,(*si)->offset.y+(*si)->miny);
		minz=std::min(minz,(*si)->offset.z+(*si)->minz);
	}
	object->maxx=maxx;
	object->maxy=maxy;
	object->maxz=maxz;

	object->minx=minx;
	object->miny=miny;
	object->minz=minz;
}

void CS3OParser::Draw(S3DModelPiece *o)
{
	if (o->isEmpty)
		return;

	SS3OPiece* so = static_cast<SS3OPiece*>(o);
	SS3OVertex* s3ov = static_cast<SS3OVertex*>(&so->vertices[0]);

	glVertexPointer(3,GL_FLOAT,sizeof(SS3OVertex),&s3ov->pos.x);
	glTexCoordPointer(2,GL_FLOAT,sizeof(SS3OVertex),&s3ov->textureX);
	glNormalPointer(GL_FLOAT,sizeof(SS3OVertex),&s3ov->normal.x);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	switch(so->primitiveType){
	case 0:
		glDrawElements(GL_TRIANGLES,so->vertexDrawOrder.size(),GL_UNSIGNED_INT,&so->vertexDrawOrder[0]);
		break;
	case 1:
		glDrawElements(GL_TRIANGLE_STRIP,so->vertexDrawOrder.size(),GL_UNSIGNED_INT,&so->vertexDrawOrder[0]);
		break;
	case 2:
		glDrawElements(GL_QUADS,so->vertexDrawOrder.size(),GL_UNSIGNED_INT,&so->vertexDrawOrder[0]);
		break;
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}
