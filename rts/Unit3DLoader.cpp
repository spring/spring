// Unit3DLoader.cpp: implementation of the CUnit3DLoader class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Unit3DLoader.h"
#include <fstream>
#include "InfoConsole.h"
#include "myGL.h"
#include <GL/glu.h>
#include "TextureHandler.h"
#include "VertexArray.h"
#include "FileHandler.h"
#include "Bitmap.h"
#include "Unit3DLoader.h"
#include "UnitHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnit3DLoader* unitModelLoader=0;

CUnit3DLoader::CUnit3DLoader()
: usedFarTextures(0)
{
	farTextureMem=new unsigned char[1024*1024*4];

	for(int a=0;a<1024*1024*4;++a)
		farTextureMem[a]=0;
	
	farTexture=0;
}

CUnit3DLoader::~CUnit3DLoader()
{
	delete[] farTextureMem;
	glDeleteTextures (1, &farTexture);

	delete texturehandler;
	texturehandler=0;

	map<string,UnitModelGeometry*>::iterator ugi;
	for(ugi=geometryModels.begin();ugi!=geometryModels.end();ugi++){
		map<string,Animation*>::iterator ai;
		for(ai=ugi->second->animations.begin();ai!=ugi->second->animations.end();ai++){
			delete ai->second;
		}
		if(ugi->second->indexBuffer)
			glDeleteBuffersARB(1,&ugi->second->indexBuffer);
		if(ugi->second->normalBuffer)
			glDeleteBuffersARB(1,&ugi->second->normalBuffer);
		delete ugi->second;
	}

	map<string,UnitModel*>::iterator ui;
	for(ui=models.begin();ui!=models.end();ui++){
		DeleteModel(ui->second);
	}

}


CUnit3DLoader::UnitModel* CUnit3DLoader::GetModel(string name,int team)
{
	char c[50];
	sprintf(c,"team%d_",team);
	string modname=string(c)+name;

	map<string,UnitModel*>::iterator ui;
	if((ui=models.find(modname))!=models.end())
			return ui->second;
	
	if(texturehandler==0)
		texturehandler=new CTextureHandler;

	UnitModel* model=new UnitModel;

	model->team=team;
	model->name=name;

	Parse(name,*model);
	CreateNormals(*model);
	CreateDispList(*model);
	for(std::vector<UnitModel*>::iterator umi=model->subModels.begin();umi!=model->subModels.end();++umi)
		CreateDispList(**umi);
	CreateFarTexture(*model);

	models[modname]=model;
	return model;
}

void CUnit3DLoader::CreateDispList(UnitModel &model)
{
	model.displist=CreateDisplistFromVector(model,model.geometry->vertex);
	if(model.geometry->animations.find("aim")!=model.geometry->animations.end()){
		Animation& anim=*model.geometry->animations["aim"];
		model.displistAim=CreateDisplistFromVector(model,anim.frames[anim.frames.size()-1].vertex,false);
	}
	if(model.geometry->isAnimated)
		CreateArrays(model);
}

void CUnit3DLoader::CreateNormals(UnitModel &model)
{
	vector<Quad>::iterator fi;
	vector<Tri>::iterator ti;

	UnitModelGeometry& geometry=*model.geometry;
	float invNormals=1;
	if(model.name.find(".s3o")!=-1)
		invNormals=-1;

	for(fi=geometry.quad.begin();fi!=geometry.quad.end();++fi){
		float3 norm=(geometry.vertex[fi->verteces[1]]-geometry.vertex[fi->verteces[0]]).cross(geometry.vertex[fi->verteces[3]]-geometry.vertex[fi->verteces[0]]);
		norm.Normalize();
		fi->normal=norm*invNormals;
	}
	for(fi=geometry.quad.begin();fi!=geometry.quad.end();++fi){
		if(fi->normalType==1){
			for(int a=0;a<4;++a)
				geometry.vertexNormal[fi->verteces[a]]+=fi->normal;
		}
	}
	for(ti=geometry.tri.begin();ti!=geometry.tri.end();++ti){
		float3 norm=(geometry.vertex[ti->verteces[1]]-geometry.vertex[ti->verteces[0]]).cross(geometry.vertex[ti->verteces[2]]-geometry.vertex[ti->verteces[0]]);
		norm.Normalize();
		ti->normal=norm*invNormals;
	}
	for(ti=geometry.tri.begin();ti!=geometry.tri.end();++ti){
		if(ti->normalType==1){
			for(int a=0;a<3;++a)
				geometry.vertexNormal[ti->verteces[a]]+=ti->normal;
		}
	}
	vector<float3>::iterator vi;
	for(vi=model.geometry->vertexNormal.begin();vi!=model.geometry->vertexNormal.end();++vi){
		vi->Normalize();
	}
}

