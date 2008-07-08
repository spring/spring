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
#include "Rendering/GL/IFramebuffer.h"
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
#include "Sim/Misc/Wind.h"
#include "Lua/LuaCallInHandler.h"
#include "System/Platform/ConfigHandler.h"
#include <boost/format.hpp>

using std::string;
using std::vector;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static void PrintShaderLog(GLuint obj)
{
	// WAS COMPILATION SUCCESSFUL ?
	GLint compiled;
	if(glIsShader(obj))
		glGetShaderiv(obj,GL_COMPILE_STATUS,&compiled);
	else
		glGetProgramiv(obj,GL_COMPILE_STATUS,&compiled);

	if (compiled) return;

	// GET INFOLOG
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
		logOutput.Print("BumpWater shader error: " + str); //string size is limited with content_error()
		throw content_error(string("BumpWater shader error!"));
	}
	delete[] infoLog;
}


static void PrintFboError(string fbo, GLenum error)
{
	switch(error) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")has no images/buffers attached!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")missing a required image/buffer attachment!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")has mismatched image/buffer dimensions!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")colorbuffer attachments have different types!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")incomplete draw buffers!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")trying to read from a non-attached color buffer!");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			logOutput.Print("BumpWater-FBO: ("+fbo+")format is not supported by current graphics card/driver!");
			break;
		default:
			logOutput.Print("BumpWater-FBO: ("+fbo+")*UNKNOWN ERROR*");
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
	fh.Read(&text[0], text.length());
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

