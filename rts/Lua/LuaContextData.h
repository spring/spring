/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_CONTEXT_DATA_H
#define LUA_CONTEXT_DATA_H

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
	SMatrixStateData matrixData; // [>0] = stack depth for mode, [0] = matrix mode
	bool listMode; // if creating display list

public:
	GLMatrixStateTracker() : listMode(false) {}

	SMatrixStateData PushMatrixState() {
		SMatrixStateData md;
		std::swap(matrixData, md);
		return md;
	}

	SMatrixStateData PushMatrixState(bool lm) {
		listMode = lm;
		return PushMatrixState();
	}

	void PopMatrixState(SMatrixStateData& md) {
		std::swap(matrixData, md);
	}

	void PopMatrixState(SMatrixStateData& md, bool lm) {
		listMode = lm;
		PopMatrixState(md);
	}

	unsigned int GetMode() const {
		return matrixData.mode;
	}

	int &GetDepth(unsigned int mode) {
        switch (mode) {
            case GL_MODELVIEW: return matrixData.modelView;
            case GL_PROJECTION: return matrixData.projection;
            case GL_TEXTURE: return matrixData.texture;
            default:
                LOG_L(L_ERROR, "unknown matrix mode = %u", mode);
                abort();
                break;
        }
    }

	bool PushMatrix() {
		unsigned int mode = GetMode();
		int &depth = GetDepth(mode);
		if (!listMode && depth >= 255)
			return false;
		depth += 1;
		return true;
	}

	bool PopMatrix() {
		unsigned int mode = GetMode();
		int &depth = GetDepth(mode);
		if (listMode) {
			depth -= 1;
			return true;
		}
		if (depth == 0)
			return false;
		depth -= 1;
		return true;
	}

	bool SetMatrixMode(int mode) {
		if (mode == GL_MODELVIEW || mode == GL_PROJECTION || mode == GL_TEXTURE) {
			matrixData.mode = mode;
			return true;
		}
		return false;
	}

	
	int ApplyMatrixState(SMatrixStateData& m) {
		// validate
#define VALIDATE(modeName) \
		{ \
			int newDepth = m.modeName + matrixData.modeName; \
			if (newDepth < 0) \
				return -1; \
			if (newDepth >= 255) \
				return 1; \
		}
			
		VALIDATE(modelView)
		VALIDATE(projection)
		VALIDATE(texture)
		
#undef VALIDATE
		
		// apply
		matrixData.mode = m.mode;
		matrixData.modelView += m.modelView;
		matrixData.projection += m.projection;
		matrixData.texture += m.texture;
		
		return 0;
	}

	const SMatrixStateData& GetMatrixState() const {
		return matrixData;
	}

	bool HasMatrixStateError() const {
		return matrixData.mode != GL_MODELVIEW ||
			matrixData.modelView != 0 ||
			matrixData.projection != 0 ||
			matrixData.texture != 0;
	}

	void HandleMatrixStateError(int error, const char* errsrc) {
		unsigned int mode = GetMode();

		// dont complain about stack/mode issues if some other error occurred
		// check if the lua code did not restore the matrix mode
		if (error == 0 && mode != GL_MODELVIEW)
			LOG_L(L_ERROR, "%s: OpenGL state check error, matrix mode = %d, please restore mode to GL.MODELVIEW before end", errsrc, mode);

		
#define CHECK_MODE(modeName, glMode) \
		assert(matrixData.modeName >= 0); \
		if (matrixData.modeName != 0) {\
			if (error == 0){ \
				LOG_L(L_ERROR, "%s: OpenGL stack check error, matrix mode = %s, depth = %d, please make sure to pop all matrices before end", errsrc, #glMode, matrixData.modeName); \
			} \
			glMatrixMode(glMode); \
			for (int p = 0; p < matrixData.modeName; ++p) { \
				glPopMatrix(); \
			} \
			matrixData.modeName = 0;\
		}
			
		CHECK_MODE(modelView, GL_MODELVIEW)
		CHECK_MODE(projection, GL_PROJECTION)
		CHECK_MODE(texture, GL_TEXTURE)
		
#undef CHECK_MODE
		
		glMatrixMode(GL_MODELVIEW);
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
