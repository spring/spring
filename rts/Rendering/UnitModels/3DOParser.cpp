// 3DOParser.cpp: implementation of the C3DOParser class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <vector>
#include <set>
#include <algorithm>
#include <cctype>
#include <locale>
#include <stdexcept>
#include "mmgr.h"

#include "Rendering/GL/myGL.h"
#include "LogOutput.h"
#include "Rendering/GL/VertexArray.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/SimpleParser.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/Textures/TAPalette.h"
#include "Matrix44f.h"
#include "Sim/Misc/Team.h"
#include "Game/Player.h"
#include "Platform/errorhandler.h"
#include "Platform/byteorder.h"
#include "SDL_types.h"
#include "3DOParser.h"
#include "s3oParser.h"
#include "Util.h"
#include "Exceptions.h"

using namespace std;

static const float scaleFactor = 1/(65536.0f);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define READ_3DOBJECT(o)					\
do {								\
	unsigned int __tmp;					\
	unsigned short __isize = sizeof(unsigned int);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).VersionSignature = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).NumberOfVertices = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).NumberOfPrimitives = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).SelectionPrimitive = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).XFromParent = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).YFromParent = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).ZFromParent = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToObjectName = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).Always_0 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToVertexArray = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToPrimitiveArray = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToSiblingObject = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToChildObject = (int)swabdword(__tmp);	\
} while (0)


#define READ_VERTEX(v)					\
do {							\
	unsigned int __tmp;				\
	unsigned short __isize = sizeof(unsigned int);	\
	SimStreamRead(&__tmp,__isize);			\
	(v).x = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);			\
	(v).y = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);			\
	(v).z = (int)swabdword(__tmp);			\
} while (0)


#define READ_PRIMITIVE(p)					\
do {								\
	unsigned int __tmp;					\
	unsigned short __isize = sizeof(unsigned int);		\
	SimStreamRead(&__tmp,__isize);				\
	(p).PaletteEntry = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(p).NumberOfVertexIndexes = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(p).Always_0 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(p).OffsetToVertexIndexArray = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(p).OffsetToTextureName = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(p).Unknown_1 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(p).Unknown_2 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(p).Unknown_3 = (int)swabdword(__tmp);			\
} while (0)



C3DOParser::C3DOParser()
{
	CFileHandler file("unittextures/tatex/teamtex.txt");
	CSimpleParser parser(file);

	while(!file.Eof()) {
		teamtex.insert(StringToLower(parser.GetLine()));
	}
}


S3DModel* C3DOParser::Load(string name)
{
	PUSH_CODE_MODE;
	ENTER_SYNCED;
//	ifstream ifs(name, ios::in|ios::binary);
	//int size=vfsHandler->GetFileSize(name);
	CFileHandler file(name);
	if(!file.FileExists()){
		POP_CODE_MODE;
		throw content_error("File not found: "+name);
	}
	fileBuf=new unsigned char[file.FileSize()];
	//vfsHandler->LoadFile(name,fileBuf);
	file.Read(fileBuf, file.FileSize());
	if (fileBuf == NULL) {
		delete [] fileBuf;
		throw content_error("Failed to read file "+name);
	}

	S3DModel *model = new S3DModel;
	S3DOPiece* object=new S3DOPiece;
	object->type=MODELTYPE_3DO;
	model->type=MODELTYPE_3DO;
	model->rootobject=object;
	model->textureType=0;
	object->isEmpty=true;
	model->name=name;
	model->numobjects=1;

	_3DObject root;
//	ifs.seekg(0);
//	ifs.read((char*)&root,sizeof(_3DObject));
	curOffset=0;
	READ_3DOBJECT(root);
	object->name = StringToLower(GetText(root.OffsetToObjectName));

	std::vector<float3> vertexes;

	GetVertexes(&root,object);
	GetPrimitives(object,root.OffsetToPrimitiveArray,root.NumberOfPrimitives,&vertexes,root.SelectionPrimitive);
	CalcNormals(object);
	if(root.OffsetToChildObject>0)
		if(!ReadChild(root.OffsetToChildObject,object,&model->numobjects))
			object->isEmpty=false;

	object->offset.x= root.XFromParent*scaleFactor;
	object->offset.y= root.YFromParent*scaleFactor;
	object->offset.z=-root.ZFromParent*scaleFactor;

	FindCenter(object);
	object->radius=FindRadius(object,-object->relMidPos);
	object->relMidPos.x=0;
	object->relMidPos.z=0;		//stupid but seems to work better
	if(object->relMidPos.y<1)
		object->relMidPos.y=1;

	model->radius = object->radius;
	model->height = FindHeight(object,ZeroVector);

	model->maxx=object->maxx;
	model->maxy=object->maxy;
	model->maxz=object->maxz;

	model->minx=object->minx;
	model->miny=object->miny;
	model->minz=object->minz;

	model->relMidPos=object->relMidPos;

	delete[] fileBuf;
	POP_CODE_MODE;
	return model;
}


