#include "StdAfx.h"
// TreeDrawer.cpp: implementation of the CBasicTreeDrawer class.
//
//////////////////////////////////////////////////////////////////////

#include "BasicTreeDrawer.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "TdfParser.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBasicTreeDrawer::CBasicTreeDrawer()
{
	lastListClean=0;

	TdfParser resources("gamedata/resources.tdf");

	CBitmap TexImage;
	std::string fn("bitmaps/"+resources.SGetValueDef("gran.bmp","resources\\graphics\\trees\\gran1"));
	if (!TexImage.Load(fn))
		throw content_error("Could not load tree texture from " + fn);
	TexImage.ReverseYAxis();
	//unsigned char gran[1024][512][4];
	unsigned char (*gran)[512][4]=SAFE_NEW unsigned char[1024][512][4];
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<256;x++){
				if(TexImage.mem[(y*256+x)*4]==72 && TexImage.mem[(y*256+x)*4+1]==72){
					gran[y][x][0]=33;
					gran[y][x][1]=54;
					gran[y][x][2]=29;
					gran[y][x][3]=0;
				} else {
					gran[y][x][0]=TexImage.mem[(y*256+x)*4];
					gran[y][x][1]=TexImage.mem[(y*256+x)*4+1];
					gran[y][x][2]=TexImage.mem[(y*256+x)*4+2];
					gran[y][x][3]=255;
				}
			}
		}
	}

	fn = "bitmaps/"+resources.SGetValueDef("gran2.bmp","resources\\graphics\\trees\\gran2");
	if (!TexImage.Load(fn))
		throw content_error("Could not load tree texture from file " + fn);
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<256;x++){
				if(TexImage.mem[(y*256+x)*4]==72 && TexImage.mem[(y*256+x)*4+1]==72){
					gran[y][x+256][0]=33;
					gran[y][x+256][1]=54;
					gran[y][x+256][2]=29;
					gran[y][x+256][3]=0;
				} else {
					gran[y][x+256][0]=TexImage.mem[(y*256+x)*4];
					gran[y][x+256][1]=TexImage.mem[(y*256+x)*4+1];
					gran[y][x+256][2]=TexImage.mem[(y*256+x)*4+2];
					gran[y][x+256][3]=255;
				}
			}
		}
	}

	fn = "bitmaps/"+resources.SGetValueDef("birch1.bmp","resources\\graphics\\trees\\birch1");
	if (!TexImage.Load(fn))
		throw content_error("Could not load tree texture from file " + fn);
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<128;x++){
				if(TexImage.mem[(y*128+x)*4]==72 && TexImage.mem[(y*128+x)*4+1]==72){
					gran[y+256][x][0]=(unsigned char)(125*0.6f);
					gran[y+256][x][1]=(unsigned char)(146*0.7f);
					gran[y+256][x][2]=(unsigned char)(82*0.6f);
					gran[y+256][x][3]=(unsigned char)(0);
				} else {
					gran[y+256][x][0]=(unsigned char)(TexImage.mem[(y*128+x)*4]*0.6f);
					gran[y+256][x][1]=(unsigned char)(TexImage.mem[(y*128+x)*4+1]*0.7f);
					gran[y+256][x][2]=(unsigned char)(TexImage.mem[(y*128+x)*4+2]*0.6f);
					gran[y+256][x][3]=255;
				}
			}
		}
	}

	fn = "bitmaps/"+resources.SGetValueDef("birch2.bmp","resources\\graphics\\trees\\birch2");
	if (!TexImage.Load(fn))
		throw content_error("Could not load tree texture from file " + fn);
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<128;x++){
				if(TexImage.mem[(y*128+x)*4]==72 && TexImage.mem[(y*128+x)*4+1]==72){
					gran[y+256][x+128][0]=(unsigned char)(125*0.6f);
					gran[y+256][x+128][1]=(unsigned char)(146*0.7f);
					gran[y+256][x+128][2]=(unsigned char)(82*0.6f);
					gran[y+256][x+128][3]=0;
				} else {
					gran[y+256][x+128][0]=(unsigned char)(TexImage.mem[(y*128+x)*4]*0.6f);
					gran[y+256][x+128][1]=(unsigned char)(TexImage.mem[(y*128+x)*4+1]*0.7f);
					gran[y+256][x+128][2]=(unsigned char)(TexImage.mem[(y*128+x)*4+2]*0.6f);
					gran[y+256][x+128][3]=255;
				}
			}
		}
	}

	fn = "bitmaps/"+resources.SGetValueDef("birch3.bmp","resources\\graphics\\trees\\birch3");
	if (!TexImage.Load(fn))
		throw content_error("Could not load tree texture from file " + fn);
	TexImage.ReverseYAxis();
	if (TexImage.xsize>1){
		for(int y=0;y<256;y++){
			for(int x=0;x<256;x++){
				if(TexImage.mem[(y*256+x)*4]==72 && TexImage.mem[(y*256+x)*4+1]==72){
					gran[y+256][x+256][0]=(unsigned char)(125*0.6f);
					gran[y+256][x+256][1]=(unsigned char)(146*0.7f);
					gran[y+256][x+256][2]=(unsigned char)(82*0.6f);
					gran[y+256][x+256][3]=0;
				} else {
					gran[y+256][x+256][0]=(unsigned char)(TexImage.mem[(y*256+x)*4]*0.6f);
					gran[y+256][x+256][1]=(unsigned char)(TexImage.mem[(y*256+x)*4+1]*0.7f);
					gran[y+256][x+256][2]=(unsigned char)(TexImage.mem[(y*256+x)*4+2]*0.6f);
					gran[y+256][x+256][3]=255;
				}
			}
		}
	}

	// create mipmapped texture
	CreateTreeTex(treetex,gran[0][0],512,1024);
	delete[] gran;

	treesX=gs->mapx/TREE_SQUARE_SIZE;
	treesY=gs->mapy/TREE_SQUARE_SIZE;
	trees=SAFE_NEW TreeSquareStruct[treesX*treesY];

	for(int y=0;y<treesY;y++){
		for(int x=0;x<treesX;x++){
			trees[y*treesX+x].displist=0;
			trees[y*treesX+x].farDisplist=0;
		}
	}
}

