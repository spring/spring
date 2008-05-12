/**
 * @file BumpWater.cpp
 * @brief extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#include "StdAfx.h"
#include "bitops.h"
#include "FileSystem/FileHandler.h"
#include "BumpWater.h"
#include "Game/Game.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "LogOutput.h"
#include "Map/BaseGroundDrawer.h"
#include "BaseSky.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Lua/LuaCallInHandler.h"
#include "System/Platform/ConfigHandler.h"
#include <boost/format.hpp>

using std::string;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern GLfloat FogLand[];


static void PrintLog(GLuint obj)
{
	int infologLength = 0;
	int maxLength;

	if(glIsShader(obj))
		glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	else
		glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);

	char* infoLog = SAFE_NEW char[maxLength];

	if (glIsShader(obj))
		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);

	if (infologLength > 0) {
		string str(infoLog, infologLength);
		delete[] infoLog;
		throw content_error(string("BumpWater shader error: " + str));
	}

	delete[] infoLog;
}


void PrintFboError(GLenum error)
{
	switch(error) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			logOutput.Print("BumpWater-FBO: missing a required image/buffer attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			logOutput.Print("BumpWater-FBO: has no images/buffers attached!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			logOutput.Print("BumpWater-FBO: has mismatched image/buffer dimensions!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			logOutput.Print("BumpWater-FBO: colorbuffer attachments have different types!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			logOutput.Print("BumpWater-FBO: trying to draw to non-attached color buffer!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			logOutput.Print("BumpWater-FBO: trying to read from a non-attached color buffer!");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			logOutput.Print("BumpWater-FBO: format is not supported by current graphics card/driver!");
			break;
		default:
			logOutput.Print("BumpWater-FBO: *UNKNOWN ERROR*");
			break;
	}
}

static string LoadShaderSource(const string& file)
{
	CFileHandler fh(file);
	if (!fh.FileExists())
		throw content_error("Can't load shader " + file);

	string text;
	text.resize(fh.FileSize());
	//char* cstr = SAFE_NEW char[fh.FileSize()+1];

	fh.Read(&text[0], text.length());
	//fh.Read(cstr,fh.FileSize());
	//cstr[fh.FileSize()] = 0;

	return text;
}

static void GLSLDefineConstf4(string& str, const string& name, const float3& v, const float& alpha)
{
	str += boost::str(boost::format(string("#define ")+name+" vec4(%1$.12f,%2$.12f,%3$.12f,%4$.12f)\n") % (v.x) % (v.y) % (v.z) % (alpha));
}

static void GLSLDefineConstf3(string& str, const string& name, const float3& v)
{
	str += boost::str(boost::format(string("#define ")+name+" vec3(%1$.12f,%2$.12f,%3$.12f)\n") % (v.x) % (v.y) % (v.z));
}

static void GLSLDefineConstf2(string& str, const string& name, const float& x, const float& y)
{
	str += boost::str(boost::format(string("#define ")+name+" vec2(%1$.12f,%2$.12f)\n") % x % y);
}

static void GLSLDefineConstf1(string& str, const string& name, const float& x)
{
	str += boost::str(boost::format(string("#define ")+name+" %1$.12f\n") % x);
}

static GLuint LoadTexture(const string& filename)
{
	GLuint texID;
	CBitmap bm;
	if (!bm.Load(filename))
		throw content_error("Could not load texture from file "+filename);

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 3.0f);
	glBuildMipmaps(GL_TEXTURE_2D, GLEW_ARB_texture_compression?GL_COMPRESSED_RGB_ARB:GL_RGB8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);
	return texID;
}

CBumpWater::CBumpWater()
{
	reflTexSize = next_power_of_2(configHandler.GetInt("BumpWaterTexSizeReflection", 256));
	reflection = !!configHandler.GetInt("BumpWaterReflection", 1);
	refraction = configHandler.GetInt("BumpWaterRefraction", 1);  //0:=off, 1:=screencopy, 2:=own rendering cycle
	waves = !!configHandler.GetInt("BumpBeachWaves", 1);

	if (!GLEW_EXT_framebuffer_object) {
		reflection = false;
	}

	if (refraction>0) {
		// CREATE REFRACTION TEXTURE
		glGenTextures(1, &refractTexture);
		//if(GLEW_ARB_texture_non_power_of_two || GLEW_EXT_texture_non_power_of_two)
		//	target = GL_TEXTURE_2D;
		//else //if(GLEW_ARB_texture_rectangle || GLEW_EXT_texture_rectangle)
			target = GL_TEXTURE_RECTANGLE_ARB;
		glBindTexture(target, refractTexture);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		if (GLEW_EXT_texture_edge_clamp) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
		if(target == GL_TEXTURE_RECTANGLE_ARB) {
			refrSizeX = gu->viewSizeX;
			refrSizeY = gu->viewSizeY;
		} else{
			refrSizeX = next_power_of_2(gu->viewSizeX);
			refrSizeY = next_power_of_2(gu->viewSizeY);
		}
		glTexImage2D(target, 0, GL_RGB8, refrSizeX, refrSizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	}

	if (reflection) {
		// CREATE REFLECTION TEXTURE
		glGenTextures(1, &reflectTexture);
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		if (GLEW_EXT_texture_edge_clamp) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, reflTexSize, reflTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		// CREATE DEPTH RBO FOR REFLECTION FBO
		glGenRenderbuffersEXT(1, &rbo);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbo);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT32, reflTexSize, reflTexSize);

		// CREATE REFLECTION FBO AND BIND TEXTURE&RBO
		glGenFramebuffersEXT(1,&fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, reflectTexture, 0);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbo);
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			PrintFboError(status);
			//logOutput.Print("BumpWater: FBO not ready");
		}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}


	foamTexture = LoadTexture( mapInfo->water.foamTexture );
	normalTexture = LoadTexture( mapInfo->water.normalTexture );
	for (int i = 0; i < 32; i++) {
		caustTextures[i] = LoadTexture( mapInfo->water.causticTextures[i] );
	}

	//heightTexture = readmap->GetShadingTexture();
	/*

	*/

	/* DEFINE SOME RUNTIME CONSTANTS (I don't use Uniforms for that, 'cos the glsl compiler can't optimize those!) */
	string definitions;
	if (reflection)   definitions += "#define use_reflection\n";
	if (refraction>0) definitions += "#define use_refraction\n\n";
	GLSLDefineConstf4(definitions, "SurfaceColor", mapInfo->water.surfaceColor*0.4, mapInfo->water.surfaceAlpha );
	GLSLDefineConstf3(definitions, "SpecularColor", mapInfo->water.specularColor );
	GLSLDefineConstf1(definitions, "SpecularFactor", mapInfo->water.specularFactor);
	GLSLDefineConstf3(definitions, "SunDir", mapInfo->light.sunDir );
	GLSLDefineConstf3(definitions, "MapMid", float3(readmap->width*SQUARE_SIZE*0.5f,0.0f,readmap->height*SQUARE_SIZE*0.5f) );
	GLSLDefineConstf2(definitions, "ScreenInverse", -1.0f/gu->viewSizeX, 1.0f/gu->viewSizeY );
	GLSLDefineConstf2(definitions, "ViewPos", gu->viewPosX,gu->viewPosY );
	GLSLDefineConstf1(definitions, "FresnelMin",  mapInfo->water.fresnelMin);
	GLSLDefineConstf1(definitions, "FresnelMax",  mapInfo->water.fresnelMax);
	GLSLDefineConstf1(definitions, "FresnelPower", mapInfo->water.fresnelPower);

	/* LOAD SHADERS */
	string vsSource = LoadShaderSource("shaders/bumpWaterVS.glsl");
	string fsSource = LoadShaderSource("shaders/bumpWaterFS.glsl");

	vector<GLint> lengths(2);
	vector<const GLchar*> strings(2);
	lengths[0]=definitions.length();
	strings[0]=definitions.c_str();

	waterVP = glCreateShader(GL_VERTEX_SHADER);
	lengths[1]=vsSource.length();
	strings[1]=vsSource.c_str();
	glShaderSource(waterVP, strings.size(), &strings.front(), &lengths.front());
	glCompileShader(waterVP);
	PrintLog(waterVP);

	waterFP = glCreateShader(GL_FRAGMENT_SHADER);
	lengths[1]= fsSource.length();
	strings[1] = fsSource.c_str();
	glShaderSource(waterFP, strings.size(), &strings.front(), &lengths.front());
	glCompileShader(waterFP);
	PrintLog(waterFP);

	//delete vsSource; delete fsSource;

	waterShader = glCreateProgram();
	glAttachShader(waterShader, waterVP);
	glAttachShader(waterShader, waterFP);
	glLinkProgram(waterShader);
	PrintLog(waterShader);

	/* BIND TEXTURE UNIFORMS */
	glUseProgram(waterShader);
		eyePosLoc     = glGetUniformLocation(waterShader, "eyePos");
		frameLoc      = glGetUniformLocation(waterShader, "frame");
		normalmapLoc  = glGetUniformLocation(waterShader, "normalmap");
		heightmapLoc  = glGetUniformLocation(waterShader, "heightmap");
		causticLoc    = glGetUniformLocation(waterShader, "caustic");
		foamLoc       = glGetUniformLocation(waterShader, "foam");
		reflectionLoc = glGetUniformLocation(waterShader, "reflection");
		refractionLoc = glGetUniformLocation(waterShader, "refraction");

		glUniform1i(normalmapLoc, 0);
		glUniform1i(heightmapLoc, 1);
		glUniform1i(causticLoc, 2);
		glUniform1i(foamLoc, 3);
		glUniform1i(reflectionLoc, 4);
		glUniform1i(refractionLoc, 5);
	glUseProgram(0);
}

