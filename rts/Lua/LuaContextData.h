/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONTEXT_DATA_H
#define LUA_CONTEXT_DATA_H

#include <map>

#include "LuaShaders.h"
#include "LuaTextures.h"
#include "LuaFBOs.h"
#include "LuaRBOs.h"
#include "LuaDisplayLists.h"
#include "System/EventClient.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringMutex.h"

class CLuaHandle;


//FIXME move to diff file
struct GLMatrixStateTracker {
public:
	MatrixStateData matrixData; // [>0] = stack depth for mode, [0] = matrix mode
	bool listMode; // if creating display list

public:
	GLMatrixStateTracker() : listMode(false) {}

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

	unsigned int GetMode() const {
		MatrixStateData::const_iterator i = matrixData.find(0);
		return (i == matrixData.end()) ? GL_MODELVIEW : i->second;
	}

	int GetDepth(unsigned int mode) const {
		MatrixStateData::const_iterator i = matrixData.find(mode);
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

	const MatrixStateData& GetMatrixState() const {
		return matrixData;
	}

	bool HasMatrixStateError() const {
		return !matrixData.empty();
	}

	void HandleMatrixStateError(int error, const char* errsrc) {
		unsigned int mode = GetMode();

		// dont complain about stack/mode issues if some other error occurred
		// check if the lua code did not restore the matrix mode
		if (error == 0 && mode != GL_MODELVIEW)
			LOG_L(L_ERROR, "%s: OpenGL state check error, matrix mode = %d, please restore mode to GL.MODELVIEW before end", errsrc, mode);

		for (MatrixStateData::iterator i = matrixData.begin(); i != matrixData.end(); ++i) {
			if (i->first == 0)
				continue;
			// check if the lua code fucked up the stack for matrix mode X
			if (error == 0)
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




struct luaContextData {
	luaContextData()
	: owner(NULL)
	, luamutex(NULL)

	, synced(false)
	, allowChanges(false)
	, drawingEnabled(false)

	, running(0)
	, curAllocedBytes(0)
	, maxAllocedBytes(0)

	, fullCtrl(false)
	, fullRead(false)

	, ctrlTeam(CEventClient::NoAccessTeam)
	, readTeam(0)
	, readAllyTeam(0)
	, selectTeam(CEventClient::NoAccessTeam) {}

	CLuaHandle* owner;
	spring::recursive_mutex* luamutex;

	bool synced;
	bool allowChanges;
	bool drawingEnabled;

	int running; //< is currently running? (0: not running; >0: is running)

	unsigned int curAllocedBytes;
	unsigned int maxAllocedBytes;

	// permission rights
	bool fullCtrl;
	bool fullRead;

	int  ctrlTeam;
	int  readTeam;
	int  readAllyTeam;
	int  selectTeam;

	LuaShaders shaders;
	LuaTextures textures;
	LuaFBOs fbos;
	LuaRBOs rbos;
	CLuaDisplayLists displayLists;

	GLMatrixStateTracker glMatrixTracker;
};


#endif // LUA_CONTEXT_DATA_H