int CUnit3DLoader::Parse(const string& filename, UnitModel &model)
{
	string dir="unitmodels\\";
	CFileHandler ifs(dir+filename);
	return ParseSub(ifs,model,filename,float3(0,0,0),model.name);
}

int CUnit3DLoader::ParseSub(CFileHandler& ifs, UnitModel &model,const string& filename,float3 offset,const string& treename)
{
	string s;
	bool inComment=false;

	Animation* curAnim=0;
	int curAnimFrame=0;

	bool firstGeometryPass=false;

	if(geometryModels.find(treename)==geometryModels.end()){
		geometryModels[treename]=new UnitModelGeometry();
		UnitModelGeometry& geometry=*geometryModels[treename];
		firstGeometryPass=true;
		geometry.normalBuffer=0;
		geometry.indexBuffer=0;
		geometry.numIndeces=0;
		geometry.numVerteces=0;
	}
	model.geometry=geometryModels[treename];
	UnitModelGeometry& geometry=*model.geometry;
	
	geometry.radius=1;
	geometry.height=1;
	geometry.isAnimated=false;

	model.texCoordBuffer=0;

	while(ifs.Peek()!=EOF){
		s=GetWord(ifs);
		MakeLow(s);
		if(s[0]=='/' && !inComment){
			if(s[1]=='/'){
				GetLine(ifs);
				continue;
			}
			if(s[1]=='*')
				inComment=true;
		}
		if(inComment){
			for(int a=0;a<s.size()-1;++a)
				if(s[a]=='*' && s[a+1]=='/')
					inComment=false;
			if(inComment)
				continue;
		}
		if(ifs.Eof())
			break;
		while((s.c_str()[0]=='/') || (s.c_str()[0]=='\n')){
			s=GetLine(ifs);
			s=GetWord(ifs);
			MakeLow(s);
			if(ifs.Eof())
				break;
		}
		if(s=="vertex"){
			int num=atoi(GetWord(ifs).c_str());
			float3 v;
			v.x=atof(GetWord(ifs).c_str());
			v.y=atof(GetWord(ifs).c_str());
			v.z=atof(GetWord(ifs).c_str());
			v-=offset;

			if(firstGeometryPass){
				if(curAnim==0){
					while(geometry.vertex.size()<=num){
						geometry.vertex.push_back(float3(0,0,0));
						geometry.vertexNormal.push_back(float3(0,0,0));
					}
					geometry.vertex[num]=v;
				} else {
					AnimFrame* af=&curAnim->frames[curAnimFrame];
					while(af->vertex.size()<=num){
						af->vertex.push_back(float3());
	//					model.vertexNormal.push_back(float3(0,0,0));
					}
					af->vertex[num]=v;
				}
			}
		} else if(s=="quad"){
			Quad q;
			QuadTex qt;
			for(int a=0;a<4;++a)
				q.verteces[a]=atoi(GetWord(ifs).c_str());
			for(int a=0;a<8;++a)
				qt.texPos[0][a]=atof(GetWord(ifs).c_str());
			qt.texName=GetWord(ifs);
			qt.teamTex=atoi(GetWord(ifs).c_str());
			q.normalType=atoi(GetWord(ifs).c_str());
			model.quadTex.push_back(qt);
			if(firstGeometryPass)
				geometry.quad.push_back(q);

		} else if(s=="tri"){
			Tri t;
			TriTex tt;
			for(int a=0;a<3;++a)
				t.verteces[a]=atoi(GetWord(ifs).c_str());
			for(int a=0;a<3;++a){
				tt.texPos[0][a*2]=1-atof(GetWord(ifs).c_str());
				tt.texPos[0][a*2+1]=1-atof(GetWord(ifs).c_str());
			}
			tt.texName=GetWord(ifs);
			tt.teamTex=atoi(GetWord(ifs).c_str());
			t.normalType=atoi(GetWord(ifs).c_str());
			model.triTex.push_back(tt);
			if(firstGeometryPass)
				geometry.tri.push_back(t);

		} else if(s=="propeller"){
			Propeller p;
			for(int a=0;a<3;++a)
				p.pos[a]=atof(GetWord(ifs).c_str());
			p.size=atof(GetWord(ifs).c_str());
			if(firstGeometryPass)
				geometry.propellers.push_back(p);

		} else if(s=="height"){
			geometry.height=atof(GetWord(ifs).c_str());

		} else if(s=="radius"){
			geometry.radius=atof(GetWord(ifs).c_str());

		} else if(s=="subobject"){
			UnitModel* um=new UnitModel;
			model.subModels.push_back(um);
			um->name=GetWord(ifs);
			float3 off,rot;
			for(int a=0;a<3;++a)
				off[a]=atof(GetWord(ifs).c_str());
			for(int a=0;a<3;++a)
				rot[a]=atof(GetWord(ifs).c_str());
			um->team=model.team;

			char c[100];
			sprintf(c,"%s%d",treename.c_str(),model.subModels.size());
			ParseSub(ifs,*um,filename,off,c);

			um->geometry->offset=off;
			um->geometry->rotVector=rot;
		} else if(s=="endsubobject"){
			return 1;

		} else if(s=="animation"){
			string name=GetWord(ifs);
			if(name=="end")
				curAnim=0;
			else{
				if(geometry.animations.find(name)==geometry.animations.end()){
					geometry.animations[name]=new Animation;
				}
				curAnim=geometry.animations[name];
			}
			curAnimFrame=0;
			geometry.isAnimated=true;

		} else if(s=="keyframe"){
			string num=GetWord(ifs);
			if(num=="end"){
				curAnimFrame=0;
			} else {
				curAnimFrame=atoi(num.c_str());
				float length=atoi(GetWord(ifs).c_str())/25.0f*30.0f;
				if(curAnim){
					while(curAnim->frames.size()<=curAnimFrame){
						curAnim->frames.push_back(AnimFrame());
					}
					curAnim->frames[curAnimFrame].length=length;
				}
			}

		} else {
			if(s!="")
				(*info) << "Unknown token " << s.c_str() << " in " << filename.c_str() << "\n";
		}
	}
	return 1;
}

