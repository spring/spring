/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_MATRIX_STATE_TRACKER_H
#define GL_MATRIX_STATE_TRACKER_H

#include "Rendering/GL/myGL.h"
#include "System/Log/ILog.h"

struct SMatrixStateData {
	SMatrixStateData(): mode(GL_MODELVIEW), 
						modelView(0), 
						projection(0),
						texture(0) {}
	int mode;
	int modelView;
	int projection;
	int texture;
};

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

	int& GetDepth(unsigned int mode) {
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
		int& depth = GetDepth(mode);
		if (!listMode && depth >= 255)
			return false;
		depth += 1;
		return true;
	}

	bool PopMatrix() {
		unsigned int mode = GetMode();
		int& depth = GetDepth(mode);
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
			GL::MatrixMode(glMode); \
			for (int p = 0; p < matrixData.modeName; ++p) { \
				GL::PopMatrix(); \
			} \
			matrixData.modeName = 0;\
		}

		CHECK_MODE(modelView, GL_MODELVIEW)
		CHECK_MODE(projection, GL_PROJECTION)
		CHECK_MODE(texture, GL_TEXTURE)

#undef CHECK_MODE

		GL::MatrixMode(GL_MODELVIEW);
	}
};

#endif

