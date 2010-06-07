/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include <cstring>

#include "AdvTreeGenerator.h"

#include "Game/Camera.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"

using std::max;
using std::min;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAdvTreeGenerator* treeGen;

CAdvTreeGenerator::CAdvTreeGenerator()
{
	PrintLoadMsg("Generating 3D trees");

	unsigned char(* tree)[2048][4] = new unsigned char[256][2048][4];
	memset(tree[0][0], 128, 256 * 2048 * 4);

	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		logOutput.Print(resourcesParser.GetErrorLog());
	}

	const LuaTable treesTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("trees");

	CBitmap bm;
	std::string fn("bitmaps/" + treesTable.GetString("bark", "Bark.bmp"));
	if (!bm.Load(fn) || bm.xsize != 256 || bm.ysize != 256)
		throw content_error("Could not load tree texture from file " + fn);

	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			tree[y][x][0] = bm.mem[(y * 256 + x) * 4    ];
			tree[y][x][1] = bm.mem[(y * 256 + x) * 4 + 1];
			tree[y][x][2] = bm.mem[(y * 256 + x) * 4 + 2];
			tree[y][x][3] = 255;
			tree[y][x + 1024][0] = (unsigned char)(bm.mem[(y * 256 + x) * 4    ] * 0.6f);
			tree[y][x + 1024][1] = (unsigned char)(bm.mem[(y * 256 + x) * 4 + 1] * 0.6f);
			tree[y][x + 1024][2] = (unsigned char)(bm.mem[(y * 256 + x) * 4 + 2] * 0.6f);
			tree[y][x + 1024][3] = 255;
		}
	}

	fn = "bitmaps/" + treesTable.GetString("leaf", "bleaf.bmp");
	if (!bm.Load(fn))
		throw content_error("Could not load tree texture from file " + fn);

	bm.CreateAlpha(0, 0, 0);
	bm.Renormalize(float3(0.22f, 0.43f, 0.18f));

	GLuint leafTex;
	glGenTextures(1, &leafTex);
	glBindTexture(GL_TEXTURE_2D, leafTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);

	CreateLeafTex(leafTex, 256, 0, tree);
	CreateLeafTex(leafTex, 512, 0, tree);
	CreateLeafTex(leafTex, 768, 0, tree);

	glDeleteTextures (1, &leafTex);

	unsigned char* data = tree[0][0];
	CreateGranTex(data, 1024 + 768, 0, 2048);
	CreateGranTex(data, 1280,       0, 2048);
	CreateGranTex(data, 1536,       0, 2048);

	glGenTextures(1, &barkTex);
	CreateTex(data, barkTex, 2048, 256, false, 10);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

	delete[] tree;


	leafDL = glGenLists(8);
	srand(15U);
	for (int a = 0; a < 8; ++a) {
		va = GetVertexArray();
		va->Initialize();
		barkva = GetVertexArray();
		barkva->Initialize();

		glNewList(leafDL + a, GL_COMPILE);
		float size = 0.65f + fRand(0.2f);
		MainTrunk(10, size * MAX_TREE_HEIGHT, size * 0.05f * MAX_TREE_HEIGHT);
		va->DrawArrayTN(GL_QUADS);
		barkva->DrawArrayTN(GL_TRIANGLE_STRIP);

		glEndList();
	}

	pineDL = glGenLists(8);
	srand(15U);
	for (int a = 0; a < 8; ++a) {
		va = GetVertexArray();
		va->Initialize();

		glNewList(pineDL + a, GL_COMPILE);
		float size = 0.7f + fRand(0.2f);
		PineTree((int)(20 + fRand(10)), MAX_TREE_HEIGHT * size);
		va->DrawArrayTN(GL_TRIANGLES);

		glEndList();
	}
}

CAdvTreeGenerator::~CAdvTreeGenerator()
{
	glDeleteTextures(1, &barkTex);
	glDeleteTextures(2, farTex);
	glDeleteLists(leafDL, 8);
	glDeleteLists(pineDL, 8);
}

void CAdvTreeGenerator::Draw()
{
}