void CUnit3DLoader::MakeLow(std::string &s)
{
	std::string s2=s;
	s="";
	std::string::iterator si;
	for(si=s2.begin();si!=s2.end();si++){
		if(((*si)>='A') && ((*si)<='Z'))
			s+=(char)('a'-'A'+(*si));
		else
			s+=(*si);
	}
}

string CUnit3DLoader::GetWord(CFileHandler& fh)
{
	char a=fh.Peek();
	while(a==' ' || a=='\xd' || a=='\xa'){
		fh.Read(&a,1);
		a=fh.Peek();
		if(fh.Eof())
			break;
	}
	string s="";
	fh.Read(&a,1);
	while(a!=',' && a!=' ' && a!='\xd' && a!='\xa'){
		s+=a;
		fh.Read(&a,1);
		if(fh.Eof())
			break;
	}
	return s;
}

string CUnit3DLoader::GetLine(CFileHandler& fh)
{
	string s="";
	char a;
	fh.Read(&a,1);
	while(a!='\xd' && a!='\xa'){
		s+=a;
		fh.Read(&a,1);
		if(fh.Eof())
			break;
	}
	return s;
}

extern GLfloat LightDiffuseLand[];
extern GLfloat LightAmbientLand[];
extern GLfloat FogLand[]; 

GLfloat LightDiffuseLand2[]=	{ 0.0f, 0.0f, 0.0f, 1.0f };
GLfloat LightAmbientLand2[]=	{ 0.6f, 0.6f, 0.6f, 1.0f };

