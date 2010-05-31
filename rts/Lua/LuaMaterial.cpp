/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include <string>
#include <vector>
#include <cstring>

using std::string;
using std::vector;

#include "LuaMaterial.h"

#include "LuaHashString.h"
#include "LuaHandle.h"
#include "LuaOpenGL.h"

#include "Game/Camera.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Sim/Units/Unit.h"
#include "LogOutput.h"
#include "Util.h"


LuaMatHandler LuaMatHandler::handler;
LuaMatHandler& luaMatHandler = LuaMatHandler::handler;


#define LOGPRINTF logOutput.Print

#define STRING_CASE(ptr, x) case x: ptr = #x; break;


/******************************************************************************/
/******************************************************************************/
//
//  LuaUnitUniforms
//

void LuaUnitUniforms::Execute(CUnit* unit) const
{
	//FIXME use vertex attributes
	if (!haveUniforms) {
		return;
	}
	if (speedLoc >= 0) {
		glUniformf3(speedLoc, unit->speed);
	}
	if (healthLoc >= 0) {
		glUniform1f(healthLoc, unit->health / unit->maxHealth);
	}
	if (unitIDLoc >= 0) {
		glUniform1i(unitIDLoc, unit->id);
	}
	if (teamIDLoc >= 0) {
		glUniform1i(teamIDLoc, unit->id);
	}
	if (customLoc >= 0) {
		if (customCount > 0) {
			glUniform1fv(customLoc, customCount, customData);
		}
	}
}


void LuaUnitUniforms::SetCustomCount(int count)
{
	customCount = count;
	delete[] customData;
	if (count > 0) {
		customData = new float[count];
		memset(customData, 0, customCount * sizeof(GLfloat));
	} else {
		customData = NULL;
	}
}


LuaUnitUniforms& LuaUnitUniforms::operator=(const LuaUnitUniforms& u)
{
	delete[] customData;
	customData = NULL;

	haveUniforms = u.haveUniforms;
	speedLoc     = u.speedLoc;
	healthLoc    = u.healthLoc;
	unitIDLoc    = u.unitIDLoc;
	teamIDLoc    = u.teamIDLoc;
	customLoc    = u.customLoc;
	customCount  = u.customCount;

	if (customCount > 0) {
		customData = new GLfloat[customCount];
		memcpy(customData, u.customData, customCount * sizeof(GLfloat));
	}

	return *this;
}


