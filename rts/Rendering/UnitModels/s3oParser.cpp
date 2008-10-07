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
#include "Rendering/FartextureHandler.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/Textures/TextureHandler.h"
#include "Platform/byteorder.h"
#include "Platform/errorhandler.h"
#include "System/Util.h"
#include "System/Exceptions.h"

void SS3O::DrawStatic()
{
	glPushMatrix();
	glTranslatef(offset.x,offset.y,offset.z);
	glCallList(displist);
	for(unsigned int i=0; i<childs.size(); i++)
		childs[i]->DrawStatic();
	glPopMatrix();
}

SS3O::~SS3O()
{
	glDeleteLists(displist, 1);
}


CS3OParser::CS3OParser()
{

}

CS3OParser::~CS3OParser()
{
	std::map<std::string,S3DOModel*>::iterator ui;
	for(ui=units.begin();ui!=units.end();++ui){
		DeleteSS3O(ui->second->rootobjects3o);
		delete ui->second;
	}
}

void CS3OParser::DeleteSS3O(SS3O* o)
{
	for(std::vector<SS3O*>::iterator di=o->childs.begin();di!=o->childs.end();di++){
		DeleteSS3O(*di);
	}
	delete o;
}

S3DOModel* CS3OParser::LoadS3O(std::string name,float scale,int side)
{
	if(name.find(".")==std::string::npos)
		name+=".s3o";

	StringToLowerInPlace(name);

	std::map<std::string,S3DOModel*>::iterator ui;
	if((ui=units.find(name))!=units.end()){
		return ui->second;
	}

	PUSH_CODE_MODE;
	ENTER_SYNCED;

	CFileHandler file(name);
	if(!file.FileExists()){
		POP_CODE_MODE;
		throw content_error("File not found: "+name);
	}
	unsigned char* fileBuf=SAFE_NEW unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());
	S3OHeader header;
	memcpy(&header,fileBuf,sizeof(header));
	header.swap();

	S3DOModel *model = SAFE_NEW S3DOModel;
	model->numobjects=0;
	SS3O* object=LoadPiece(fileBuf,header.rootPiece,model);
	model->rootobjects3o=object;
	model->rootobject3do=0;
	object->isEmpty=true;
	model->name=name;
	model->textureType=texturehandler->LoadS3OTexture((char*)&fileBuf[header.texture1],(char*)&fileBuf[header.texture2]);

	FindMinMax(object);

	units[name]=model;

	CreateLists(object);

	// this is a hack to make aircrafts less likely to collide and get hit by nontracking weapons
	// note: does not apply anymore, unit <--> projectile coldet no longer depends on model->radius
	model->radius = header.radius * scale;
	model->height = header.height;
	model->relMidPos.x=header.midx;
	model->relMidPos.y=header.midy;
	model->relMidPos.z=header.midz;
	if(model->relMidPos.y<1)
		model->relMidPos.y=1;

//	logOutput.Print("%s has height %f",name,model->height);
	fartextureHandler->CreateFarTexture(model);

	model->maxx=model->rootobjects3o->maxx;
	model->maxy=model->rootobjects3o->maxy;
	model->maxz=model->rootobjects3o->maxz;

	model->minx=model->rootobjects3o->minx;
	model->miny=model->rootobjects3o->miny;
	model->minz=model->rootobjects3o->minz;

	delete[] fileBuf;
	POP_CODE_MODE;
	return model;
}

LocalS3DOModel* CS3OParser::CreateLocalModel(S3DOModel *model, std::vector<struct PieceInfo> *pieces)
{
	LocalS3DOModel *lmodel = SAFE_NEW LocalS3DOModel;
	lmodel->numpieces = model->numobjects;

	int piecenum=0;
	lmodel->pieces = SAFE_NEW LocalS3DO[model->numobjects];
	lmodel->pieces->parent = NULL;
	lmodel->scritoa = SAFE_NEW int[pieces->size()];
	for (int a = 0; a < pieces->size(); ++a) {
		lmodel->scritoa[a]=-1;
	}

	CreateLocalModel(model->rootobjects3o, lmodel, pieces, &piecenum);

	return lmodel;
}

