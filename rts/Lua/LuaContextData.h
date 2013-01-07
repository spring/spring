/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONTEXT_DATA_H
#define LUA_CONTEXT_DATA_H

#include <map>

#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
//FIXME#include "LuaVBOs.h"
#include "LuaDisplayLists.h"
#include "System/EventClient.h"
#include "System/Log/ILog.h"

class CLuaHandle;

struct luaContextData {
	luaContextData() : fullCtrl(false), fullRead(false), ctrlTeam(CEventClient::NoAccessTeam),
		readTeam(0), readAllyTeam(0), selectTeam(CEventClient::NoAccessTeam), synced(false),
		owner(NULL), drawingEnabled(false), running(0), listMode(false) {}
	bool fullCtrl;
	bool fullRead;
	int  ctrlTeam;
	int  readTeam;
	int  readAllyTeam;
	int  selectTeam;
	//FIXME		LuaArrays arrays;
	LuaShaders shaders;
	LuaTextures textures;
	//FIXME		LuaVBOs vbos;
	LuaFBOs fbos;
	LuaRBOs rbos;
	CLuaDisplayLists displayLists;
	bool synced;
	CLuaHandle *owner;
	bool drawingEnabled;
	int running; //< is currently running? (0: not running; >0: is running)
	MatrixStateData matrixData; // [>0] = stack depth for mode, [0] = matrix mode
	bool listMode; // if creating display list

	MatrixStateData PushMatrixState() {
		MatrixStateData md;
		matrixData.swap(md);
		return md;
	}

	MatrixStateData PushMatrixState(bool lm) {
		listMode = lm;
		return PushMatrixState();
	}

	void PopMatrixState(MatrixStateData& md) {
		matrixData.swap(md);
	}

	void PopMatrixState(MatrixStateData& md, bool lm) {
		listMode = lm;
		PopMatrixState(md);
	}

	unsigned int GetMode() {
		MatrixStateData::iterator i = matrixData.find(0);
		return (i == matrixData.end()) ? GL_MODELVIEW : i->second;
	}

	int GetDepth(unsigned int mode) {
		MatrixStateData::iterator i = matrixData.find(mode);
		return (i == matrixData.end()) ? 0 : i->second;
	}

	bool PushMatrix() {
		unsigned int mode = GetMode();
		int depth = GetDepth(mode);
		if (!listMode && depth >= 255)
			return false;
		matrixData[mode] = depth + 1;
		return true;
	}

	bool PopMatrix() {
		unsigned int mode = GetMode();
		int depth = GetDepth(mode);
		if (listMode) {
			matrixData[mode] = depth - 1;
			return true;
		}
		if (depth == 0)
			return false;
		if (depth == 1)
			matrixData.erase(mode);
		else
			matrixData[mode] = depth - 1;
		return true;
	}

	bool SetMatrixMode(int mode) {
		if (mode <= 0)
			return false;
		if (!listMode && mode == GL_MODELVIEW)
			matrixData.erase(0);
		else
			matrixData[0] = mode;
		return true;
	}

	int ApplyMatrixState(MatrixStateData& m) {
		// validate
		for (MatrixStateData::iterator i = m.begin(); i != m.end(); ++i) {
			if (i->first == 0)
				continue;
			int newdepth = GetDepth(i->first) + i->second;
			if (newdepth < 0)
				return -1;
			if (newdepth >= 255)
				return 1;
		}
		// apply
		for (MatrixStateData::iterator i = m.begin(); i != m.end(); ++i) {
			if (i->first == 0) {
				if (i->second == GL_MODELVIEW)
					matrixData.erase(0);
				else
					matrixData[0] = i->second;
				continue;
			}
			int newdepth = GetDepth(i->first) + i->second;
			if (newdepth == 0)
				matrixData.erase(i->first);
			else
				matrixData[i->first] = newdepth;
		}
		return 0;
	}

	MatrixStateData GetMatrixState() {
		return matrixData;
	}

	bool HasMatrixStateError() {
		return !matrixData.empty();
	}

	void HandleMatrixStateError(int error, const char* errsrc) {
		unsigned int mode = GetMode();
		// dont complain about stack/mode issues if some other error occurred
		if (error == 0 && mode != GL_MODELVIEW) // check if the lua code did not restore the matrix mode
			LOG_L(L_ERROR, "%s: OpenGL state check error, matrix mode = %d, please restore mode to GL.MODELVIEW before end", errsrc, mode);
		for (MatrixStateData::iterator i = matrixData.begin(); i != matrixData.end(); ++i) {
			if (i->first == 0)
				continue;
			if (error == 0) // check if the lua code fucked up the stack for matrix mode X
				LOG_L(L_ERROR, "%s: OpenGL stack check error, matrix mode = %d, depth = %d, please make sure to pop all matrices before end", errsrc, i->first, i->second);
			glMatrixMode(i->first);
			for (int p = 0; p < i->second; ++p) {
				glPopMatrix();
			}
		}
		glMatrixMode(GL_MODELVIEW);
		matrixData.clear();
	}
};

#endif // LUA_CONTEXT_DATA_H