LuaUnitUniforms::LuaUnitUniforms(const LuaUnitUniforms& u)
{
	customData = NULL;
	*this = u;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaUnitMaterial
//

bool LuaUnitMaterial::SetLODCount(unsigned int count)
{
	lodCount = count;
	lastLOD = lodCount - 1;
	lodMats.resize(count);
	return true;
}


bool LuaUnitMaterial::SetLastLOD(unsigned int lod)
{
	lastLOD = std::min(lod, lodCount - 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatShader
//

void LuaMatShader::Finalize()
{
	if (type != LUASHADER_GL) {
		openglID = 0;
	}
}


int LuaMatShader::Compare(const LuaMatShader& a, const LuaMatShader& b)
{
	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;
	}
	if (a.type == LUASHADER_GL) {
		if (a.openglID != b.openglID) {
			return (a.openglID < b.openglID) ? -1 : +1;
		}
	}
	return 0; // LUASHADER_NONE and LUASHADER_SHADOWMAP ignore openglID
}


void LuaMatShader::Execute(const LuaMatShader& prev) const
{
	if (type != prev.type) {
		if (prev.type == LUASHADER_GL) {
			glUseProgram(0);
		}
		else if (prev.type == LUASHADER_3DO) {
			if (luaMatHandler.reset3doShader) { luaMatHandler.reset3doShader(); }
		}
		else if (prev.type == LUASHADER_S3O) {
			if (luaMatHandler.resetS3oShader) { luaMatHandler.resetS3oShader(); }
		}

		if (type == LUASHADER_GL) {
			glUseProgram(openglID);
		}
		else if (type == LUASHADER_3DO) {
			if (luaMatHandler.setup3doShader) { luaMatHandler.setup3doShader(); }
		}
		else if (type == LUASHADER_S3O) {
			if (luaMatHandler.setupS3oShader) { luaMatHandler.setupS3oShader(); }
		}
	}
	else if (type == LUASHADER_GL) {
		if (openglID != prev.openglID) {
			glUseProgram(openglID);
		}
	}
}


void LuaMatShader::Print(const string& indent) const
{
	const char* typeName = "Unknown";
	switch (type) {
		STRING_CASE(typeName, LUASHADER_NONE);
		STRING_CASE(typeName, LUASHADER_GL);
		STRING_CASE(typeName, LUASHADER_3DO);
		STRING_CASE(typeName, LUASHADER_S3O);
	}
	LOGPRINTF("%s%s %i\n", indent.c_str(), typeName, openglID);
}

/******************************************************************************/
/******************************************************************************/
//
//  LuaMatTexture
//

void LuaMatTexture::Finalize()
{
	for (int t = 0; t < maxTexUnits; t++) {
		if ((type == LUATEX_GL) && (openglID == 0)) {
			type = LUATEX_NONE;
		}
		if (type == LUATEX_NONE) {
			enable = false;
		}
		if (type != LUATEX_GL) {
			openglID = 0;
		}
	}
}


int LuaMatTexture::Compare(const LuaMatTexture& a, const LuaMatTexture& b)
{
	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;
	}
	if (a.type == LUATEX_GL) {
		if (a.openglID != b.openglID) {
			return (a.openglID < b.openglID) ? -1 : +1;
		}
	}
	if (a.enable != b.enable) {
		return a.enable ? -1 : +1;
	}
	return 0; // LUATEX_NONE and LUATEX_SHADOWMAP ignore openglID
}


void LuaMatTexture::Bind(const LuaMatTexture& prev) const
{
	// blunt force -- poor form
	prev.Unbind();

	if (type == LUATEX_GL) {
		glBindTexture(GL_TEXTURE_2D, openglID);
		if (enable) {
			glEnable(GL_TEXTURE_2D);
		}
	}
	else if (type == LUATEX_SHADOWMAP) {
		glBindTexture(GL_TEXTURE_2D, shadowHandler->shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		if (enable) {
			glEnable(GL_TEXTURE_2D);
		}
	}
	else if (type == LUATEX_REFLECTION) {
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetEnvReflectionTextureID());
		if (enable) {
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		}
	}
	else if (type == LUATEX_SPECULAR) {
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cubeMapHandler->GetSpecularTextureID());
		if (enable) {
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		}
	}
}


void LuaMatTexture::Unbind() const
{
	if (type == LUATEX_NONE || (!enable && (type != LUATEX_SHADOWMAP)))
        return;

	if (type == LUATEX_GL) {
		glDisable(GL_TEXTURE_2D);
	}
	else if (type == LUATEX_SHADOWMAP) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
		glDisable(GL_TEXTURE_2D);
	}
	else if (type == LUATEX_REFLECTION) {
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	}
	else if (type == LUATEX_SPECULAR) {
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	}
}