void CAdvTreeGenerator::DrawTrunk(const float3& start, const float3& end, const float3& orto1, const float3& orto2, float size)
{
	float3 flatSun = mapInfo->light.sunDir;
	flatSun.y = 0.0f;

	int numIter=(int)max(3.0f,size*10);
	for(int a=0;a<=numIter;a++){
		float angle=a/(float)numIter*2*PI;
		float col=0.4f+(((orto1*sin(angle)+orto2*cos(angle)).dot(flatSun)))*0.3f;
		barkva->AddVertexTN(start+orto1*sin(angle)*size+orto2*cos(angle)*size,angle/PI*0.125f*0.5f,0,float3(0,0,col));
		barkva->AddVertexTN(end+orto1*sin(angle)*size*0.2f+orto2*cos(angle)*size*0.2f,angle/PI*0.125f*0.5f,3,float3(0,0,col));
	}
	barkva->EndStrip();
}

void CAdvTreeGenerator::MainTrunk(int numBranch,float height,float width)
{
	const float3 orto1(1, 0, 0);
	const float3 orto2(0, 0, 1);
	const float baseAngle = fRand(2 * PI);

	DrawTrunk(ZeroVector, float3(0, height, 0), orto1, orto2, width);

	for (int a = 0; a < numBranch; ++a) {
		const float angle = baseAngle + (a * 3.88f) + fRand(0.5f);
		float3 dir = orto1 * sin(angle) + orto2 * cos(angle);
			dir.y = 0.3f + fRand(0.4f);
			dir.ANormalize();
		const float3 start(0, (a + 5) * height / (numBranch + 5), 0);
		const float length = (height * (0.4f + fRand(0.1f))) * sqrt(float(numBranch - a) / numBranch);

		TrunkIterator(start, dir, length, length * 0.05f, 1);
	}

	for (int a = 0; a < 3; ++a) {
		const float angle = (a * 3.88f) + fRand(0.5f);
		float3 dir = orto1*sin(angle)+orto2*cos(angle);
			dir.y = 0.8f;
			dir.ANormalize();
		const float3 start(0, height - 0.3f, 0);
		const float length = MAX_TREE_HEIGHT * 0.1f;

		TrunkIterator(start, dir, length, length * 0.05f, 0);
	}
}

void CAdvTreeGenerator::TrunkIterator(const float3& start, const float3& dir, float length, float size, int depth)
{
	float3 orto1;
	if (dir.dot(UpVector) < 0.9f)
		orto1 = dir.cross(UpVector);
	else
		orto1 = dir.cross(float3(1, 0, 0));
	orto1.ANormalize();

	float3 orto2 = dir.cross(orto1);
	orto2.ANormalize();

	DrawTrunk(start, start + dir * length, orto1, orto2, size);

	if (depth <= 1)
		CreateLeaves(start, dir, length, orto1, orto2);

	if (depth == 0)
		return;

	const float dirDif = fRand(0.8f) + 1.0f;
	const int numTrunks = (int) length * 5 / MAX_TREE_HEIGHT;

	for (int a = 0; a < numTrunks; a++) {
		const float angle = PI + float(a) * PI + fRand(0.3f);
		const float newLength = length * (float(numTrunks - a) / (numTrunks + 1));

		float3 newbase = start + dir * length * (float(a + 1) / (numTrunks + 1));
		float3 newDir = dir + orto1 * cos(angle) * dirDif + orto2 * sin(angle) * dirDif;
		newDir.ANormalize();

		TrunkIterator(newbase, newDir, newLength, newLength * 0.05f, depth - 1);
	}
}

