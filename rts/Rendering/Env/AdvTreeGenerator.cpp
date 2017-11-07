/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <vector>

#include "AdvTreeGenerator.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/myMath.h"

using std::max;
using std::min;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CAdvTreeGenerator::Init()
{
	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);

	if (!resourcesParser.Execute())
		LOG_L(L_ERROR, "%s", resourcesParser.GetErrorLog().c_str());

	const LuaTable treesTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("trees");

	CBitmap bm;
	std::string fn("bitmaps/" + treesTable.GetString("bark", "Bark.bmp"));
	if (!bm.Load(fn) || bm.xsize != 256 || bm.ysize != 256)
		throw content_error("Could not load tree texture from file " + fn);

	FBO fbo;
	fbo.Bind();
	fbo.CreateRenderBuffer(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT16, 256, 256);
	fbo.CreateRenderBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA8, 256, 256);
	if (!fbo.CheckStatus("ADVTREE")) {
		fbo.Unbind();
		throw content_error("Could not create FBO!");
	}

	glDisable(GL_CULL_FACE);

	unsigned char(* tree)[2048][4] = new unsigned char[256][2048][4];
	const unsigned char* rmem = bm.GetRawMem();

	memset(tree[0][0], 128, 256 * 2048 * 4);
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			tree[y][x][0] = rmem[(y * 256 + x) * 4    ];
			tree[y][x][1] = rmem[(y * 256 + x) * 4 + 1];
			tree[y][x][2] = rmem[(y * 256 + x) * 4 + 2];
			tree[y][x][3] = 255;
			tree[y][x + 1024][0] = (unsigned char)(rmem[(y * 256 + x) * 4    ] * 0.6f);
			tree[y][x + 1024][1] = (unsigned char)(rmem[(y * 256 + x) * 4 + 1] * 0.6f);
			tree[y][x + 1024][2] = (unsigned char)(rmem[(y * 256 + x) * 4 + 2] * 0.6f);
			tree[y][x + 1024][3] = 255;
		}
	}


	fn = "bitmaps/" + treesTable.GetString("leaf", "bleaf.bmp");
	if (!bm.Load(fn)) {
		delete[] tree;
		fbo.Unbind();
		throw content_error("Could not load tree texture from file " + fn);
	}

	bm.CreateAlpha(0, 0, 0);
	//bm.Renormalize(float3(0.22f, 0.43f, 0.18f));

	GLuint leafTex;
	glGenTextures(1, &leafTex);
	glBindTexture(GL_TEXTURE_2D, leafTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetRawMem());

	CreateLeafTex(leafTex, 256, 0, tree);
	CreateLeafTex(leafTex, 512, 0, tree);
	CreateLeafTex(leafTex, 768, 0, tree);

	glDeleteTextures(1, &leafTex);

	unsigned char* data = tree[0][0];
	CreateGranTex(data, 1024 + 768, 0, 2048);
	CreateGranTex(data, 1280,       0, 2048);
	CreateGranTex(data, 1536,       0, 2048);

	glGenTextures(1, &barkTex);
	glBindTexture(GL_TEXTURE_2D, barkTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glBuildMipmaps(GL_TEXTURE_2D, GL_RGBA8, 2048, 256, GL_RGBA, GL_UNSIGNED_BYTE, data);

	delete[] tree;

	fbo.Unbind();


	leafDL = glGenLists(8);

	for (int a = 0; a < 8; ++a) {
		va = GetVertexArray();
		va->Initialize();
		barkva = GetVertexArray();
		barkva->Initialize();

		glNewList(leafDL + a, GL_COMPILE);
			float size = 0.65f + 0.2f * guRNG.NextFloat();
			MainTrunk(10, size * MAX_TREE_HEIGHT, size * 0.05f * MAX_TREE_HEIGHT);
			va->DrawArrayTN(GL_QUADS);
			barkva->DrawArrayTN(GL_TRIANGLE_STRIP);
		glEndList();
	}

	pineDL = glGenLists(8);

	for (int a = 0; a < 8; ++a) {
		va = GetVertexArray();
		va->Initialize();

		glNewList(pineDL + a, GL_COMPILE);
			float size = 0.7f + 0.2f * guRNG.NextFloat();
			PineTree((int)(20 + 10 * guRNG.NextFloat()), MAX_TREE_HEIGHT * size);
			va->DrawArrayTN(GL_TRIANGLES);
		glEndList();
	}
}