void LuaMatTexture::Print(const string& indent) const
{
	const char* typeName = "Unknown";
	switch (type) {
		STRING_CASE(typeName, LUATEX_NONE);
		STRING_CASE(typeName, LUATEX_GL);
		STRING_CASE(typeName, LUATEX_SHADOWMAP);
		STRING_CASE(typeName, LUATEX_REFLECTION);
		STRING_CASE(typeName, LUATEX_SPECULAR);
	}
	LOGPRINTF("%s%s %i %s\n", indent.c_str(),
	       typeName, openglID, enable ? "true" : "false");
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMaterial
//

const LuaMaterial LuaMaterial::defMat;


void LuaMaterial::Finalize()
{
	shader.Finalize();

	texCount = 0;
	for (int t = 0; t < LuaMatTexture::maxTexUnits; t++) {
		LuaMatTexture& tex = textures[t];
		tex.Finalize();
		if (tex.type != LuaMatTexture::LUATEX_NONE) {
			texCount = (t + 1);
		}
	}
}


void LuaMaterial::Execute(const LuaMaterial& prev) const
{
	if (prev.postList != 0) {
		glCallList(prev.postList);
	}
	if (preList != 0) {
		glCallList(preList);
	}

	shader.Execute(prev.shader);

	if (cameraLoc >= 0) {
		// FIXME: this is happening too much, just use floats?
		GLfloat array[16];
		const GLdouble* viewMat = camera->GetViewMat(); // GetMatrixData("camera")
		for (int i = 0; i < 16; i += 4) {
			array[i    ] = (GLfloat) viewMat[i    ];
			array[i + 1] = (GLfloat) viewMat[i + 1];
			array[i + 2] = (GLfloat) viewMat[i + 2];
			array[i + 3] = (GLfloat) viewMat[i + 3];
		}
		glUniformMatrix4fv(cameraLoc, 1, GL_FALSE, array);
	}
	if (cameraInvLoc >= 0) {
		GLfloat array[16];
		const GLdouble* viewMatInv = camera->GetViewMatInv(); // GetMatrixData("caminv")
		for (int i = 0; i < 16; i += 4) {
			array[i    ] = (GLfloat) viewMatInv[i    ];
			array[i + 1] = (GLfloat) viewMatInv[i + 1];
			array[i + 2] = (GLfloat) viewMatInv[i + 2];
			array[i + 3] = (GLfloat) viewMatInv[i + 3];
		}
		glUniformMatrix4fv(cameraInvLoc, 1, GL_FALSE, array);
	}

	if (cameraPosLoc >= 0) {
		glUniformf3(cameraPosLoc, camera->pos);
	}

	if (shadowLoc >= 0) {
		glUniformMatrix4fv(shadowLoc, 1, GL_FALSE, shadowHandler->shadowMatrix.m);
	}

	if (shadowParamsLoc >= 0) {
		glUniform4fv(shadowParamsLoc, 1, const_cast<float*>(&(shadowHandler->GetShadowParams().x)));
	}

	const int maxTex = std::max(texCount, prev.texCount);
	for (int t = (maxTex - 1); t >= 0; t--) {
		glActiveTexture(GL_TEXTURE0 + t);
		textures[t].Bind(prev.textures[t]);
	}

	if (useCamera != prev.useCamera) {
		if (useCamera) {
			glPopMatrix();
		} else {
			glPushMatrix();
			glLoadIdentity();
		}
	}

	if (culling != prev.culling) {
		if (culling) {
			glEnable(GL_CULL_FACE);
			glCullFace(culling);
		} else {
			glDisable(GL_CULL_FACE);
		}
	}
}


int LuaMaterial::Compare(const LuaMaterial& a, const LuaMaterial& b)
{
	// NOTE: the order of the comparisons is important
	int cmp;

	if (a.type != b.type) {
		return (a.type < b.type) ? -1 : +1;  // should not happen
	}

	if (a.order != b.order) {
		return (a.order < b.order) ? -1 : +1;
	}

	cmp = LuaMatShader::Compare(a.shader, b.shader);
	if (cmp != 0) {
		return cmp;
	}

	const int maxTex = std::min(a.texCount, b.texCount);
	for (int t = 0; t < maxTex; t++) {
		cmp = LuaMatTexture::Compare(a.textures[t], b.textures[t]);
		if (cmp != 0) {
			return cmp;
		}
	}
	if (a.texCount != b.texCount) {
		return (a.texCount < b.texCount) ? -1 : +1;
	}

	if (a.preList != b.preList) {
		return (a.preList < b.preList) ? -1 : +1;
	}

	if (a.postList != b.postList) {
		return (a.postList < b.postList) ? -1 : +1;
	}

	if (a.useCamera != b.useCamera) {
		return a.useCamera ? -1 : +1;
	}

	if (a.culling != b.culling) {
		return (a.culling < b.culling) ? -1 : +1;
	}

	if (a.cameraLoc != b.cameraLoc) {
		return (a.cameraLoc < b.cameraLoc) ? -1 : +1;
	}
	if (a.cameraInvLoc != b.cameraInvLoc) {
		return (a.cameraInvLoc < b.cameraInvLoc) ? -1 : +1;
	}

	if (a.cameraPosLoc != b.cameraPosLoc) {
		return (a.cameraPosLoc < b.cameraPosLoc) ? -1 : +1;
	}

	if (a.shadowLoc != b.shadowLoc) {
		return (a.shadowLoc < b.shadowLoc) ? -1 : +1;
	}

	if (a.shadowParamsLoc != b.shadowParamsLoc) {
		return (a.shadowParamsLoc < b.shadowParamsLoc) ? -1 : +1;
	}

	return 0;
}


static const char* GetMatTypeName(LuaMatType type)
{
	const char* typeName = "Unknown";
	switch (type) {
		STRING_CASE(typeName, LUAMAT_ALPHA);
		STRING_CASE(typeName, LUAMAT_OPAQUE);
		STRING_CASE(typeName, LUAMAT_ALPHA_REFLECT);
		STRING_CASE(typeName, LUAMAT_OPAQUE_REFLECT);
		STRING_CASE(typeName, LUAMAT_SHADOW);
		case LUAMAT_TYPE_COUNT: break;
	}
	return typeName;
}


void LuaMaterial::Print(const string& indent) const
{
#define CULL_TO_STR(x) \
	(x==GL_FRONT) ? "front" : (x==GL_BACK) ? "back" : (x!=0) ? "false" : "unknown"

	LOGPRINTF("%s%s\n", indent.c_str(), GetMatTypeName(type));
	LOGPRINTF("%sorder = %i\n", indent.c_str(), order);
	shader.Print(indent);
	LOGPRINTF("%stexCount = %i\n", indent.c_str(), texCount);
	for (int t = 0; t < texCount; t++) {
		char buf[32];
		SNPRINTF(buf, sizeof(buf), "%s  tex[%i] ", indent.c_str(), t);
		textures[t].Print(buf);
	}
	LOGPRINTF("%spreList  = %i\n",  indent.c_str(), preList);
	LOGPRINTF("%spostList = %i\n",  indent.c_str(), postList);
	LOGPRINTF("%suseCamera = %s\n", indent.c_str(), useCamera ? "true" : "false");
	LOGPRINTF("%sculling = %s\n", indent.c_str(), CULL_TO_STR(culling));
	LOGPRINTF("%scameraLoc = %i\n", indent.c_str(), cameraLoc);
	LOGPRINTF("%scameraInvLoc = %i\n", indent.c_str(), cameraInvLoc);
	LOGPRINTF("%scameraPosLoc = %i\n", indent.c_str(), cameraPosLoc);
	LOGPRINTF("%sshadowLoc = %i\n", indent.c_str(), shadowLoc);
	LOGPRINTF("%sshadowParamsLoc = %i\n", indent.c_str(), shadowParamsLoc);
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatRef
//

LuaMatRef::LuaMatRef(LuaMatBin* _bin)
{
	bin = _bin;
	if (bin != NULL) {
		bin->Ref();
	}
}


LuaMatRef::~LuaMatRef()
{
	if (bin != NULL) {
		bin->UnRef();
	}
}


void LuaMatRef::Reset()
{
	if (bin != NULL) {
		bin->UnRef();
	}
	bin = NULL;
}


LuaMatRef& LuaMatRef::operator=(const LuaMatRef& mr)
{
	if (bin != mr.bin) {
		if (bin != NULL) { bin->UnRef(); }
		bin = mr.bin;
		if (bin != NULL) { bin->Ref(); }
	}
	return *this;
}


LuaMatRef::LuaMatRef(const LuaMatRef& mr)
{
	bin = mr.bin;
	if (bin != NULL) {
		bin->Ref();
	}
}


void LuaMatRef::AddUnit(CUnit* unit)
{
  if (bin != NULL) {
    bin->AddUnit(unit);
  }
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatBin
//

void LuaMatBin::Ref()
{
	refCount++;
}


void LuaMatBin::UnRef()
{
	refCount--;
	if (refCount <= 0) {
		luaMatHandler.FreeBin(this);
	}
}


void LuaMatBin::Print(const string& indent) const
{
	LOGPRINTF("%sunitCount = %i\n", indent.c_str(), (int)units.size());
	LOGPRINTF("%spointer = %p\n", indent.c_str(), this);
	LuaMaterial::Print(indent + "  ");
}


/******************************************************************************/
/******************************************************************************/
//
//  LuaMatHandler
//

LuaMatHandler::LuaMatHandler()
{
	setup3doShader = NULL;
	reset3doShader = NULL;
	setupS3oShader = NULL;
	resetS3oShader = NULL;
}


LuaMatHandler::~LuaMatHandler()
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		LuaMatBinSet& binSet = binTypes[LuaMatType(m)];
		LuaMatBinSet::iterator it;
		for (it = binSet.begin(); it != binSet.end(); ++it) {
			delete *it;
		}
	}
}


LuaMatRef LuaMatHandler::GetRef(const LuaMaterial& mat)
{
	if ((mat.type < 0) || (mat.type >= LUAMAT_TYPE_COUNT)) {
		logOutput.Print("ERROR: LuaMatHandler::GetRef() untyped material\n");
		return LuaMatRef();
	}
	LuaMatBinSet& binSet = binTypes[mat.type];

	LuaMatBin* fakeBin = (LuaMatBin*) &mat;
	LuaMatBinSet::iterator it = binSet.find(fakeBin);
	if (it != binSet.end()) {
		return LuaMatRef(*it);
	}

	LuaMatBin* bin = new LuaMatBin(mat);
	binSet.insert(bin);
	return LuaMatRef(bin);
}


void LuaMatHandler::ClearBins(LuaMatType type)
{
	if ((type < 0) || (type >= LUAMAT_TYPE_COUNT)) {
		return;
	}
	LuaMatBinSet& binSet = binTypes[type];
	LuaMatBinSet::iterator it;
	for (it = binSet.begin(); it != binSet.end(); ++it) {
		LuaMatBin* bin = *it;
		bin->Clear();
	}
}


void LuaMatHandler::ClearBins()
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		ClearBins(LuaMatType(m));
	}
}