void C3DOParser::GetVertexes(_3DObject* o,S3DOPiece* object)
{
	curOffset=o->OffsetToVertexArray;
	object->vertices.reserve(o->NumberOfVertices);
	for(int a=0;a<o->NumberOfVertices;a++){
		_Vertex v;
		READ_VERTEX(v);

		S3DOVertex vertex;
		float3 f;
		f.x=(v.x)*scaleFactor;
		f.y=(v.y)*scaleFactor;
		f.z=-(v.z)*scaleFactor;
		vertex.pos=f;
		object->vertices.push_back(vertex);
	}
}


void C3DOParser::GetPrimitives(S3DOPiece* obj,int pos,int num,vertex_vector* vv,int excludePrim)
{
	map<int,int> prevHashes;

	for(int a=0;a<num;a++){
		if(excludePrim==a){
			continue;
		}
		curOffset=pos+a*sizeof(_Primitive);
		_Primitive p;

		READ_PRIMITIVE(p);
		S3DOPrimitive sp;
		sp.numVertex=p.NumberOfVertexIndexes;

		if(sp.numVertex<3)
			continue;

		sp.vertices.reserve(sp.numVertex);
		sp.normals.reserve(sp.numVertex);

		curOffset=p.OffsetToVertexIndexArray;
		Uint16 w;

		list<int> orderVert;
		for(int b=0;b<sp.numVertex;b++){
			SimStreamRead(&w,2);
			w = swabword(w);
			sp.vertices.push_back(w);
			orderVert.push_back(w);
		}
		orderVert.sort();
		int vertHash=0;

		for(list<int>::iterator vi=orderVert.begin();vi!=orderVert.end();++vi)
			vertHash=(vertHash+(*vi))*(*vi);

		sp.texture=0;
		if(p.OffsetToTextureName!=0)
		{
			string texture = GetText(p.OffsetToTextureName);
			StringToLowerInPlace(texture);

			if(teamtex.find(texture) != teamtex.end())
				sp.texture=texturehandler3DO->Get3DOTexture(texture);
			else
				sp.texture=texturehandler3DO->Get3DOTexture(texture + "00");

			if(sp.texture==0)
				logOutput << "Parser couldnt get texture " << GetText(p.OffsetToTextureName).c_str() << "\n";
		} else {
			char t[50];
			sprintf(t,"ta_color%i",p.PaletteEntry);
			sp.texture=texturehandler3DO->Get3DOTexture(t);
		}
		float3 n=-(obj->vertices[sp.vertices[1]].pos-obj->vertices[sp.vertices[0]].pos).cross(obj->vertices[sp.vertices[2]].pos-obj->vertices[sp.vertices[0]].pos);
		n.Normalize();
		sp.normal=n;
		sp.normals.insert(sp.normals.begin(), sp.numVertex, n);

		//sometimes there are more than one selection primitive (??)
		if(n.dot(float3(0,-1,0))>0.99f){
			int ignore=true;

			if(sp.numVertex!=4) {
				ignore=false;
			} else {
				float3 s1=obj->vertices[sp.vertices[0]].pos-obj->vertices[sp.vertices[1]].pos;
				float3 s2=obj->vertices[sp.vertices[1]].pos-obj->vertices[sp.vertices[2]].pos;
				if(s1.SqLength()<900 || s2.SqLength()<900)
					ignore=false;

				if (ignore) {
					for(int a=0;a<sp.numVertex;++a) {
						if(obj->vertices[sp.vertices[a]].pos.y>0) {
							ignore=false;
							break;
						}
					}
				}
			}

			if(ignore)
				continue;
		}

		map<int,int>::iterator phi;
		if((phi=prevHashes.find(vertHash))!=prevHashes.end()){
			if(n.y>0)
				obj->prims[phi->second]=sp;
			continue;
		} else {
			prevHashes[vertHash]=obj->prims.size();
			obj->prims.push_back(sp);
			obj->isEmpty=false;
		}
		curOffset=p.OffsetToVertexIndexArray;

		for(int b=0;b<sp.numVertex;b++){
			SimStreamRead(&w,2);
			w = swabword(w);
			obj->vertices[w].prims.push_back(obj->prims.size()-1);
		}
	}
}