CAdvTreeGenerator::~CAdvTreeGenerator()
{
	glDeleteTextures(1, &barkTex);

	glDeleteLists(leafDL, 8);
	glDeleteLists(pineDL, 8);
}

void CAdvTreeGenerator::DrawTrunk(const float3& start, const float3& end, const float3& orto1, const float3& orto2, float size)
{
	const float3 flatSun = sky->GetLight()->GetLightDir() * XZVector;

	const int numIter = (int)max(3.0f, size * 10.0f);
	for (int a = 0; a <= numIter; a++) {
		const float angle = a / (float)numIter * math::TWOPI;
		const float col = 0.4f + (((orto1 * std::sin(angle) + orto2 * std::cos(angle)).dot(flatSun))) * 0.3f;

		barkva->AddVertexTN(start + orto1 * std::sin(angle) * size        + orto2 * std::cos(angle) * size       , angle / math::PI * 0.125f * 0.5f, 0, FwdVector * col);
		barkva->AddVertexTN(  end + orto1 * std::sin(angle) * size * 0.2f + orto2 * std::cos(angle) * size * 0.2f, angle / math::PI * 0.125f * 0.5f, 3, FwdVector * col);
	}
	barkva->EndStrip();
}

void CAdvTreeGenerator::MainTrunk(int numBranch,float height,float width)
{
	const float3 orto1 = RgtVector;
	const float3 orto2 = FwdVector;
	const float baseAngle = math::TWOPI * guRNG.NextFloat();

	DrawTrunk(ZeroVector, float3(0, height, 0), orto1, orto2, width);

	for (int a = 0; a < numBranch; ++a) {
		const float angle = baseAngle + (a * 3.88f) + 0.5f * guRNG.NextFloat();
		float3 dir = orto1 * std::sin(angle) + orto2 * std::cos(angle);
			dir.y = 0.3f + 0.4f * guRNG.NextFloat();
			dir.ANormalize();
		const float3 start(0, (a + 5) * height / (numBranch + 5), 0);
		const float length = (height * (0.4f + 0.1f * guRNG.NextFloat())) * std::sqrt(float(numBranch - a) / numBranch);

		TrunkIterator(start, dir, length, length * 0.05f, 1);
	}

	for (int a = 0; a < 3; ++a) {
		const float angle = (a * 3.88f) + 0.5f * guRNG.NextFloat();
		float3 dir = orto1*std::sin(angle)+orto2*std::cos(angle);
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
		orto1 = dir.cross(RgtVector);
	orto1.ANormalize();

	float3 orto2 = dir.cross(orto1);
	orto2.ANormalize();

	DrawTrunk(start, start + dir * length, orto1, orto2, size);

	if (depth <= 1)
		CreateLeaves(start, dir, length, orto1, orto2);

	if (depth == 0)
		return;

	const float dirDif = 0.8f * guRNG.NextFloat() + 1.0f;
	const int numTrunks = (int) length * 5 / MAX_TREE_HEIGHT;

	for (int a = 0; a < numTrunks; a++) {
		const float angle = math::PI + float(a) * math::PI + 0.3f * guRNG.NextFloat();
		const float newLength = length * (float(numTrunks - a) / (numTrunks + 1));

		float3 newbase = start + dir * length * (float(a + 1) / (numTrunks + 1));
		float3 newDir = dir + orto1 * std::cos(angle) * dirDif + orto2 * std::sin(angle) * dirDif;
		newDir.ANormalize();

		TrunkIterator(newbase, newDir, newLength, newLength * 0.05f, depth - 1);
	}
}

void CAdvTreeGenerator::CreateLeaves(const float3& start, const float3& dir, float length, float3& orto1,float3& orto2)
{
	const float baseRot = math::TWOPI * guRNG.NextFloat();
	const int numLeaves = (int) length * 10 / MAX_TREE_HEIGHT;

	float3 flatSun = sky->GetLight()->GetLightDir();
	flatSun.y = 0.0f;

	auto ADD_LEAF = [&](const float3& pos) {
		float3 npos = pos;
		npos.y = 0;
		npos.ANormalize();

		const float tex = (guRNG.NextInt(3)) * 0.125f;
		const float flipTex = (guRNG.NextInt(2)) * 0.123f;
		const float col = 0.5f + (npos.dot(flatSun) * 0.3f) + 0.1f * guRNG.NextFloat();

		va->AddVertexTN(pos, 0.126f + tex + flipTex, 0.98f, float3( 0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col));
		va->AddVertexTN(pos, 0.249f + tex - flipTex, 0.98f, float3(-0.09f * MAX_TREE_HEIGHT, -0.09f * MAX_TREE_HEIGHT, col));
		va->AddVertexTN(pos, 0.249f + tex - flipTex, 0.02f, float3(-0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col));
		va->AddVertexTN(pos, 0.126f + tex + flipTex, 0.02f, float3( 0.09f * MAX_TREE_HEIGHT,  0.09f * MAX_TREE_HEIGHT, col));
	};

	for (int a = 0; a < numLeaves + 1; a++) {
		const float angle = baseRot + a * 0.618f * math::TWOPI;

		float3 pos = start + dir * length * (0.7f + 0.3f * guRNG.NextFloat());
		pos +=
			(orto1 * std::sin(angle) + orto2 * std::cos(angle)) *
			mix(guRNG.NextFloat(), std::sqrt((float) a + 1), 0.6f) *
			0.1f * MAX_TREE_HEIGHT;

		if (pos.y < 0.2f * MAX_TREE_HEIGHT)
			pos.y = 0.2f * MAX_TREE_HEIGHT;

		ADD_LEAF(pos);
	}

	ADD_LEAF(start + dir * length * 1.03f);
}


void CAdvTreeGenerator::CreateGranTex(unsigned char* data, int xpos, int ypos, int xsize)
{
	glSpringMatrix2dSetupVP(0.0f, 1.0f, 0.0f, 1.0f, -4.0f, 4.0f, true, true);
	glViewport(0,0,256,256);

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glColor4f(1,1,1,1);

	glAlphaFunc(GL_GREATER,0.5f);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);

	glClearColor(0.0f,0.0f,0.0f,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CreateGranTexBranch(ZeroVector,float3(0.93f,0.93f,0));

	std::vector<unsigned char> buf(256 * 256 * 4);
	glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,&buf[0]);

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

	glPopAttrib();

	glSpringMatrix2dResetPV(true, true);
}