void CAdvTreeGenerator::CreateLeaves(const float3& start, const float3& dir, float length, float3& orto1,float3& orto2)
{
	const float baseRot = fRand(2 * PI);
	const int numLeaves = (int) length * 10 / MAX_TREE_HEIGHT;

	float3 flatSun = mapInfo->light.sunDir;
	flatSun.y = 0;

	for (int a = 0; a < numLeaves + 1; a++) {
		const float angle = baseRot + a * 0.618f * 2 * PI;

		float3 pos = start + dir * length * (0.7f + fRand(0.3f));
		pos +=
			(orto1 * sin(angle) + orto2 * cos(angle)) *
			(sqrt((float) a + 1) * 0.6f + fRand(0.4f)) *
			0.1f * MAX_TREE_HEIGHT;

		if (pos.y < 0.2f * MAX_TREE_HEIGHT)
			pos.y = 0.2f * MAX_TREE_HEIGHT;

		float3 npos = pos;
		npos.y = 0;
		npos.ANormalize();

		const float tex = float(int(rand() * 3 / RAND_MAX)) * 0.125f;
		const float flipTex = float(int(rand()*2/RAND_MAX)) * 0.123f;
		const float col = 0.5f + (npos.dot(flatSun) * 0.3f) + fRand(0.1f);

		va->AddVertexTN(pos, 0.126f + tex + flipTex, 0.98f, float3( 0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col));
		va->AddVertexTN(pos, 0.249f + tex - flipTex, 0.98f, float3(-0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col));
		va->AddVertexTN(pos, 0.249f + tex - flipTex, 0.02f, float3(-0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col));
		va->AddVertexTN(pos, 0.126f + tex + flipTex, 0.02f, float3( 0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col));
	}

	float3 pos = start + dir * length * 1.03f;
	float3 npos = pos;
	npos.y = 0;
	npos.ANormalize();

	const float tex = float(int(rand() * 3 / RAND_MAX)) * 0.125f;
	const float flipTex = float(int(rand() * 2 / RAND_MAX)) * 0.123f;
	const float col = 0.5f + (npos.dot(flatSun) * 0.3f) + fRand(0.1f);

	va->AddVertexTN(pos, 0.126f + tex + flipTex, 0.98f, float3( 0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col));
	va->AddVertexTN(pos, 0.249f + tex - flipTex, 0.98f, float3(-0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col));
	va->AddVertexTN(pos, 0.249f + tex - flipTex, 0.02f, float3(-0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col));
	va->AddVertexTN(pos, 0.126f + tex + flipTex, 0.02f, float3( 0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col));
}

void CAdvTreeGenerator::CreateFarTex(Shader::IProgramObject* treeShader)
{
	unsigned char* data = new unsigned char[512 * 512 * 4];
	unsigned char* data2 = new unsigned char[512 * 512 * 4];

	for (int y = 0; y < 512; ++y) {
		for (int x = 0; x < 512; ++x) {
			data[((y) * 512 + x) * 4 + 0] = 60;
			data[((y) * 512 + x) * 4 + 1] = 90;
			data[((y) * 512 + x) * 4 + 2] = 40;
			data[((y) * 512 + x) * 4 + 3] =  0;
		}
	}

	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, barkTex);
	glEnable(GL_TEXTURE_2D);

	treeShader->Enable();

	{
		#define L mapInfo->light
		if (globalRendering->haveGLSL) {
			treeShader->SetUniform3f(0, 1.0f, 0.0f, 0.0f);
			treeShader->SetUniform3f(1, 0.0f, 1.0f, 0.0f);
			treeShader->SetUniform3f(2, 0.0f, 0.0f, 0.0f);
			treeShader->SetUniform2f(5, 0.02f, 0.85f);
		} else {
			treeShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
			treeShader->SetUniform3f(13, 1.0f, 0.0f, 0.0f); // camera side-dir
			treeShader->SetUniform3f( 9, 0.0f, 1.0f, 0.0f); // camera up-dir
			treeShader->SetUniform3f(10, 0.0f, 0.0f, 0.0f); // tree position-offset
			treeShader->SetUniform4f(11, L.groundSunColor.x,     L.groundSunColor.y,     L.groundSunColor.z,     0.85f);
			treeShader->SetUniform4f(14, L.groundAmbientColor.x, L.groundAmbientColor.y, L.groundAmbientColor.z, 0.85f);
			treeShader->SetUniform4f(12, 0.0f, 0.0f, 0.0f, 0.02f); // w = alpha / height modifier
		}
		#undef L

		glAlphaFunc(GL_GREATER, 0.5f);
		glDisable(GL_FOG);
		glDisable(GL_BLEND);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glViewport(0, 0, 64, 64);
		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);

		for (int a = 0; a < 8; ++a) {
			treeShader->SetUniform3f(((globalRendering->haveGLSL)? 0: 13), 1.0f, 0.0f, 0.0f);
			treeShader->SetUniform3f(((globalRendering->haveGLSL)? 1:  9), 0.0f, 1.0f, 0.0f);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glScalef(-1.0f, 1.0f, 1.0f);
			glOrtho(-MAX_TREE_HEIGHT * 0.5f, MAX_TREE_HEIGHT * 0.5f, 0, MAX_TREE_HEIGHT, -MAX_TREE_HEIGHT * 0.5f, MAX_TREE_HEIGHT * 0.5f);

			CreateFarView(data, a * 64,   0, leafDL + a);
			CreateFarView(data, a * 64, 256, pineDL + a);
			glScalef(-1.0f, 1.0f, 1.0f);


			treeShader->SetUniform3f(((globalRendering->haveGLSL)? 0: 13), 0.0f, 0.0f, 1.0f);

			glMatrixMode(GL_MODELVIEW);
			glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
			CreateFarView(data, a * 64, 64,       leafDL + a);
			CreateFarView(data, a * 64, 64 + 256, pineDL + a);


			treeShader->SetUniform3f(((globalRendering->haveGLSL)? 0: 13), -1.0f, 0.0f, 0.0f);

			glMatrixMode(GL_MODELVIEW);
			glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
			CreateFarView(data2, a * 64,   0, leafDL + a);
			CreateFarView(data2, a * 64, 256, pineDL + a);


			treeShader->SetUniform3f(((globalRendering->haveGLSL)? 0: 13), 0.0f, 0.0f, 1.0f);
			treeShader->SetUniform3f(((globalRendering->haveGLSL)? 1:  9), 1.0f, 0.0f, 0.0f);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(-MAX_TREE_HEIGHT * 0.5f, MAX_TREE_HEIGHT * 0.5f, -MAX_TREE_HEIGHT * 0.5f, MAX_TREE_HEIGHT * 0.5f, -MAX_TREE_HEIGHT, MAX_TREE_HEIGHT);

			CreateFarView(data, a * 64, 128,       leafDL + a);
			CreateFarView(data, a * 64, 128 + 256, pineDL + a);
		}

		glDisable(GL_ALPHA_TEST);
	}

	treeShader->Disable();


	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	memcpy(&data[512 * (192      ) * 4], &data[512 * (64      ) * 4], 512 * 64 * 4);  // far-away trees
	memcpy(&data[512 * (192 + 256) * 4], &data[512 * (64 + 256) * 4], 512 * 64 * 4);  // far-away pine trees

	memcpy(&data2[512 * (64      ) * 4], &data[512 * (64      ) * 4], 512 * 192 * 4);
	memcpy(&data2[512 * (64 + 256) * 4], &data[512 * (64 + 256) * 4], 512 * 192 * 4);

	glGenTextures(2, farTex);

	CreateTex(data,  farTex[0], 512, 512, true, 4);
	CreateTex(data2, farTex[1], 512, 512, true, 4);

	delete[] data;
	delete[] data2;
}