void C3DOParser::CalcNormals(S3DOPiece *o)
{
	for(std::vector<S3DOPrimitive>::iterator ps=o->prims.begin();ps!=o->prims.end();ps++){
		for(int a=0;a<ps->numVertex;++a){
			S3DOVertex* vertex=&o->vertices[ps->vertices[a]];
			float3 vnormal(0,0,0);
			for(std::vector<int>::iterator pi=vertex->prims.begin();pi!=vertex->prims.end();++pi){
				if(ps->normal.dot(o->prims[*pi].normal)>0.45f)
					vnormal+=o->prims[*pi].normal;
			}
			vnormal.Normalize();
			ps->normals[a]=vnormal;
		}
	}
}


std::string C3DOParser::GetText(int pos)
{
//	ifs->seekg(pos);
	curOffset=pos;
	char c;
	std::string s;
//	ifs->read(&c,1);
	SimStreamRead(&c,1);
	while(c!=0){
		s+=c;
//		ifs->read(&c,1);
		SimStreamRead(&c,1);
	}
	return s;
}


bool C3DOParser::ReadChild(int pos, S3DOPiece *root,int *numobj)
{
	(*numobj)++;

	S3DOPiece* object=new S3DOPiece;
	_3DObject me;

	curOffset=pos;
	READ_3DOBJECT(me);

	string s = StringToLower(GetText(me.OffsetToObjectName));
	object->name = s;
	object->displist = 0;
	object->type = MODELTYPE_3DO;

	object->offset.x= me.XFromParent*scaleFactor;
	object->offset.y= me.YFromParent*scaleFactor;
	object->offset.z=-me.ZFromParent*scaleFactor;
	std::vector<float3> vertexes;
	object->isEmpty=true;

	GetVertexes(&me,object);
	GetPrimitives(object,me.OffsetToPrimitiveArray,me.NumberOfPrimitives,&vertexes,-1/*me.SelectionPrimitive*/);
	CalcNormals(object);


	if(me.OffsetToChildObject>0){
		if(!ReadChild(me.OffsetToChildObject,object,numobj)){
			object->isEmpty=false;
		}
	}
	bool ret=object->isEmpty;

	object->vertexCount = object->vertices.size();

	root->childs.push_back(object);

	if(me.OffsetToSiblingObject>0)
		if(!ReadChild(me.OffsetToSiblingObject,root,numobj))
			ret=false;

	return ret;
}


void C3DOParser::Draw(S3DModelPiece *o)
{
	if (o->isEmpty)
		return;

	S3DOPiece* o3 = static_cast<S3DOPiece*>(o);

	CVertexArray* va=GetVertexArray();
	CVertexArray* va2=GetVertexArray();	//dont try to use more than 2 via getvertexarray, it wraps around
	va->Initialize();
	va2->Initialize();
	std::vector<S3DOPrimitive>::iterator ps;

	//glFrontFace(GL_CW);
	for(ps=o3->prims.begin();ps!=o3->prims.end();ps++){
		C3DOTextureHandler::UnitTexture* tex=ps->texture;
		if(ps->numVertex==4){
			va->AddVertexTN(o3->vertices[ps->vertices[0]].pos,tex->xstart,tex->ystart,ps->normals[0]);
			va->AddVertexTN(o3->vertices[ps->vertices[1]].pos,tex->xend,tex->ystart,ps->normals[1]);
			va->AddVertexTN(o3->vertices[ps->vertices[2]].pos,tex->xend,tex->yend,ps->normals[2]);
			va->AddVertexTN(o3->vertices[ps->vertices[3]].pos,tex->xstart,tex->yend,ps->normals[3]);
		} else if (ps->numVertex==3) {
			va2->AddVertexTN(o3->vertices[ps->vertices[0]].pos,tex->xstart,tex->ystart,ps->normals[0]);
			va2->AddVertexTN(o3->vertices[ps->vertices[1]].pos,tex->xend,tex->ystart,ps->normals[1]);
			va2->AddVertexTN(o3->vertices[ps->vertices[2]].pos,tex->xend,tex->yend,ps->normals[2]);
		} else {
			glNormal3f(ps->normal.x,ps->normal.y,ps->normal.z);
			glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(tex->xstart,tex->ystart);
			for(std::vector<int>::iterator fi=ps->vertices.begin();fi!=ps->vertices.end();fi++){
				float3 t=o3->vertices[(*fi)].pos;
				glNormalf3(ps->normal);
				glVertex3f(t.x,t.y,t.z);
			}
			glEnd();
		}
	}
	va->DrawArrayTN(GL_QUADS);
	if(va2->drawIndex()!=0)
		va2->DrawArrayTN(GL_TRIANGLES);
	//glFrontFace(GL_CCW);
}