void LuaMatHandler::FreeBin(LuaMatBin* bin)
{
	LuaMatBinSet& binSet = binTypes[bin->type];
	LuaMatBinSet::iterator it = binSet.find(bin);
	if (it != binSet.end()) {
		if (*it != bin) {
			logOutput.Print("ERROR: LuaMatHandler::FreeBin pointer mismatch\n");
		}
		delete bin;
		binSet.erase(it);
	}
}


void LuaMatHandler::PrintBins(const string& indent, LuaMatType type) const
{
	if ((type < 0) || (type >= LUAMAT_TYPE_COUNT)) {
		return;
	}
	const LuaMatBinSet& binSet = binTypes[type];
	LuaMatBinSet::const_iterator it;
	int num = 0;
	LOGPRINTF("%sBINCOUNT = "_STPF_"\n", indent.c_str(), binSet.size());
	for (it = binSet.begin(); it != binSet.end(); ++it) {
		LuaMatBin* bin = *it;
		LOGPRINTF("%sBIN %i:\n", indent.c_str(), num);
		bin->Print(indent + "    ");
		num++;
	}
}


void LuaMatHandler::PrintAllBins(const string& indent) const
{
	for (int m = 0; m < LUAMAT_TYPE_COUNT; m++) {
		string newIndent = indent + GetMatTypeName(LuaMatType(m));
		newIndent += "  ";
		PrintBins(newIndent, LuaMatType(m));
	}
}


/******************************************************************************/
/******************************************************************************/