void CAdvTreeGenerator::CreateFarView(unsigned char* mem,int dx,int dy,unsigned int displist)
{
	unsigned char* buf=new unsigned char[64*64*4];
	glClearColor(0.0f,0.0f,0.0f,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCallList(displist);

	glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE,buf);

	for(int y=0;y<64;++y){
		for(int x=0;x<64;++x){
			if(buf[(y*64+x)*4]==0 && buf[(y*64+x)*4+1]==0 && buf[(y*64+x)*4+2]==0){
				mem[((y+dy)*512+x+dx)*4+0]=60;
				mem[((y+dy)*512+x+dx)*4+1]=90;
				mem[((y+dy)*512+x+dx)*4+2]=40;
				mem[((y+dy)*512+x+dx)*4+3]=0;
			} else {
				mem[((y+dy)*512+x+dx)*4+0]=buf[(y*64+x)*4];
				mem[((y+dy)*512+x+dx)*4+1]=buf[(y*64+x)*4+1];
				mem[((y+dy)*512+x+dx)*4+2]=buf[(y*64+x)*4+2];
				mem[((y+dy)*512+x+dx)*4+3]=255;
			}
		}
	}

	delete[] buf;
}

void CAdvTreeGenerator::FixAlpha(unsigned char* data)
{
	for(int y=0;y<63;++y){
		for(int x=0;x<512;++x){
			if(data[((y)*512+x)*4+3]==0){
				data[((y)*512+x)*4+3]=y*2;
			} else {
				data[((y)*512+x)*4+3]=255-y*2;
			}
		}
	}
	for(int y=0;y<63;++y){
		for(int x=0;x<512;++x){
			if(data[((y+64)*512+x)*4+3]==0){
				data[((y+64)*512+x)*4+3]=y*2;
			} else {
				data[((y+64)*512+x)*4+3]=255-y*2;
			}
		}
	}
	for(int y=0;y<63;++y){
		for(int x=0;x<512;++x){
			if(data[((y+128)*512+x)*4+3]==0){
				data[((y+128)*512+x)*4+3]=y*2;
			} else {
				data[((y+128)*512+x)*4+3]=255-y*2;
			}
		}
	}
}