static GLuint LoadTexture(const string& filename, const float anisotropy = 0.0f)
{
	GLuint texID;
	CBitmap bm;
	if (!bm.Load(filename))
		throw content_error("Could not load texture from file "+filename);

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	if (anisotropy > 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	glBuildMipmaps(GL_TEXTURE_2D, GLEW_ARB_texture_compression?GL_COMPRESSED_RGBA_ARB:GL_RGBA8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);
	return texID;
}

CBumpWater::CBumpWater()
{
	/** LOAD USER CONFIGS **/
	reflTexSize = next_power_of_2(configHandler.GetInt("BumpWaterTexSizeReflection", 256));
	reflection  = !!configHandler.GetInt("BumpWaterReflection", 1);
	refraction  = configHandler.GetInt("BumpWaterRefraction", 1);  //0:=off, 1:=screencopy, 2:=own rendering cycle
	shorewaves  = !!configHandler.GetInt("BumpWaterShoreWaves", 0);
	anisotropy  = atof(configHandler.GetString("BumpWaterAnisotropy", "0.0").c_str());
	depthCopy   = !!configHandler.GetInt("BumpWaterUseDepthTexture", 1);
	depthBits   = configHandler.GetInt("BumpWaterDepthBits", 24);
	blurRefl    = !!configHandler.GetInt("BumpWaterBlurReflection", 0);

	if (refraction>1)
		drawSolid = true;


	/** CHECK HARDWARE **/
	if (!GL_ARB_shading_language_100)
		throw content_error("BumpWater: your hardware/driver setup does not support GLSL.");

	if (!(GLEW_ARB_texture_rectangle || GLEW_EXT_texture_rectangle))
		refraction = 0;

	shorewaves = shorewaves && (GLEW_EXT_framebuffer_object);


	/** CREATE TEXTURES **/
	if (refraction>0 || depthCopy) {
		if(GLEW_ARB_texture_rectangle || GLEW_EXT_texture_rectangle) {
			target = GL_TEXTURE_RECTANGLE_ARB;
			screenTextureX = gu->viewSizeX;
			screenTextureY = gu->viewSizeY;
		}else{
			target = GL_TEXTURE_2D;
			screenTextureX = next_power_of_2(gu->viewSizeX);
			screenTextureY = next_power_of_2(gu->viewSizeY);
		}
	}

	if (refraction>0) {
		// CREATE REFRACTION TEXTURE
		glGenTextures(1, &refractTexture);
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
		glTexImage2D(target, 0, GL_RGBA8, screenTextureX, screenTextureY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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
	}

	if (depthCopy) {
		glGenTextures(1, &depthTexture);
		glBindTexture(target, depthTexture);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		GLuint depthFormat = GL_DEPTH_COMPONENT;
		switch (gu->depthBufferBits) {
			case 16: depthFormat = GL_DEPTH_COMPONENT16; break;
			case 24: depthFormat = GL_DEPTH_COMPONENT24; break;
			case 32: depthFormat = GL_DEPTH_COMPONENT32; break;
		}
		glTexImage2D(target, 0, depthFormat, screenTextureX, screenTextureY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	}


	/** CREATE FBOs **/
	if (GLEW_EXT_framebuffer_object) {
		GLuint depthRBOFormat = GL_DEPTH_COMPONENT;
		switch (depthBits) {
			case 16: depthRBOFormat = GL_DEPTH_COMPONENT16; break;
			case 24: depthRBOFormat = GL_DEPTH_COMPONENT24; break;
			case 32: depthRBOFormat = GL_DEPTH_COMPONENT32; break;
		}

		if (reflection) {
			glGenRenderbuffersEXT(1, &reflectRBO);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, reflectRBO);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthRBOFormat, reflTexSize, reflTexSize);

			glGenFramebuffersEXT(1,&reflectFBO);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, reflectFBO);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, reflectRBO);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, reflectTexture, 0);
			GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
				PrintFboError("reflection",status);
				glDeleteRenderbuffersEXT(1, &reflectRBO);
				glDeleteFramebuffersEXT(1,  &reflectFBO);
				reflectFBO = 0;
			}
		}

		if (refraction>0) {
			glGenRenderbuffersEXT(1, &refractRBO);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, refractRBO);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthRBOFormat, screenTextureX, screenTextureY);

			glGenFramebuffersEXT(1,&refractFBO);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, refractFBO);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, refractRBO);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, target, refractTexture, 0);
			GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
				PrintFboError("refraction",status);
				glDeleteRenderbuffersEXT(1, &refractRBO);
				glDeleteFramebuffersEXT(1,  &refractFBO);
				refractFBO = 0;
			}
		}

	}


	/** LOAD TEXTURES **/
	foamTexture     = LoadTexture( mapInfo->water.foamTexture );
	normalTexture   = LoadTexture( mapInfo->water.normalTexture , anisotropy );

	// caustic textures
	const vector<string>& causticNames = mapInfo->water.causticTextures;
	if (causticNames.size() <= 0) {
		throw content_error("no caustic textures");
	}
	for (int i = 0; i < (int)causticNames.size(); i++) {
		caustTextures.push_back(LoadTexture(causticNames[i]));
	}

	if (shorewaves) {
		waveRandTexture = LoadTexture( "bitmaps/shorewaverand.bmp" );

		const GLchar* fsSource = LoadShaderSource("shaders/bumpWaterCoastBlurFS.glsl").c_str();
		blurFP = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(blurFP, 1, &fsSource, NULL);
		glCompileShader(blurFP);
		PrintShaderLog(blurFP);

		blurShader = glCreateProgram();
		glAttachShader(blurShader, blurFP);
		glLinkProgram(blurShader);
		PrintShaderLog(blurShader);
		blurDirLoc    = glGetUniformLocation(blurShader, "blurDir");
		GLuint texLoc = glGetUniformLocation(blurShader, "tex0");
		glUniform1i(texLoc, 0);

		glGenFramebuffersEXT(1,&coastFBO);
		GenerateCoastMap();
	}


	/** DEFINE SOME SHADER RUNTIME CONSTANTS (I don't use Uniforms for that, because the glsl compiler can't optimize those!) **/
	string definitions;
	if (reflection)   definitions += "#define use_reflection\n";
	if (refraction>0) definitions += "#define use_refraction\n";
	if (shorewaves)   definitions += "#define use_shorewaves\n";
	if (depthCopy)    definitions += "#define use_depth\n";
	if (blurRefl)     definitions += "#define blur_reflection\n";
	GLSLDefineConstf4(definitions, "SurfaceColor",   mapInfo->water.surfaceColor*0.4, mapInfo->water.surfaceAlpha );
	GLSLDefineConstf4(definitions, "PlaneColor",     mapInfo->water.planeColor*0.4, mapInfo->water.surfaceAlpha );
	GLSLDefineConstf3(definitions, "DiffuseColor",   mapInfo->water.diffuseColor );
	GLSLDefineConstf3(definitions, "SpecularColor",  mapInfo->water.specularColor );
	GLSLDefineConstf1(definitions, "SpecularPower",  mapInfo->water.specularPower );
	GLSLDefineConstf1(definitions, "SpecularFactor", mapInfo->water.specularFactor );
	GLSLDefineConstf1(definitions, "AmbientFactor",  mapInfo->water.ambientFactor );
	GLSLDefineConstf1(definitions, "DiffuseFactor",  mapInfo->water.diffuseFactor*15.0f );
	GLSLDefineConstf3(definitions, "SunDir",         mapInfo->light.sunDir );
	GLSLDefineConstf3(definitions, "MapMid",         float3(readmap->width*SQUARE_SIZE*0.5f,0.0f,readmap->height*SQUARE_SIZE*0.5f) );
	GLSLDefineConstf2(definitions, "ScreenInverse",  -1.0f/gu->viewSizeX, 1.0f/gu->viewSizeY );
	GLSLDefineConstf2(definitions, "ViewPos",        gu->viewPosX,gu->viewPosY );
	GLSLDefineConstf1(definitions, "FresnelMin",     mapInfo->water.fresnelMin);
	GLSLDefineConstf1(definitions, "FresnelMax",     mapInfo->water.fresnelMax);
	GLSLDefineConstf1(definitions, "FresnelPower",   mapInfo->water.fresnelPower);
	GLSLDefineConstf1(definitions, "ReflDistortion", mapInfo->water.reflDistortion);
	GLSLDefineConstf2(definitions, "BlurBase",       0.0f,mapInfo->water.blurBase/gu->viewSizeY);
	GLSLDefineConstf1(definitions, "BlurExponent",   mapInfo->water.blurExponent);
	GLSLDefineConstf1(definitions, "PerlinStartFreq",  mapInfo->water.perlinStartFreq);
	GLSLDefineConstf1(definitions, "PerlinLacunarity", mapInfo->water.perlinLacunarity);
	GLSLDefineConstf1(definitions, "PerlinAmp",        mapInfo->water.perlinAmplitude);

	/** LOAD SHADERS **/
	string vsSource = LoadShaderSource("shaders/bumpWaterVS.glsl");
	string fsSource = LoadShaderSource("shaders/bumpWaterFS.glsl");

	vector<GLint> lengths(2);
	vector<const GLchar*> strings(2);
	lengths[0] = definitions.length();
	strings[0] = definitions.c_str();

	waterVP = glCreateShader(GL_VERTEX_SHADER);
	lengths[1] = vsSource.length();
	strings[1] = vsSource.c_str();
	glShaderSource(waterVP, strings.size(), &strings.front(), &lengths.front());
	glCompileShader(waterVP);
	PrintShaderLog(waterVP);

	waterFP = glCreateShader(GL_FRAGMENT_SHADER);
	lengths[1] = fsSource.length();
	strings[1] = fsSource.c_str();
	glShaderSource(waterFP, strings.size(), &strings.front(), &lengths.front());
	glCompileShader(waterFP);
	PrintShaderLog(waterFP);

	waterShader = glCreateProgram();
	glAttachShader(waterShader, waterVP);
	glAttachShader(waterShader, waterFP);
	glLinkProgram(waterShader);
	PrintShaderLog(waterShader);


	/** BIND TEXTURE UNIFORMS **/
	glUseProgram(waterShader);
		eyePosLoc     = glGetUniformLocation(waterShader, "eyePos");
		frameLoc      = glGetUniformLocation(waterShader, "frame");

		GLuint normalmapLoc  = glGetUniformLocation(waterShader, "normalmap");
		GLuint heightmapLoc  = glGetUniformLocation(waterShader, "heightmap");
		GLuint causticLoc    = glGetUniformLocation(waterShader, "caustic");
		GLuint foamLoc       = glGetUniformLocation(waterShader, "foam");
		GLuint reflectionLoc = glGetUniformLocation(waterShader, "reflection");
		GLuint refractionLoc = glGetUniformLocation(waterShader, "refraction");
		GLuint depthmapLoc   = glGetUniformLocation(waterShader, "depthmap");
		GLuint coastmapLoc   = glGetUniformLocation(waterShader, "coastmap");
		GLuint waverandLoc   = glGetUniformLocation(waterShader, "waverand");

		glUniform1i(normalmapLoc, 0);
		glUniform1i(heightmapLoc, 1);
		glUniform1i(causticLoc, 2);
		glUniform1i(foamLoc, 3);
		glUniform1i(reflectionLoc, 4);
		glUniform1i(refractionLoc, 5);
		glUniform1i(coastmapLoc, 6);
		glUniform1i(depthmapLoc, 7);
		glUniform1i(waverandLoc, 8);
	glUseProgram(0);
}