void CUnit3DLoader::CreateFarTexture(UnitModel &model)
{
	UnitModelGeometry& geometry=*model.geometry;

	model.farTextureNum=usedFarTextures;
	int bx=(usedFarTextures%8)*128;
	int by=(usedFarTextures/8)*16;
	if(model.name=="wall.txt"){
		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbientLand);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuseLand);
		glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand);
	} else {
		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbientLand2);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuseLand2);
		glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand2);
	}

	glViewport(0,0,16,16);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();		
	glOrtho(-geometry.radius,geometry.radius,-geometry.radius,geometry.radius,-geometry.radius*1.5,geometry.radius*1.5);
	glMatrixMode(GL_MODELVIEW);
	glClearColor(0.5f,0.5f,0.5f,0);
	glDisable(GL_BLEND);

	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glAlphaFunc(GL_GREATER,0.05f);
	glEnable(GL_ALPHA_TEST);
	float cols[]={1,1,1,1};
	float cols2[]={1,1,1,1};
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,cols);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,cols2);
	glColor3f(1,1,1);
	glRotatef(10,1,0,0);
	glLightfv(GL_LIGHT1, GL_POSITION,gs->sunVector4);

	unsigned char buf[16*16*4];
	for(int a=0;a<8;++a){
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		texturehandler->SetTexture();
		glCallList(model.displist);
//		glBindTexture(GL_TEXTURE_2D, farTexture);
		glReadPixels(0,0,16,16,GL_RGBA,GL_UNSIGNED_BYTE,buf);
		for(int y=0;y<16;++y)
			for(int x=0;x<16;++x){
				farTextureMem[(bx+x+(by+y)*1024)*4]=buf[(x+y*16)*4];
				farTextureMem[(bx+x+(by+y)*1024)*4+1]=buf[(x+y*16)*4+1];
				farTextureMem[(bx+x+(by+y)*1024)*4+2]=buf[(x+y*16)*4+2];
				if(buf[(x+y*16)*4]==128 && buf[(x+y*16)*4+1]==128 && buf[(x+y*16)*4+2]==128)
					farTextureMem[(bx+x+(by+y)*1024)*4+3]=0;
				else
					farTextureMem[(bx+x+(by+y)*1024)*4+3]=255;
			}
		bx+=16;
//		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,16,16);
		glRotatef(45,0,1,0);
		glLightfv(GL_LIGHT1, GL_POSITION,gs->sunVector4);
	}
	
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glClearColor(FogLand[0],FogLand[1],FogLand[2],0);
	glViewport(0,0,gu->screenx,gu->screeny);
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbientLand);		// Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuseLand);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand);		// Setup The Diffuse Light

	if(farTexture==0){
		glGenTextures(1, &farTexture);
		glBindTexture(GL_TEXTURE_2D, farTexture);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, 1024, 1024,0,GL_RGBA, GL_UNSIGNED_BYTE, farTextureMem);
//		gluBuild2DMipmaps(GL_TEXTURE_2D,4 ,1024, 1024, GL_RGBA, GL_UNSIGNED_BYTE, farTextureMem);
	}
	glBindTexture(GL_TEXTURE_2D, farTexture);
//	glTexImage2D(GL_TEXTURE_2D,0,4, 1024, 1024,0,GL_RGBA, GL_UNSIGNED_BYTE, farTextureMem);
	glTexSubImage2D(GL_TEXTURE_2D,0, 0,0,1024, 1024,GL_RGBA, GL_UNSIGNED_BYTE, farTextureMem);
	usedFarTextures++;
/*	CBitmap b(farTextureMem,1024,1024);
	b.Save("fartex.bmp");*/
}

void CUnit3DLoader::DeleteModel(UnitModel* model)
{
	for(vector<UnitModel*>::iterator mi=model->subModels.begin();mi!=model->subModels.end();++mi){
		DeleteModel(*mi);
	}
//	for(map<string,Animation*>::iterator ai=model->animations.begin();ai!=model->animations.end();++ai){
//		delete ai->second;
//	}
	if(model->texCoordBuffer)
		glDeleteBuffersARB(1,&model->texCoordBuffer);
	delete model;
}