void CAdvTreeGenerator::FixAlpha2(unsigned char* data)
{
	for(int y=0;y<31;++y){
		for(int x=0;x<256;++x){
			if(data[((y)*256+x)*4+3]==0){
				data[((y)*256+x)*4+3]=y*4;
			} else {
				data[((y)*256+x)*4+3]=255-y*4;
			}
		}
	}
	for(int y=0;y<31;++y){
		for(int x=0;x<256;++x){
			if(data[((y+32)*256+x)*4+3]==0){
				data[((y+32)*256+x)*4+3]=y*4;
			} else {
				data[((y+32)*256+x)*4+3]=255-y*4;
			}
		}
	}
	for(int y=0;y<31;++y){
		for(int x=0;x<256;++x){
			if(data[((y+64)*256+x)*4+3]==0){
				data[((y+64)*256+x)*4+3]=y*4;
			} else {
				data[((y+64)*256+x)*4+3]=255-y*4;
			}
		}
	}
}

void CAdvTreeGenerator::CreateTex(unsigned char* data, unsigned int tex,int xsize,int ysize,bool fixAlpha,int maxMipLevel)
{
	unsigned char* mipmaps[12];
	mipmaps[0]=data;
	int mipnum=0;
	int xs=xsize;
	int ys=ysize;
	while(xsize!=1 || ysize!=1){
		if(xsize!=1)
			xsize/=2;
		if(ysize!=1)
			ysize/=2;
		mipnum++;
		mipmaps[mipnum]=new unsigned char[max(2,xsize)*max(2,ysize)*4];
		memset(mipmaps[mipnum],0,max(2,xsize)*max(2,ysize)*4);
		for(int y=0;y<ysize;++y){
			for(int x=0;x<xsize;++x){
				int r=0,g=0,b=0,a=0;
				for(int y2=0;y2<2;++y2){
					for(int x2=0;x2<2;++x2){
						int alpha=mipmaps[mipnum-1][((y*2+y2)*xsize*2+x*2+x2)*4+3];
						r+=mipmaps[mipnum-1][((y*2+y2)*xsize*2+x*2+x2)*4+0]*alpha;
						g+=mipmaps[mipnum-1][((y*2+y2)*xsize*2+x*2+x2)*4+1]*alpha;
						b+=mipmaps[mipnum-1][((y*2+y2)*xsize*2+x*2+x2)*4+2]*alpha;
						a+=alpha;
					}
				}
				if(a!=0){
					mipmaps[mipnum][((y)*xsize+x)*4+0]=(unsigned char)( (r+a*0.5f)/a );
					mipmaps[mipnum][((y)*xsize+x)*4+1]=(unsigned char)( (g+a*0.5f)/a );
					mipmaps[mipnum][((y)*xsize+x)*4+2]=(unsigned char)( (b+a*0.5f)/a );
				} else {
					mipmaps[mipnum][((y)*xsize+x)*4+0]=40;
					mipmaps[mipnum][((y)*xsize+x)*4+1]=80;
					mipmaps[mipnum][((y)*xsize+x)*4+2]=20;
				}
				mipmaps[mipnum][((y)*xsize+x)*4+3]=(unsigned char)( (a+2.0f)/4 );
			}
		}
	}
	xsize=xs*2;
	ysize=ys*2;
	mipnum=-1;
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);

	while(xsize!=1 || ysize!=1){
		if(xsize!=1)
			xsize/=2;
		if(ysize!=1)
			ysize/=2;
		mipnum++;
		if(mipnum>0){
			for(int y=0;y<ysize;++y){
				int cumError=0;
				for(int x=0;x<xsize;++x){
					if(mipmaps[mipnum][(y*xsize+x)*4+3]+cumError>126){
						cumError+=int(mipmaps[mipnum][(y*xsize+x)*4+3])-255;
						mipmaps[mipnum][(y*xsize+x)*4+3]=255;
					} else {
						cumError+=mipmaps[mipnum][(y*xsize+x)*4+3];
						mipmaps[mipnum][(y*xsize+x)*4+3]=0;
						for(int c=0;c<3;++c){
							int alpha=0,col=0;
							for(int y2=max(0,y-1);y2<min(ysize,y+2);++y2){
								for(int x2=max(0,x-1);x2<min(xsize,x+2);++x2){
									if(abs(x2-x)+abs(y2-y)==1){
										if(mipmaps[mipnum][(y2*xsize+x2)*4+3]>126){
											col+=mipmaps[mipnum][(y2*xsize+x2)*4+c];
											alpha+=1;
										}
									}
								}
							}
							if(alpha!=0)
								mipmaps[mipnum][(y*xsize+x)*4+c]=(unsigned char)( (col+alpha*0.5f)/alpha ); // !
						}
					}
				}
			}
		}
		if(mipnum>maxMipLevel){
			for(int y=0;y<ysize;++y){
				for(int x=0;x<xsize;++x){
					mipmaps[mipnum][(y*xsize+x)*4+3]=0;
				}
			}
		}
		if(fixAlpha && mipnum==0)
			FixAlpha(mipmaps[0]);
		if(fixAlpha && mipnum==1)
			FixAlpha2(mipmaps[1]);

		glTexImage2D(GL_TEXTURE_2D,mipnum,GL_RGBA8 ,xsize, ysize,0, GL_RGBA, GL_UNSIGNED_BYTE, mipmaps[mipnum]);
		if(mipnum!=0)
			delete[] mipmaps[mipnum];
	}
}