void CAdvTreeGenerator::CreateGranTexBranch(const float3& start, const float3& end)
{
	float3 dir=end-start;
	float length=dir.Length();
	dir.ANormalize();
	float3 orto=dir.cross(FwdVector);

	glBegin(GL_QUADS);
		float3 c = float3(guRNG.NextFloat(), guRNG.NextFloat(), guRNG.NextFloat());
		c *= float3(0.02f, 0.05f, 0.01f);
		c += float3(0.05f, 0.21f, 0.04f);
		glColorf3(c);
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

	float tipDist = 0.025f;
	float delta = 0.013f;
	int side = (guRNG.NextInt() & 1) * 2 - 1;

	while(tipDist < length * (0.15f * guRNG.NextFloat() + 0.83f)){
		float3 bstart=start+dir*(length-tipDist);
		float3 bdir=dir+orto*side*(1.0f + 0.7f * guRNG.NextFloat());
		bdir.ANormalize();
		float3 bend=bstart+bdir*tipDist*((6-tipDist)*(0.10f + 0.05f * guRNG.NextFloat()));
		CreateGranTexBranch(bstart,bend);
		side*=-1;
		tipDist+=delta;
		delta+=0.005f;
	}
}

void CAdvTreeGenerator::PineTree(int numBranch, float height)
{
	DrawPineTrunk(ZeroVector,float3(0,height,0),height*0.025f);
	float3 orto1 = RgtVector;
	float3 orto2 = FwdVector;
	float baseAngle = math::TWOPI * guRNG.NextFloat();
	for (int a = 0; a < numBranch; ++a) {
		float sh = 0.2f + 0.2f * guRNG.NextFloat();
		float h  = height * std::pow(sh + float(a)/numBranch * (1-sh), (float)0.7f);
		float angle = baseAngle + (a * 0.618f + 0.1f * guRNG.NextFloat()) * math::TWOPI;
		float3 dir(orto1 * std::sin(angle) + orto2 * std::cos(angle));
		dir.y = (a - numBranch) * 0.01f - (0.2f + 0.2f * guRNG.NextFloat());
		dir.ANormalize();
		float size = std::sqrt((float)numBranch - a + 5) * 0.08f * MAX_TREE_HEIGHT;
		DrawPineBranch(float3(0,h,0),dir,size);
	}
	//create the top
	float col=0.55f + 0.2f * guRNG.NextFloat();
	va->AddVertexTN(float3(0,height-0.09f*MAX_TREE_HEIGHT,0), 0.126f+0.5f, 0.02f, float3(0,0,col));
	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.249f+0.5f, 0.02f, float3(0.05f*MAX_TREE_HEIGHT,0,col));
	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.126f+0.5f, 0.98f, float3(-0.05f*MAX_TREE_HEIGHT,0,col));

	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.249f+0.5f, 0.02f, float3(0.05f*MAX_TREE_HEIGHT,0,col));
	va->AddVertexTN(float3(0,height-0.03f*MAX_TREE_HEIGHT,0), 0.126f+0.5f, 0.98f, float3(-0.05f*MAX_TREE_HEIGHT,0,col));
	va->AddVertexTN(float3(0,height+0.03f*MAX_TREE_HEIGHT,0), 0.249f+0.5f, 0.98f, float3(0,0,col));
}