void CUnit3DLoader::CreateArrays(UnitModel& model)
{
	UnitModelGeometry& geometry=*model.geometry;
	vector<ArrayVertex>& va=geometry.vertexSplitting;
	bool newSplit=false;


	if(va.empty()){
		newSplit=true;
		va.resize(geometry.vertex.size());

		for(int a=0;a<geometry.vertex.size();++a){
			ArrayVertex& v=va[a];
			ArrayVertexSub vs;
			vs.num=a;
			vs.texCoord[0]=-1;
			v.sub.push_back(vs);
		}
	} else {
		for(vector<ArrayVertex>::iterator vi=va.begin();vi!=va.end();++vi){
			for(vector<ArrayVertexSub>::iterator vsi=vi->sub.begin();vsi!=vi->sub.end();++vsi){
				vsi->texCoord[0]=-1;
			}
		}
	}

	vector<Tri>::iterator ti;
	vector<TriTex>::iterator tti;
	int newNum=geometry.vertex.size();
	for(ti=geometry.tri.begin(),tti=model.triTex.begin();ti!=geometry.tri.end();++ti,++tti){	//split on texcoord
		for(int b=0;b<3;++b){
			ArrayVertex& v=va[ti->verteces[b]];
			vector<ArrayVertexSub>::iterator vi;
			bool createNew=true;
			for(vi=v.sub.begin();vi!=v.sub.end();++vi){
				if(vi->texCoord[0]==-1){
					vi->texCoord[0]=tti->texPos[b][0];
					vi->texCoord[1]=tti->texPos[b][1];
				}
				if(vi->texCoord[0]==tti->texPos[b][0] && vi->texCoord[0]==tti->texPos[b][0]){
					createNew=false;
					break;
				}
			}
			if(createNew){
				if(!newSplit){
					MessageBox(0,"Impossible split","error in unit3dloader",0);
				}
				ArrayVertexSub vs;
				vs.num=newNum++;
				vs.texCoord[0]=tti->texPos[b][0];
				vs.texCoord[1]=tti->texPos[b][1];
				v.sub.push_back(vs);
			}
		}
	}

	if(newSplit){
		SplitArray(geometry.vertex,geometry.vertexSplitting);//split the main frame;
		for(map<string,Animation*>::iterator ai=geometry.animations.begin();ai!=geometry.animations.end();++ai){//split the animation frames
			for(vector<AnimFrame>::iterator fi=ai->second->frames.begin();fi!=ai->second->frames.end();++fi){
				SplitArray(fi->vertex,geometry.vertexSplitting);
			}
		}
	}

	if(!model.texCoordBuffer){
		glGenBuffersARB( 1, &model.texCoordBuffer );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, model.texCoordBuffer );
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, geometry.vertex.size()*2*sizeof(float), 0, GL_STATIC_DRAW_ARB );

		float* texCoordBuf=(float*)glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB);
		if(!texCoordBuf){
			MessageBox(0,"glMapBuffer failed","Exiting",0);
			exit(0);
		}

		for(vector<ArrayVertex>::iterator vi=va.begin();vi!=va.end();++vi){
			for(vector<ArrayVertexSub>::iterator vsi=vi->sub.begin();vsi!=vi->sub.end();++vsi){
				texCoordBuf[vsi->num*2]=vsi->texCoord[0];
				texCoordBuf[vsi->num*2+1]=vsi->texCoord[1];

			}
		}

		if(!glUnmapBufferARB(GL_ARRAY_BUFFER_ARB)){
			MessageBox(0,"glUnmapBuffer failed","Exiting",0);
			exit(0);
		}
	}

	if(!geometry.indexBuffer){
		glGenBuffersARB( 1, &geometry.indexBuffer );
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, geometry.indexBuffer );
		glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, geometry.tri.size()*3*sizeof(unsigned short), 0, GL_STATIC_DRAW_ARB );
		
		unsigned short* indexBuf=(unsigned short*)glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB);
		if(!indexBuf){
			MessageBox(0,"glMapBuffer failed","Exiting",0);
			exit(0);
		}

		int ip=0;
		vector<Tri>::iterator ti;
		vector<TriTex>::iterator tti;
		for(ti=geometry.tri.begin(),tti=model.triTex.begin();ti!=geometry.tri.end();++ti,++tti){
			for(int a=0;a<3;++a){
				int vert=ti->verteces[a];
				vector<ArrayVertexSub>& avs=va[vert].sub;
				for(vector<ArrayVertexSub>::iterator vsi=avs.begin();vsi!=avs.end();++vsi){
					if(vsi->texCoord[0]==tti->texPos[a][0] && vsi->texCoord[1]==tti->texPos[a][1]){
						vert=vsi->num;
						break;
					}
				}
				indexBuf[ip++]=vert;
			}
		}
		if(!glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB)){
			MessageBox(0,"glUnmapBuffer failed","Exiting",0);
			exit(0);
		}
		glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	if(!geometry.normalBuffer){
		glGenBuffersARB( 1, &geometry.normalBuffer );
		glBindBufferARB( GL_ARRAY_BUFFER_ARB, geometry.normalBuffer );
		glBufferDataARB( GL_ARRAY_BUFFER_ARB, geometry.vertex.size()*3*sizeof(float), 0, GL_STATIC_DRAW_ARB );
		
		float* normalBuf=(float*)glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB);
		if(!normalBuf){
			MessageBox(0,"glMapBuffer failed","Exiting",0);
			exit(0);
		}
		for(vector<ArrayVertex>::iterator vi=va.begin();vi!=va.end();++vi){
			int oldNum=vi->sub.begin()->num;
			for(vector<ArrayVertexSub>::iterator vsi=vi->sub.begin();vsi!=vi->sub.end();++vsi){
				normalBuf[vsi->num*3+0]=geometry.vertexNormal[oldNum].x;	
				normalBuf[vsi->num*3+1]=geometry.vertexNormal[oldNum].y;	
				normalBuf[vsi->num*3+2]=geometry.vertexNormal[oldNum].z;	
			}
		}
		if(!glUnmapBufferARB(GL_ARRAY_BUFFER_ARB)){
			MessageBox(0,"glUnmapBuffer failed","Exiting",0);
			exit(0);
		}
	}
	geometry.numVerteces=geometry.vertex.size();
	geometry.numIndeces=geometry.tri.size()*3;
	glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0);
}