void CAdvTreeGenerator::CreateGranTex(unsigned char* data, int xpos, int ypos, int xsize)
{
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glOrtho(-1,1,-1,1,-4,4);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glColor4f(1,1,1,1);
	glViewport(0,0,256,256);
	glAlphaFunc(GL_GREATER,0.5f);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f,0.0f,0.0f,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CreateGranTexBranch(ZeroVector,float3(0.93f,0.93f,0));

	unsigned char* buf=new unsigned char[256*256*4];
	glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,buf);

	for(int y=0;y<256;++y){
		for(int x=0;x<256;++x){
			if(buf[(y*256+x)*4]==0 && buf[(y*256+x)*4+1]==0 && buf[(y*256+x)*4+2]==0){
				data[((y+ypos)*xsize+x+xpos)*4+0]=60;
				data[((y+ypos)*xsize+x+xpos)*4+1]=90;
				data[((y+ypos)*xsize+x+xpos)*4+2]=40;
				data[((y+ypos)*xsize+x+xpos)*4+3]=0;
			} else {
				data[((y+ypos)*xsize+x+xpos)*4+0]=buf[(y*256+x)*4];
				data[((y+ypos)*xsize+x+xpos)*4+1]=buf[(y*256+x)*4+1];
				data[((y+ypos)*xsize+x+xpos)*4+2]=buf[(y*256+x)*4+2];
				data[((y+ypos)*xsize+x+xpos)*4+3]=255;
			}
		}
	}

	delete[] buf;
	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void CAdvTreeGenerator::CreateGranTexBranch(const float3& start, const float3& end)
{
	float3 dir=end-start;
	float length=dir.Length();
	dir.ANormalize();
	float3 orto=dir.cross(float3(0,0,1));

	glBegin(GL_QUADS);
		glColor3f(0.05f+fRand(0.02f),0.21f+fRand(0.05f),0.04f+fRand(0.01f));
		glVertexf3(start+dir*0.006f+orto*0.007f);
		glVertexf3(start+dir*0.006f-orto*0.007f);
		glVertexf3(end-orto*0.007f);
		glVertexf3(end+orto*0.007f);

		glColor3f(0.18f,0.18f,0.07f);
		glVertexf3(start+orto*length*0.01f);
		glVertexf3(start-orto*length*0.01f);
		glVertexf3(end-orto*0.001f);
		glVertexf3(end+orto*0.001f);
	glEnd();

	float tipDist=0.025f;
	float delta=0.013f;
	int side=(rand()&1)*2-1;
	while(tipDist<length*(0.83f+fRand(0.15f))){
		float3 bstart=start+dir*(length-tipDist);
		float3 bdir=dir+orto*side*(1.0f+fRand(0.7f));
		bdir.ANormalize();
		float3 bend=bstart+bdir*tipDist*((6-tipDist)*(0.10f+fRand(0.05f)));
		CreateGranTexBranch(bstart,bend);
		side*=-1;
		tipDist+=delta;
		delta+=0.005f;
	}
}

