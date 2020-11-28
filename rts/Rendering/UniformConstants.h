#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <array>
#include <cstdint>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "System/Log/ILog.h"

struct UniformMatricesBuffer {
	CMatrix44f screenView;
	CMatrix44f screenProj;
	CMatrix44f screenViewProj;

	CMatrix44f cameraView;
	CMatrix44f cameraProj;
	CMatrix44f cameraViewProj;
	CMatrix44f cameraBillboard;

	CMatrix44f cameraViewInv;
	CMatrix44f cameraProjInv;
	CMatrix44f cameraViewProjInv;

	CMatrix44f shadowView;
	CMatrix44f shadowProj;
	CMatrix44f shadowViewProj;

	//TODO: minimap matrices
};

struct UniformParamsBuffer {
	float4 timeInfo; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	float4 viewGeometry; //vsx, vsy, vpx, vpy
	float4 mapSize; //xz, xzPO2

	float4 rndVec3; //new every draw frame. Only xyz are initialized
};

class UniformConstants {
public:
	static UniformConstants& GetInstance() {
		static UniformConstants uniformConstantsInstance;
		return uniformConstantsInstance;
	};
	static bool Supported() {
		static bool supported = GLEW_ARB_uniform_buffer_object && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
		return supported;
	}
public:
	void Init();
	void Kill();
	void Update();
	void Bind();
private:

	template<typename TBuffType, typename TUpdateFunc>
	static void UpdateMapStandard(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize);

	template<typename TBuffType, typename TUpdateFunc>
	static void UpdateMapPersistent(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize);

	template<typename TBuffType, typename TUpdateFunc>
	void UpdateMap(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize);

	static void InitVBO(VBO*& vbo, const int vboSingleSize);

	template<typename TBuffType>
	static GLint RoundBuffSizeUp() {
		const auto getAllignment = []() {
			GLint buffAlignment = 0;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &buffAlignment);
			LOG_L(L_WARNING, "[%s] GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT = %d", __func__, buffAlignment);
			return std::max(buffAlignment, 32);
		};
		static uint32_t uboAlignment = getAllignment(); //executed once
		constexpr uint32_t buffTypeSize = sizeof(TBuffType);

		return (buffTypeSize + (uboAlignment - buffTypeSize % uboAlignment) % uboAlignment);
	}

	static intptr_t GetBufferOffset(const int vboSingleSize);
	static void UpdateMatrices(UniformMatricesBuffer* updateBuffer);
	static void UpdateParams(UniformParamsBuffer* updateBuffer);
private:
	static constexpr int BUFFERING = 3;

	static constexpr int UBO_MATRIX_IDX = 0;
	static constexpr int UBO_PARAMS_IDX = 1;

	int umbBufferSize = 0;
	int upbBufferSize = 0;

	UniformMatricesBuffer* umbBufferMap = nullptr;
	UniformParamsBuffer* upbBufferMap = nullptr;

	VBO* umbVBO = nullptr;
	VBO* upbVBO = nullptr;
};

#endif