CBumpWater::~CBumpWater()
{
	if (reflection) {
		glDeleteTextures(1, &reflectTexture);
		glDeleteRenderbuffersEXT(1, &rbo);
		glDeleteFramebuffersEXT(1, &fbo);
	}
	if (refraction>0)
		glDeleteTextures(1, &refractTexture);

	glDeleteTextures(1, &foamTexture);
	glDeleteTextures(32, caustTextures);
	glDeleteTextures(1, &normalTexture);

	glDeleteShader(waterVP);
	glDeleteShader(waterFP);
	glDeleteProgram(waterShader);
}

void CBumpWater::Draw()
{
	if(readmap->minheight>10)
		return;

	if (refraction == 1) {
		// _SCREENCOPY_ REFRACT TEXTURE
		glBindTexture(target, refractTexture);
		glEnable(target);
		glCopyTexSubImage2D(target, 0, 0, 0, gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
		glDisable(target);
	}

	glDisable(GL_ALPHA_TEST);
	if (refraction>0)
		glDisable(GL_BLEND);
	if (refraction<2)
		glDepthMask(0);

	glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, caustTextures[gs->frameNum%32]);
	glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, foamTexture);
	glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glActiveTexture(GL_TEXTURE5);
		glBindTexture(target, refractTexture);
	glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, normalTexture);

	glUseProgram(waterShader);
	glUniform1f(frameLoc,gs->frameNum/15000.0f);
	glUniformf3(eyePosLoc,camera->pos);

	glBegin(GL_QUADS);
		glTexCoord4f(0.0f,0.0f,0.0f,0.0f);
			glVertex3i(0,0,0);
		glTexCoord4f(0.0f,1.0f,0.0f,(float)gs->mapy/gs->pwr2mapy);
			glVertex3i(0,0,readmap->height*SQUARE_SIZE);
		glTexCoord4f(1.0f,1.0f,(float)gs->mapx/gs->pwr2mapx,(float)gs->mapy/gs->pwr2mapy);
			glVertex3i(readmap->width*SQUARE_SIZE,0,readmap->height*SQUARE_SIZE);
		glTexCoord4f(1.0f,0.0f,(float)gs->mapx/gs->pwr2mapx,0.0f);
			glVertex3i(readmap->width*SQUARE_SIZE,0,0);
	glEnd();

	glUseProgram(0);

	if (refraction<2)
		glDepthMask(1);
}