void CAdvTreeGenerator::PineTree(int numBranch, float height)
{
	DrawPineTrunk(ZeroVector,float3(0,height,0),height*0.025f);
	float3 orto1(1,0,0);
	float3 orto2(0,0,1);
	float baseAngle=fRand(2*PI);
	for(int a=0;a<numBranch;++a){
		float sh=0.2f+fRand(0.2f);
		float h=pow(sh+float(a)/numBranch*(1-sh),(float)0.7f)*height;
		float angle=baseAngle+(a*0.618f+fRand(0.1f))*2*PI;
		float3 dir(orto1*sin(angle)+orto2*cos(angle));
		dir.y=(a-numBranch)*0.01f-fRand(0.2f)-0.2f;
		dir.ANormalize();
		float size=sqrt((float)numBranch-a+5)*0.08f*MAX_TREE_HEIGHT;
		DrawPineBranch(float3(0,h,0),dir,size);
	}
	//create the top
	float col=0.55f+fRand(0.2f);
	va->AddVertexTN(float3(0,height-0.09f*MAX_TREE_HEIGHT,0), 0.126f+0.5f,0.02f,float3(0,0,col));
	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.249f+0.5f,0.02f,float3(0.05f*MAX_TREE_HEIGHT,0,col));
	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.126f+0.5f,0.98f,float3(-0.05f*MAX_TREE_HEIGHT,0,col));

	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.249f+0.5f,0.02f,float3(0.05f*MAX_TREE_HEIGHT,0,col));
	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.126f+0.5f,0.98f,float3(-0.05f*MAX_TREE_HEIGHT,0,col));
	va->AddVertexTN(float3(0,height+0.03f*MAX_TREE_HEIGHT,0), 0.249f+0.5f,0.98f,float3(0,0,col));
}

void CAdvTreeGenerator::DrawPineTrunk(const float3 &start, const float3 &end, float size)
{
	float3 orto1(1,0,0);
	float3 orto2(0,0,1);
	float3 flatSun=mapInfo->light.sunDir;
	flatSun.y=0;

	int numIter=8;
	for(int a=0;a<numIter;a++){
		float angle=a/(float)numIter*2*PI;
		float angle2=(a+1)/(float)numIter*2*PI;
		float col=0.45f+(((orto1*sin(angle)+orto2*cos(angle)).dot(flatSun)))*0.3f;
		float col2=0.45f+(((orto1*sin(angle2)+orto2*cos(angle2)).dot(flatSun)))*0.3f;

		va->AddVertexTN(start+orto1*sin(angle)*size+orto2*cos(angle)*size, angle/PI*0.125f*0.5f+0.5f,0,float3(0,0,col));
		va->AddVertexTN(end+orto1*sin(angle)*size*0.1f+orto2*cos(angle)*size*0.1f, angle/PI*0.125f*0.5f+0.5f,3,float3(0,0,col));
		va->AddVertexTN(start+orto1*sin(angle2)*size+orto2*cos(angle2)*size, angle2/PI*0.125f*0.5f+0.5f,0,float3(0,0,col2));

		va->AddVertexTN(start+orto1*sin(angle2)*size+orto2*cos(angle2)*size, angle2/PI*0.125f*0.5f+0.5f,0,float3(0,0,col2));
		va->AddVertexTN(end+orto1*sin(angle)*size*0.1f+orto2*cos(angle)*size*0.1f, angle/PI*0.125f*0.5f+0.5f,3,float3(0,0,col));
		va->AddVertexTN(end+orto1*sin(angle2)*size*0.1f+orto2*cos(angle2)*size*0.1f, angle2/PI*0.125f*0.5f+0.5f,3,float3(0,0,col2));
	}
}