CBumpWater::~CBumpWater()
{
	if (reflection)
		glDeleteTextures(1, &reflectTexture);
	if (refraction>0)
		glDeleteTextures(1, &refractTexture);
	if (depthCopy)
		glDeleteTextures(1, &depthTexture);

	if (reflectFBO) {
		glDeleteRenderbuffersEXT(1, &reflectRBO);
		glDeleteFramebuffersEXT(1,  &reflectFBO);
	}

	if (refractFBO) {
		glDeleteRenderbuffersEXT(1, &refractRBO);
		glDeleteFramebuffersEXT(1,  &refractFBO);
	}

	glDeleteTextures(1, &foamTexture);
	glDeleteTextures(1, &normalTexture);
	for (int i = 0; i < (int)caustTextures.size(); i++) {
		glDeleteTextures(1, &caustTextures[i]);
	}

	glDeleteShader(waterVP);
	glDeleteShader(waterFP);
	glDeleteProgram(waterShader);

	if (shorewaves) {
		glDeleteTextures(1, &coastTexture);
		glDeleteTextures(1, &waveRandTexture);

		if (coastFBO)
			glDeleteFramebuffersEXT(1,  &coastFBO);

		glDeleteShader(blurFP);
		glDeleteProgram(blurShader);
	}
}