void CAdvTreeGenerator::DrawPineTrunk(const float3 &start, const float3 &end, float size)
{
	float3 orto1 = RgtVector;
	float3 orto2 = FwdVector;

	float3 flatSun = sky->GetLight()->GetLightDir();
	flatSun.y = 0.0f;

	int numIter=8;
	for(int a=0;a<numIter;a++){
		float angle=a/(float)numIter * math::TWOPI;
		float angle2=(a+1)/(float)numIter * math::TWOPI;
		float col=0.45f+(((orto1*std::sin(angle)+orto2*std::cos(angle)).dot(flatSun)))*0.3f;
		float col2=0.45f+(((orto1*std::sin(angle2)+orto2*std::cos(angle2)).dot(flatSun)))*0.3f;

		va->AddVertexTN(start+orto1*std::sin(angle)*size+orto2*std::cos(angle)*size,           angle / math::PI*0.125f*0.5f+0.5f, 0, float3(0,0,col));
		va->AddVertexTN(end+orto1*std::sin(angle)*size*0.1f+orto2*std::cos(angle)*size*0.1f,   angle / math::PI*0.125f*0.5f+0.5f, 3, float3(0,0,col));
		va->AddVertexTN(start+orto1*std::sin(angle2)*size+orto2*std::cos(angle2)*size,         angle2/ math::PI*0.125f*0.5f+0.5f, 0, float3(0,0,col2));

		va->AddVertexTN(start+orto1*std::sin(angle2)*size+orto2*std::cos(angle2)*size,         angle2 / math::PI*0.125f*0.5f+0.5f, 0, float3(0,0,col2));
		va->AddVertexTN(end+orto1*std::sin(angle)*size*0.1f+orto2*std::cos(angle)*size*0.1f,   angle  / math::PI*0.125f*0.5f+0.5f, 3, float3(0,0,col));
		va->AddVertexTN(end+orto1*std::sin(angle2)*size*0.1f+orto2*std::cos(angle2)*size*0.1f, angle2 / math::PI*0.125f*0.5f+0.5f, 3, float3(0,0,col2));
	}
}