void C3DOParser::SimStreamRead(void *buf, int length)
{
	memcpy(buf,&fileBuf[curOffset],length);
	curOffset+=length;
}


void C3DOParser::FindCenter(S3DOPiece* object)
{
	std::vector<S3DModelPiece*>::iterator si;
	for(si=object->childs.begin();si!=object->childs.end();++si){
		FindCenter((S3DOPiece*)*si);
	}

	float maxSize=0;
	float maxx=-1000,maxy=-1000,maxz=-1000;
	float minx=10000,miny=10000,minz=10000;

	std::vector<S3DOVertex>::iterator vi;
	for(vi=object->vertices.begin();vi!=object->vertices.end();++vi){
		maxx=max(maxx,vi->pos.x);
		maxy=max(maxy,vi->pos.y);
		maxz=max(maxz,vi->pos.z);

		minx=min(minx,vi->pos.x);
		miny=min(miny,vi->pos.y);
		minz=min(minz,vi->pos.z);
	}
	for(si=object->childs.begin();si!=object->childs.end();++si){
		maxx=max(maxx,(*si)->offset.x+(*si)->maxx);
		maxy=max(maxy,(*si)->offset.y+(*si)->maxy);
		maxz=max(maxz,(*si)->offset.z+(*si)->maxz);

		minx=min(minx,(*si)->offset.x+(*si)->minx);
		miny=min(miny,(*si)->offset.y+(*si)->miny);
		minz=min(minz,(*si)->offset.z+(*si)->minz);
	}

	object->maxx=maxx;
	object->maxy=maxy;
	object->maxz=maxz;
	object->minx=minx;
	object->miny=miny;
	object->minz=minz;

	object->relMidPos=float3((maxx+minx)*0.5f,(maxy+miny)*0.5f,(maxz+minz)*0.5f);

	for(vi=object->vertices.begin();vi!=object->vertices.end();++vi){
		maxSize=max(maxSize,object->relMidPos.distance(vi->pos));
	}
	for(si=object->childs.begin();si!=object->childs.end();++si){
		S3DOPiece* p3do = (S3DOPiece*)(*si);
		maxSize=max(maxSize,object->relMidPos.distance(p3do->offset+p3do->relMidPos)+p3do->radius);
	}
	object->radius=maxSize;
}


float C3DOParser::FindRadius(S3DOPiece* object,float3 offset)
{
	float maxSize=0;
	offset+=object->offset;

	std::vector<S3DModelPiece*>::iterator si;
	for(si=object->childs.begin();si!=object->childs.end();++si){
		float maxChild = FindRadius((S3DOPiece*)*si,offset);
		if(maxChild>maxSize)
			maxSize=maxChild;
	}

	std::vector<S3DOVertex>::iterator vi;

	for(vi=object->vertices.begin();vi!=object->vertices.end();++vi){
		maxSize=max(maxSize,(vi->pos+offset).Length());
	}

	return maxSize*0.8f;
}


float C3DOParser::FindHeight(S3DOPiece* object,float3 offset)
{
	float height=0;
	offset+=object->offset;

	std::vector<S3DModelPiece*>::iterator si;
	for(si=object->childs.begin();si!=object->childs.end();++si){
		float maxChild=FindHeight((S3DOPiece*)*si,offset);
		if(maxChild>height)
			height=maxChild;
	}

	for(std::vector<S3DOVertex>::iterator vi=object->vertices.begin();vi!=object->vertices.end();++vi){
		if(vi->pos.y+offset.y>height)
			height=vi->pos.y+offset.y;
	}
	return height;
}