void CBumpWater::UpdateWater(CGame* game)
{
	if (readmap->minheight > 10 || mapInfo->map.voidWater)
		return;

	if (refraction>1) DrawRefraction(game);
	if (reflection)   DrawReflection(game);
}

void CBumpWater::DrawRefraction(CGame* game)
{
	// _RENDER_ REFRACTION TEXTURE
	drawRefraction=true;
	camera->Update(false);
	glViewport(0,0,refrSizeX,refrSizeY);

	glClearColor(FogLand[0],FogLand[1],FogLand[2],1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float3 oldsun=unitDrawer->unitSunColor;
	float3 oldambient=unitDrawer->unitAmbientColor;

	unitDrawer->unitSunColor*=float3(0.5f,0.7f,0.9f);
	unitDrawer->unitAmbientColor*=float3(0.6f,0.8f,1.0f);

	game->SetDrawMode(CGame::refractionDraw);

	glEnable(GL_CLIP_PLANE2);
	double plane[4]={0,-1,0,2};
	glClipPlane(GL_CLIP_PLANE2 ,plane);
	drawReflection=true;

	readmap->GetGroundDrawer()->Draw();
	unitDrawer->Draw(false,true);
	featureHandler->Draw();
	drawReflection=false;
	ph->Draw(false,true);
	luaCallIns.DrawWorldRefraction();

	glDisable(GL_CLIP_PLANE2);
	game->SetDrawMode(CGame::normalDraw);
	drawRefraction=false;

	glBindTexture(target, refractTexture);
	glEnable(target);
	glCopyTexSubImage2D(target,0,0,0,0,0,refrSizeX,refrSizeY);
	glDisable(target);

	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
	glClearColor(FogLand[0],FogLand[1],FogLand[2],1);

	unitDrawer->unitSunColor=oldsun;
	unitDrawer->unitAmbientColor=oldambient;
}

void CBumpWater::DrawReflection(CGame* game)
{
	// CREATE REFLECTION TEXTURE
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

	CCamera *realCam = camera;
	camera = new CCamera(*realCam);
	camera->up.x=0;
	camera->up.y=1;
	camera->up.z=0;
	camera->forward.y*=-1;
	camera->pos.y*=-1;
	camera->Update(false);

	glViewport(0,0,reflTexSize,reflTexSize);

	glClearColor(0.2f,0.4f,0.2f,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	game->SetDrawMode(CGame::reflectionDraw);

	sky->Draw();

	glEnable(GL_CLIP_PLANE2);
	double plane[4]={0,1,0,0}; // make angle dependent?
	glClipPlane(GL_CLIP_PLANE2 ,plane);
	drawReflection=true;

	readmap->GetGroundDrawer()->Draw(true);
	unitDrawer->Draw(true);
	featureHandler->Draw();
	ph->Draw(true);
	luaCallIns.DrawWorldReflection();

	game->SetDrawMode(CGame::normalDraw);
	drawReflection=false;
	glDisable(GL_CLIP_PLANE2);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
	glClearColor(FogLand[0],FogLand[1],FogLand[2],1);

	delete camera;
	camera = realCam;
	camera->Update(false);
}