void CAdvTreeGenerator::DrawPineBranch(const float3 &start, const float3 &dir, float size)
{
	float3 flatSun = sky->GetLight()->GetLightDir();
	flatSun.y = 0.0f;

	float3 orto1 = dir.cross(UpVector);
	orto1.ANormalize();
	float3 orto2 = dir.cross(orto1);

	float tex = 0.0f; // int(guRNG.NextFloat() * 3.0f) * 0.125f;
	float baseCol = 0.4f + 0.3f * dir.dot(flatSun) + 0.1f * guRNG.NextFloat();

	float col1=baseCol + 0.2f * guRNG.NextFloat();
	float col2=baseCol + 0.2f * guRNG.NextFloat();
	float col3=baseCol + 0.2f * guRNG.NextFloat();
	float col4=baseCol + 0.2f * guRNG.NextFloat();
	float col5=baseCol + 0.2f * guRNG.NextFloat();

	va->AddVertexTN(start,                                               0.5f+0.126f+tex,  0.02f, float3(0,0,col1));
	va->AddVertexTN(start+dir*size*0.5f+orto1*size*0.5f+orto2*size*0.2f, 0.5f+0.249f+tex,  0.02f, float3(0,0,col2));
	va->AddVertexTN(start+dir*size*0.5f,                                 0.5f+0.1875f+tex, 0.50f, float3(0,0,col4));

	va->AddVertexTN(start,                                               0.5f+0.126f+tex,  0.02f, float3(0,0,col1));
	va->AddVertexTN(start+dir*size*0.5f-orto1*size*0.5f+orto2*size*0.2f, 0.5f+0.126f+tex,  0.98f, float3(0,0,col3));
	va->AddVertexTN(start+dir*size*0.5f,                                 0.5f+0.1875f+tex, 0.50f, float3(0,0,col4));

	va->AddVertexTN(start+dir*size*0.5f,                                 0.5f+0.1875f+tex, 0.50f, float3(0,0,col4));
	va->AddVertexTN(start+dir*size*0.5f+orto1*size*0.5f+orto2*size*0.2f, 0.5f+0.249f+tex,  0.02f, float3(0,0,col2));
	va->AddVertexTN(start+dir*size+orto2*size*0.1f,                      0.5f+0.249f+tex,  0.98f, float3(0,0,col5));

	va->AddVertexTN(start+dir*size*0.5f,                                 0.5f+0.1875f+tex, 0.50f, float3(0,0,col4));
	va->AddVertexTN(start+dir*size*0.5f-orto1*size*0.5f+orto2*size*0.2f, 0.5f+0.126f+tex,  0.98f, float3(0,0,col3));
	va->AddVertexTN(start+dir*size+orto2*size*0.1f,                      0.5f+0.249f+tex,  0.98f, float3(0,0,col5));
}


void CAdvTreeGenerator::CreateLeafTex(unsigned int baseTex, int xpos, int ypos,unsigned char buf[256][2048][4])
{
	glViewport(0,0,256,256);
	glSpringMatrix2dSetupPV(-1.0f, 1.0f, -1.0f, 1.0f, -5.0f, 5.0f, true, true);

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

	float baseCol=0.8f + 0.2f * guRNG.NextFloat();

	for(int a=0;a<84;++a){
		float xp=0.9f - 1.8f * guRNG.NextFloat();
		float yp=0.9f - 1.8f * guRNG.NextFloat();

		float rot=360 * guRNG.NextFloat();

		float rCol=0.7f + 0.3f * guRNG.NextFloat();
		float gCol=0.7f + 0.3f * guRNG.NextFloat();
		float bCol=0.7f + 0.3f * guRNG.NextFloat();

		GL::PushMatrix();
		GL::LoadIdentity();
		glColor3f(baseCol*rCol,baseCol*gCol,baseCol*bCol);
		GL::Translate(xp,yp,0);
		GL::RotateZ(rot);
		GL::RotateX(360 * guRNG.NextFloat());
		GL::RotateY(360 * guRNG.NextFloat());

		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(-0.1f,-0.2f,0);
			glTexCoord2f(0,1); glVertex3f(-0.1f,0.2f,0);
			glTexCoord2f(1,1); glVertex3f(0.1f,0.2f,0);
			glTexCoord2f(1,0); glVertex3f(0.1f,-0.2f,0);
		glEnd();

		GL::PopMatrix();
	}

	std::vector<unsigned char> buf2(256 * 256 * 4);
	glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,&buf2[0]);

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
	glSpringMatrix2dResetPV(true, true);
}