void CUnit3DLoader::SplitArray(vector<float3>& vert, vector<ArrayVertex> va)
{
	vector<float3> newVert;
	int newNum=0;
	for(vector<ArrayVertex>::iterator vi=va.begin();vi!=va.end();++vi)
		for(vector<ArrayVertexSub>::iterator vsi=vi->sub.begin();vsi!=vi->sub.end();++vsi)
			++newNum;
	newVert.resize(newNum);
	for(vector<ArrayVertex>::iterator vi=va.begin();vi!=va.end();++vi){
		int oldNum=vi->sub.begin()->num;
		for(vector<ArrayVertexSub>::iterator vsi=vi->sub.begin();vsi!=vi->sub.end();++vsi){
			newVert[vsi->num]=vert[oldNum];
		}
	}
	vert=newVert;
}

unsigned int CUnit3DLoader::CreateDisplistFromVector(UnitModel& model, vector<float3>& vertex,bool fixTexCoords)
{
	unsigned int displist=glGenLists(1);
	glNewList(displist, GL_COMPILE);

	CVertexArray* va=GetVertexArray();
	if(!model.quadTex.empty()){
		va->Initialize();
		vector<Quad>::iterator qi;
		vector<QuadTex>::iterator qti;
		for(qti=model.quadTex.begin(),qi=model.geometry->quad.begin();qti!=model.quadTex.end();++qti,++qi){
			CTextureHandler::UnitTexture* ut=texturehandler->GetTexture(qti->texName,model.team,qti->teamTex);

			if(qi->normalType==0){
				va->AddVertexTN(model.geometry->vertex[qi->verteces[0]],
					ut->xstart*qti->texPos[0][0]+ut->xend*(1-qti->texPos[0][0]),ut->ystart*qti->texPos[0][1]+ut->yend*(1-qti->texPos[0][1]),
					qi->normal);
				va->AddVertexTN(model.geometry->vertex[qi->verteces[1]],
					ut->xstart*qti->texPos[1][0]+ut->xend*(1-qti->texPos[1][0]),ut->ystart*qti->texPos[1][1]+ut->yend*(1-qti->texPos[1][1]),
					qi->normal);
				va->AddVertexTN(model.geometry->vertex[qi->verteces[2]],
					ut->xstart*qti->texPos[2][0]+ut->xend*(1-qti->texPos[2][0]),ut->ystart*qti->texPos[2][1]+ut->yend*(1-qti->texPos[2][1]),
					qi->normal);
				va->AddVertexTN(model.geometry->vertex[qi->verteces[3]],
					ut->xstart*qti->texPos[3][0]+ut->xend*(1-qti->texPos[3][0]),ut->ystart*qti->texPos[3][1]+ut->yend*(1-qti->texPos[3][1]),
					qi->normal);
			} else {
				va->AddVertexTN(model.geometry->vertex[qi->verteces[0]],
					ut->xstart*qti->texPos[0][0]+ut->xend*(1-qti->texPos[0][0]),ut->ystart*qti->texPos[0][1]+ut->yend*(1-qti->texPos[0][1]),
					model.geometry->vertexNormal[qi->verteces[0]]);
				va->AddVertexTN(model.geometry->vertex[qi->verteces[1]],
					ut->xstart*qti->texPos[1][0]+ut->xend*(1-qti->texPos[1][0]),ut->ystart*qti->texPos[1][1]+ut->yend*(1-qti->texPos[1][1]),
					model.geometry->vertexNormal[qi->verteces[1]]);
				va->AddVertexTN(model.geometry->vertex[qi->verteces[2]],
					ut->xstart*qti->texPos[2][0]+ut->xend*(1-qti->texPos[2][0]),ut->ystart*qti->texPos[2][1]+ut->yend*(1-qti->texPos[2][1]),
					model.geometry->vertexNormal[qi->verteces[2]]);
				va->AddVertexTN(model.geometry->vertex[qi->verteces[3]],
					ut->xstart*qti->texPos[3][0]+ut->xend*(1-qti->texPos[3][0]),ut->ystart*qti->texPos[3][1]+ut->yend*(1-qti->texPos[3][1]),
					model.geometry->vertexNormal[qi->verteces[3]]);
			}
		}
		va->DrawArrayTN(GL_QUADS);
	}
	if(!model.triTex.empty()){
		va->Initialize();
		vector<Tri>::iterator ti;
		vector<TriTex>::iterator tti;
		for(ti=model.geometry->tri.begin(),tti=model.triTex.begin();ti!=model.geometry->tri.end();++ti,++tti){
			if(fixTexCoords){
				CTextureHandler::UnitTexture* ut=texturehandler->GetTexture(tti->texName,model.team,tti->teamTex);
				tti->texPos[0][0]=ut->xstart*tti->texPos[0][0]+ut->xend*(1-tti->texPos[0][0]);
				tti->texPos[0][1]=ut->ystart*tti->texPos[0][1]+ut->yend*(1-tti->texPos[0][1]);
				tti->texPos[1][0]=ut->xstart*tti->texPos[1][0]+ut->xend*(1-tti->texPos[1][0]);
				tti->texPos[1][1]=ut->ystart*tti->texPos[1][1]+ut->yend*(1-tti->texPos[1][1]);
				tti->texPos[2][0]=ut->xstart*tti->texPos[2][0]+ut->xend*(1-tti->texPos[2][0]);
				tti->texPos[2][1]=ut->ystart*tti->texPos[2][1]+ut->yend*(1-tti->texPos[2][1]);
			}
			if(ti->normalType==0){
				va->AddVertexTN(vertex[ti->verteces[0]],
					tti->texPos[0][0],tti->texPos[0][1],
					ti->normal);
				va->AddVertexTN(vertex[ti->verteces[1]],
					tti->texPos[1][0],tti->texPos[1][1],
					ti->normal);
				va->AddVertexTN(vertex[ti->verteces[2]],
					tti->texPos[2][0],tti->texPos[2][1],
					ti->normal);
			} else {
				va->AddVertexTN(vertex[ti->verteces[0]],
					tti->texPos[0][0],tti->texPos[0][1],
					model.geometry->vertexNormal[ti->verteces[0]]);
				va->AddVertexTN(vertex[ti->verteces[1]],
					tti->texPos[1][0],tti->texPos[1][1],
					model.geometry->vertexNormal[ti->verteces[1]]);
				va->AddVertexTN(vertex[ti->verteces[2]],
					tti->texPos[2][0],tti->texPos[2][1],
					model.geometry->vertexNormal[ti->verteces[2]]);
			}
		}
		va->DrawArrayTN(GL_TRIANGLES);
	}
	glEndList();
	return displist;
}