void CBumpWater::GenerateCoastMap()
{
	/*GLfloat* normalmap = SAFE_NEW GLfloat[gs->mapx*gs->mapy*4];
	for (int y = 0; y < gs->mapy; y++) {
		for (int x = 0; x < gs->mapx; x++) {
			float3& normal = readmap->facenormals[(y*gs->mapx+x)*2];
			normalmap[(y*gs->mapx+x)*4]   = normal.x;
			normalmap[(y*gs->mapx+x)*4+1] = normal.y;
			normalmap[(y*gs->mapx+x)*4+2] = normal.z;
			normalmap[(y*gs->mapx+x)*4+3] = 1;
		}
	}*/

	const float* heightMap = readmap->GetHeightmap();
	GLfloat* coastmap = SAFE_NEW GLfloat[gs->mapx*gs->mapy*4];
	for (int y = 0; y < gs->mapy; y++) {
		for (int x = 0; x < gs->mapx; x++) {
			const float& height = heightMap[(y*(gs->mapx+1)+x)];
			coastmap[(y*gs->mapx+x)*4]   = (height>0.0f)?1:0;
			coastmap[(y*gs->mapx+x)*4+1] = (height>0.0f)?1:0;
			coastmap[(y*gs->mapx+x)*4+2] = (height>0.0f)?1:0;
			coastmap[(y*gs->mapx+x)*4+3] = height;
		}
	}
	
	glGenTextures(1, &coastTexture);
	glBindTexture(GL_TEXTURE_2D, coastTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, gs->mapx, gs->mapy, 0, GL_RGBA, GL_FLOAT, coastmap);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
	delete[] coastmap;

	GLuint coast2Texture;
	glGenTextures(1, &coast2Texture);
	glBindTexture(GL_TEXTURE_2D, coast2Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, gs->mapx, gs->mapy, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, coastFBO);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, coast2Texture, 0);
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		
	if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_BLEND);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glViewport(0,0,gs->mapx, gs->mapy);
		glUseProgram(blurShader);

		for (int i=0; i<10; ++i){
			glUniform2f(blurDirLoc,1.0f/gs->mapx,0.0f);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, coast2Texture, 0);

			glBindTexture(GL_TEXTURE_2D, coastTexture);
			glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex2f(0,0);
			glTexCoord2f(0,1); glVertex2f(0,1);
			glTexCoord2f(1,1); glVertex2f(1,1);
			glTexCoord2f(1,0); glVertex2f(1,0);
			glEnd();

			glUniform2f(blurDirLoc,0.0f,1.0f/gs->mapy);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, coastTexture, 0);

			glBindTexture(GL_TEXTURE_2D, coast2Texture);
			glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex2f(0,0);
			glTexCoord2f(0,1); glVertex2f(0,1);
			glTexCoord2f(1,1); glVertex2f(1,1);
			glTexCoord2f(1,0); glVertex2f(1,0);
			glEnd();
		}

		glUseProgram(0);
		glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
	}else PrintFboError("coast",status);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDeleteTextures(1, &coast2Texture);

	glBindTexture(GL_TEXTURE_2D, coastTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmapEXT(GL_TEXTURE_2D);
}