void CAdvTreeGenerator::DrawPineBranch(const float3 &start, const float3 &dir, float size)
{
	float3 flatSun=mapInfo->light.sunDir;
	flatSun.y=0;

	float3 orto1=dir.cross(UpVector);
	orto1.ANormalize();
	float3 orto2=dir.cross(orto1);

	float tex=float(int(rand()*3.0f/(float)RAND_MAX))*0.125f;
	float baseCol=0.4f+dir.dot(flatSun)*0.3f+fRand(0.1f);

	float col1=baseCol+fRand(0.2f);
	float col2=baseCol+fRand(0.2f);
	float col3=baseCol+fRand(0.2f);
	float col4=baseCol+fRand(0.2f);
	float col5=baseCol+fRand(0.2f);

	va->AddVertexTN(start, 0.126f+tex+0.5f,0.02f,float3(0,0,col1));
	va->AddVertexTN(start+dir*size*0.5f+orto1*size*0.5f+orto2*size*0.2f, 0.249f+tex+0.5f,0.02f,float3(0,0,col2));
	va->AddVertexTN(start+dir*size*0.5f, 0.1875f+tex+0.5f,0.50f,float3(0,0,col4));

	va->AddVertexTN(start, 0.126f+tex+0.5f,0.02f,float3(0,0,col1));
	va->AddVertexTN(start+dir*size*0.5f-orto1*size*0.5f+orto2*size*0.2f, 0.126f+tex+0.5f,0.98f,float3(0,0,col3));
	va->AddVertexTN(start+dir*size*0.5f, 0.1875f+tex+0.5f,0.50f,float3(0,0,col4));

	va->AddVertexTN(start+dir*size*0.5f, 0.1875f+tex+0.5f,0.50f,float3(0,0,col4));
	va->AddVertexTN(start+dir*size*0.5f+orto1*size*0.5f+orto2*size*0.2f, 0.249f+tex+0.5f,0.02f,float3(0,0,col2));
	va->AddVertexTN(start+dir*size+orto2*size*0.1f, 0.249f+tex+0.5f,0.98f,float3(0,0,col5));

	va->AddVertexTN(start+dir*size*0.5f, 0.1875f+tex+0.5f,0.50f,float3(0,0,col4));
	va->AddVertexTN(start+dir*size*0.5f-orto1*size*0.5f+orto2*size*0.2f, 0.126f+tex+0.5f,0.98f,float3(0,0,col3));
	va->AddVertexTN(start+dir*size+orto2*size*0.1f, 0.249f+tex+0.5f,0.98f,float3(0,0,col5));
}

float CAdvTreeGenerator::fRand(float size)
{
	return float(rand())/RAND_MAX*size;
}

void CAdvTreeGenerator::CreateLeafTex(unsigned int baseTex, int xpos, int ypos,unsigned char buf[256][2048][4])
{
	unsigned char* buf2=new unsigned char[256*256*4];

	glViewport(0,0,256,256);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -5, 5);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.5f);
	glBindTexture(GL_TEXTURE_2D, baseTex);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glColor4f(1,1,1,1);

	glClearColor(0.0f,0.0f,0.0f,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float baseCol=0.8f+fRand(0.2f);

	for(int a=0;a<84;++a){
		float xp=0.9f-fRand(1.8f);
		float yp=0.9f-fRand(1.8f);

		float rot=fRand(360);

		float rCol=0.7f+fRand(0.3f);
		float gCol=0.7f+fRand(0.3f);
		float bCol=0.7f+fRand(0.3f);

		glPushMatrix();
		glLoadIdentity();
		glColor3f(baseCol*rCol,baseCol*gCol,baseCol*bCol);
		glTranslatef(xp,yp,0);
		glRotatef(rot,0,0,1);
		glRotatef(fRand(360),1,0,0);
		glRotatef(fRand(360),0,1,0);

		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(-0.1f,-0.2f,0);
			glTexCoord2f(0,1); glVertex3f(-0.1f,0.2f,0);
			glTexCoord2f(1,1); glVertex3f(0.1f,0.2f,0);
			glTexCoord2f(1,0); glVertex3f(0.1f,-0.2f,0);
		glEnd();

		glPopMatrix();
	}

	glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,buf2);

//	CBitmap bm(buf2,256,256);
//	bm.Save("leaf.bmp");

	for(int y=0;y<256;++y){
		for(int x=0;x<256;++x){
			if(buf2[(y*256+x)*4+1]!=0){
				buf[y+ypos][x+xpos][0]=buf2[(y*256+x)*4+0];
				buf[y+ypos][x+xpos][1]=buf2[(y*256+x)*4+1];
				buf[y+ypos][x+xpos][2]=buf2[(y*256+x)*4+2];
				buf[y+ypos][x+xpos][3]=255;
			} else {
				buf[y+ypos][x+xpos][0]=(unsigned char)(0.24f*1.2f*255);
				buf[y+ypos][x+xpos][1]=(unsigned char)(0.40f*1.2f*255);
				buf[y+ypos][x+xpos][2]=(unsigned char)(0.23f*1.2f*255);
				buf[y+ypos][x+xpos][3]=0;
			}
		}
	}

	glViewport(globalRendering->viewPosX,0,globalRendering->viewSizeX,globalRendering->viewSizeY);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	delete[] buf2;
}