void CS3OParser::CreateLocalModel(SS3O *model, LocalS3DOModel *lmodel, std::vector<struct PieceInfo> *pieces, int *piecenum)
{
	PUSH_CODE_MODE;
	ENTER_SYNCED;
	lmodel->pieces[*piecenum].displist = model->displist;
	lmodel->pieces[*piecenum].offset = model->offset;
	lmodel->pieces[*piecenum].name = model->name;
	lmodel->pieces[*piecenum].originals3o = model;
	lmodel->pieces[*piecenum].original3do = 0;

	lmodel->pieces[*piecenum].anim = NULL;
	unsigned int cur;

	//Map this piecename to an index in the script's pieceinfo
	for (cur=0; cur<pieces->size(); cur++) {
		if (lmodel->pieces[*piecenum].name.compare((*pieces)[cur].name) == 0) {
			break;
		}
	}

	//Not found? Try again with partial matching
	if (cur == pieces->size()) {
		std::string &s1 = lmodel->pieces[*piecenum].name;
		for (cur = 0; cur < pieces->size(); ++cur) {
			std::string &s2 = (*pieces)[cur].name;
			int maxcompare = std::min(s1.size(), s2.size());
			int j;
			for (j = 0; j < maxcompare; ++j) {
				if (s1[j] != s2[j]) {
					break;
				}
			}
			//Match now?
			if (j == maxcompare) {
				break;
			}
		}
	}

	//Did we find it now?
	if (cur < pieces->size()) {
		lmodel->pieces[*piecenum].anim = &((*pieces)[cur]);
		lmodel->scritoa[cur] = *piecenum;
	}
	else {
//		logOutput.Print("CreateLocalModel: Could not map %s to script", lmodel->pieces[*piecenum].name.c_str());
	}

	int thispiece = *piecenum;

	for (unsigned int i=0; i<model->childs.size(); i++) {
		(*piecenum)++;
		lmodel->pieces[thispiece].childs.push_back(&lmodel->pieces[*piecenum]);
		lmodel->pieces[*piecenum].parent = &lmodel->pieces[thispiece];
		CreateLocalModel(model->childs[i], lmodel, pieces, piecenum);
	}

	POP_CODE_MODE;
}

SS3O* CS3OParser::LoadPiece(unsigned char* buf, int offset,S3DOModel* model)
{
	model->numobjects++;

	SS3O* piece=SAFE_NEW SS3O;

	Piece* fp=(Piece*)&buf[offset];
	fp->swap(); // Does it matter we mess with the original buffer here? Don't hope so.

	piece->offset.x=fp->xoffset;
	piece->offset.y=fp->yoffset;
	piece->offset.z=fp->zoffset;
	piece->primitiveType=fp->primitiveType;
	piece->name=(char*)&buf[fp->name];

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
		int num=swabdword(*(int*)&buf[vertexTablePointer]);
		piece->vertexDrawOrder.push_back(num);
		vertexTablePointer+=sizeof(int);

		if(num==-1 && a!=fp->vertexTableSize-1){		//for triangle strips
			piece->vertexDrawOrder.push_back(num);

			num=swabdword(*(int*)&buf[vertexTablePointer]);
			piece->vertexDrawOrder.push_back(num);
		}
	}
	int childPointer=fp->childs;
	for(int a=0;a<fp->numChilds;++a){
		piece->childs.push_back(LoadPiece(buf,swabdword(*(int*)&buf[childPointer]),model));
		childPointer+=sizeof(int);
	}
	return piece;
}

void CS3OParser::FindMinMax(SS3O *object)
{
	std::vector<SS3O*>::iterator si;
	for(si=object->childs.begin();si!=object->childs.end();++si){
		FindMinMax(*si);
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

void CS3OParser::DrawSub(SS3O* o)
{
	if (o->vertexDrawOrder.empty())
		return;

	glVertexPointer(3,GL_FLOAT,sizeof(SS3OVertex),&o->vertices[0].pos.x);
	glTexCoordPointer(2,GL_FLOAT,sizeof(SS3OVertex),&o->vertices[0].textureX);
	glNormalPointer(GL_FLOAT,sizeof(SS3OVertex),&o->vertices[0].normal.x);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	switch(o->primitiveType){
	case 0:
		glDrawElements(GL_TRIANGLES,o->vertexDrawOrder.size(),GL_UNSIGNED_INT,&o->vertexDrawOrder[0]);
		break;
	case 1:
		glDrawElements(GL_TRIANGLE_STRIP,o->vertexDrawOrder.size(),GL_UNSIGNED_INT,&o->vertexDrawOrder[0]);
		break;
	case 2:
		glDrawElements(GL_QUADS,o->vertexDrawOrder.size(),GL_UNSIGNED_INT,&o->vertexDrawOrder[0]);
		break;
	}
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

}


void CS3OParser::CreateLists(SS3O *o)
{
	o->displist = glGenLists(1);
	PUSH_CODE_MODE;
	ENTER_MIXED;
	glNewList(o->displist,GL_COMPILE);
	DrawSub(o);
	glEndList();
	POP_CODE_MODE;

	for(std::vector<SS3O*>::iterator bs=o->childs.begin();bs!=o->childs.end();bs++){
		CreateLists(*bs);
	}
}