void CBumpWater::Draw()
{
	if(readmap->minheight>1)
		return;

	if (refraction == 1) {
		// _SCREENCOPY_ REFRACT TEXTURE
		glBindTexture(target, refractTexture);
		glEnable(target);
		glCopyTexSubImage2D(target, 0, 0, 0, gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
		glDisable(target);
	}

	if (depthCopy) {
		// _SCREENCOPY_ DEPTH TEXTURE
		glBindTexture(target, depthTexture);
		glEnable(target);
		glCopyTexSubImage2D(target, 0, 0, 0, gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
		glDisable(target);
	}

	glDisable(GL_ALPHA_TEST);
	if (refraction<2)
		glDepthMask(0);
	if (refraction>0)
		glDisable(GL_BLEND);

	const int causticTexNum = (gs->frameNum % caustTextures.size());
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, readmap->GetShadingTexture());
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, caustTextures[causticTexNum]);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, foamTexture);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, reflectTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(target,        refractTexture);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, coastTexture);
	glActiveTexture(GL_TEXTURE7); glBindTexture(target,        depthTexture);
	glActiveTexture(GL_TEXTURE8); glBindTexture(GL_TEXTURE_2D, waveRandTexture);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, normalTexture);

	glUseProgram(waterShader);
	glUniform1f(frameLoc, gs->frameNum / 15000.0f);
	glUniformf3(eyePosLoc, camera->pos);

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
	if (refraction>0)
		glEnable(GL_BLEND);
}

void CBumpWater::Update()
{
	if (readmap->minheight>1 || mapInfo->map.voidWater)
		return;

	float3 w = wind.GetCurrentWind();
	texcoord1.x += w.x*0.001f;
	texcoord1.y += w.z*0.001f;
/*
	texcoord1.x += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord2.x += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord3.x += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord4.x += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord1.y += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord2.y += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord3.y += (gu->usRandFloat()-0.5f)*0.0002f;
	texcoord4.y += (gu->usRandFloat()-0.5f)*0.0002f;
*/
}

void CBumpWater::UpdateWater(CGame* game)
{
	if (readmap->minheight>1 || mapInfo->map.voidWater)
		return;

	if (refraction>1) DrawRefraction(game);
	if (reflection)   DrawReflection(game);
}

void CBumpWater::DrawRefraction(CGame* game)
{
	// _RENDER_ REFRACTION TEXTURE
	if (refractFBO)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, refractFBO);

	camera->Update(false);
	glViewport(0,0,screenTextureX,screenTextureY);

	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_FOG);

	float3 oldsun=unitDrawer->unitSunColor;
	float3 oldambient=unitDrawer->unitAmbientColor;

	unitDrawer->unitSunColor*=float3(0.5f,0.7f,0.9f);
	unitDrawer->unitAmbientColor*=float3(0.6f,0.8f,1.0f);

	game->SetDrawMode(CGame::refractionDraw);
	drawRefraction=true;

	glEnable(GL_CLIP_PLANE2);
	static double plane[4]={0,-1,0,5};
	glClipPlane(GL_CLIP_PLANE2 ,plane);

	readmap->GetGroundDrawer()->Draw();
	unitDrawer->Draw(false,true);
	featureHandler->Draw();
	drawReflection=false;
	ph->Draw(false,true);
	luaCallIns.DrawWorldRefraction();

	glDisable(GL_CLIP_PLANE2);
	game->SetDrawMode(CGame::normalDraw);
	drawRefraction=false;

	if (refractFBO) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}else{
		glBindTexture(target, refractTexture);
		glEnable(target);
		glCopyTexSubImage2D(target,0,0,0,0,0,screenTextureX,screenTextureY);
		glDisable(target);
	}

	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);
	glEnable(GL_FOG);

	unitDrawer->unitSunColor=oldsun;
	unitDrawer->unitAmbientColor=oldambient;
}

void CBumpWater::DrawReflection(CGame* game)
{
	// CREATE REFLECTION TEXTURE
	if (reflectFBO)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, reflectFBO);

	CCamera *realCam = camera;
	camera = new CCamera(*realCam);
	camera->up.x=0;
	camera->up.y=1;
	camera->up.z=0;
	camera->forward.y*=-1;
	camera->pos.y*=-1;
	camera->Update(false);

	glViewport(0,0,reflTexSize,reflTexSize);
	glClear(GL_DEPTH_BUFFER_BIT);

	game->SetDrawMode(CGame::reflectionDraw);
	sky->Draw();

	glEnable(GL_CLIP_PLANE2);
	static double plane[4]={0,1,0,1};
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

	if (reflectFBO) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}else{
		glBindTexture(GL_TEXTURE_2D, reflectTexture);
		glEnable(GL_TEXTURE_2D);
		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,reflTexSize, reflTexSize);
		glDisable(GL_TEXTURE_2D);
	}

	glViewport(gu->viewPosX,0,gu->viewSizeX,gu->viewSizeY);

	delete camera;
	camera = realCam;
	camera->Update(false);
}