CBasicTreeDrawer::~CBasicTreeDrawer()
{
	glDeleteTextures (1, &treetex);

	for(int y=0;y<treesY;y++){
		for(int x=0;x<treesX;x++){
			if(trees[y*treesX+x].displist)
				glDeleteLists(trees[y*treesX+x].displist,1);
			if(trees[y*treesX+x].farDisplist)
				glDeleteLists(trees[y*treesX+x].farDisplist,1);
		}
	}
	delete[] trees;
}

static CVertexArray* va;

static void inline SetArray(float t1,float t2,float3 v)
{
	va->AddVertexT(v,t1,t2);
}

struct CBasicTreeSquareDrawer : CReadMap::IQuadDrawer
{
	CBasicTreeSquareDrawer() {td=0;}
	void DrawQuad (int x,int y);

	CBasicTreeDrawer *td;
	int cx,cy;
	float treeDistance;
};

void CBasicTreeSquareDrawer::DrawQuad (int x,int y)
{
	int treesX = td->treesX;
	CBasicTreeDrawer::TreeSquareStruct* tss=&td->trees[y*treesX+x];

	float3 dif;
	dif.x=camera->pos.x-(x*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	dif.y=0;
	dif.z=camera->pos.z-(y*SQUARE_SIZE*TREE_SQUARE_SIZE + SQUARE_SIZE*TREE_SQUARE_SIZE/2);
	float dist=dif.Length();
	dif/=dist;

	if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2 && dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance)){//far trees
		tss->lastSeenFar=gs->frameNum;
		if(!tss->farDisplist || dif.dot(tss->viewVector)<0.97f){
			va=GetVertexArray();
			va->Initialize();
			tss->viewVector=dif;
			if(!tss->farDisplist)
				tss->farDisplist=glGenLists(1);
			float3 up(0,1,0);
			float3 side=up.cross(dif);

			for(std::map<int,CBasicTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CBasicTreeDrawer::TreeStruct* ts=&ti->second;
				if(ts->type<8){
					float3 base(ts->pos);
					float height=MAX_TREE_HEIGHT;
					float width=MAX_TREE_HEIGHT*0.3f;

					SetArray(0,0,base+side*width);
					SetArray(0,0.25f,base+side*width+float3(0,height,0));
					SetArray(0.5f,0.25f,base-side*width+float3(0,height,0));
					SetArray(0.5f,0,base-side*width);
				} else {
					float3 base(ts->pos);
					float height=MAX_TREE_HEIGHT;
					float width=MAX_TREE_HEIGHT*0.3f;

					SetArray(0,0.25f,base+side*width);
					SetArray(0,0.5f,base+side*width+float3(0,height,0));
					SetArray(0.25f,0.5f,base-side*width+float3(0,height,0));
					SetArray(0.25f,0.25f,base-side*width);
				}
			}
			glNewList(td->trees[y*treesX+x].farDisplist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		if(dist>SQUARE_SIZE*TREE_SQUARE_SIZE*(treeDistance*2-1)){
			float trans=(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE);
			glEnable(GL_BLEND);
			glColor4f(1,1,1,trans);
			glAlphaFunc(GL_GREATER,(SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance*2-dist)/(SQUARE_SIZE*TREE_SQUARE_SIZE*2));
		} else {
			glColor4f(1,1,1,1);
			glDisable(GL_BLEND);
			glAlphaFunc(GL_GREATER,0.5f);
		}
		glCallList(tss->farDisplist);
	}

	if(dist<SQUARE_SIZE*TREE_SQUARE_SIZE*treeDistance){	//middle distance trees
		tss->lastSeen=gs->frameNum;
		if(!tss->displist){
			va=GetVertexArray();
			va->Initialize();
			tss->displist=glGenLists(1);

			for(std::map<int,CBasicTreeDrawer::TreeStruct>::iterator ti=tss->trees.begin();ti!=tss->trees.end();++ti){
				CBasicTreeDrawer::TreeStruct* ts=&ti->second;
				if(ts->type<8){
					float3 base(ts->pos);
					float height=MAX_TREE_HEIGHT;
					float width=MAX_TREE_HEIGHT*0.3f;

					SetArray(0,0,base+float3(width,0,0));
					SetArray(0,0.25f,base+float3(width,height,0));
					SetArray(0.5f,0.25f,base+float3(-width,height,0));
					SetArray(0.5f,0,base+float3(-width,0,0));

					SetArray(0,0,base+float3(0,0,width));
					SetArray(0,0.25f,base+float3(0,height,width));
					SetArray(0.5f,0.25f,base+float3(0,height,-width));
					SetArray(0.5f,0,base+float3(0,0,-width));

					width*=1.2f;
					SetArray(0.5f,0,base+float3(width,height*0.25f,0));
					SetArray(0.5f,0.25f,base+float3(0,height*0.25f,-width));
					SetArray(1,0.25f,base+float3(-width,height*0.25f,0));
					SetArray(1,0,base+float3(0,height*0.25f,width));
				} else {
					float3 base(ts->pos);
					float height=MAX_TREE_HEIGHT;
					float width=MAX_TREE_HEIGHT*0.3f;

					SetArray(0,0.25f,base+float3(width,0,0));
					SetArray(0,0.5f,base+float3(width,height,0));
					SetArray(0.25f,0.5f,base+float3(-width,height,0));
					SetArray(0.25f,0.25f,base+float3(-width,0,0));

					SetArray(0.25f,0.25f,base+float3(0,0,width));
					SetArray(0.25f,0.5f,base+float3(0,height,width));
					SetArray(0.5f,0.5f,base+float3(0,height,-width));
					SetArray(0.5f,0.25f,base+float3(0,0,-width));

					width*=1.2f;
					SetArray(0.5f,0.25f,base+float3(width,height*0.3f,0));
					SetArray(0.5f,0.5f,base+float3(0,height*0.3f,-width));
					SetArray(1,0.5f,base+float3(-width,height*0.3f,0));
					SetArray(1,0.25f,base+float3(0,height*0.3f,width));
				}
			}
			glNewList(tss->displist,GL_COMPILE);
			va->DrawArrayT(GL_QUADS);
			glEndList();
		}
		glColor4f(1,1,1,1);
		glDisable(GL_BLEND);
		glAlphaFunc(GL_GREATER,0.5f);
		glCallList(tss->displist);
	}
}

void CBasicTreeDrawer::Draw(float treeDistance,bool drawReflection)
{
	glBindTexture(GL_TEXTURE_2D, treetex);
	glEnable(GL_ALPHA_TEST);

	int cx=(int)(camera->pos.x/(SQUARE_SIZE*TREE_SQUARE_SIZE));
	int cy=(int)(camera->pos.z/(SQUARE_SIZE*TREE_SQUARE_SIZE));

	CBasicTreeSquareDrawer drawer;
	drawer.td = this;
	drawer.cx = cx;
	drawer.cy = cy;
	drawer.treeDistance = treeDistance;

	readmap->GridVisibility (camera, TREE_SQUARE_SIZE, treeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE, &drawer);

	int startClean=lastListClean*20%(treesX*treesY);
	lastListClean=gs->frameNum;
	int endClean=gs->frameNum*20%(treesX*treesY);

	if(startClean>endClean){
		for(int a=startClean;a<treesX*treesY;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
		for(int a=0;a<endClean;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
	} else {
		for(int a=startClean;a<endClean;a++){
			if(trees[a].lastSeen<gs->frameNum-50 && trees[a].displist){
				glDeleteLists(trees[a].displist,1);
				trees[a].displist=0;
			}
			if(trees[a].lastSeenFar<gs->frameNum-50 && trees[a].farDisplist){
				glDeleteLists(trees[a].farDisplist,1);
				trees[a].farDisplist=0;
			}
		}
	}
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}

void CBasicTreeDrawer::Update()
{

}

void CBasicTreeDrawer::CreateTreeTex(GLuint& texnum, unsigned char *data, int xsize, int ysize)
{
	glGenTextures(1, &texnum);
	glBindTexture(GL_TEXTURE_2D, texnum);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void CBasicTreeDrawer::ResetPos(const float3& pos)
{
	int x=(int)(pos.x/TREE_SQUARE_SIZE/SQUARE_SIZE);
	int y=(int)(pos.z/TREE_SQUARE_SIZE/SQUARE_SIZE);
	int a=y*treesX+x;
	if(trees[a].displist){
		glDeleteLists(trees[a].displist,1);
		trees[a].displist=0;
	}
	if(trees[a].farDisplist){
		glDeleteLists(trees[a].farDisplist,1);
		trees[a].farDisplist=0;
	}
}

void CBasicTreeDrawer::AddTree(int type, float3 pos, float size)
{
	TreeStruct ts;
	ts.pos=pos;
	ts.type=type;
	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;
	trees[square].trees[hash]=ts;
	ResetPos(pos);
}

void CBasicTreeDrawer::DeleteTree(float3 pos)
{
	int hash=(int)pos.x+((int)(pos.z))*20000;
	int square=((int)pos.x)/(SQUARE_SIZE*TREE_SQUARE_SIZE)+((int)pos.z)/(SQUARE_SIZE*TREE_SQUARE_SIZE)*treesX;

	trees[square].trees.erase(hash);

	ResetPos(pos);
}
