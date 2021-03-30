#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <array>
#include <cstdint>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"

struct UniformMatricesBuffer {
	CMatrix44f screenView;
	CMatrix44f screenProj;
	CMatrix44f screenViewProj;

	CMatrix44f cameraView;
	CMatrix44f cameraProj;
	CMatrix44f cameraViewProj;
	CMatrix44f cameraBillboardProj;

	CMatrix44f cameraViewInv;
	CMatrix44f cameraProjInv;
	CMatrix44f cameraViewProjInv;

	CMatrix44f shadowView;
	CMatrix44f shadowProj;
	CMatrix44f shadowViewProj;

	CMatrix44f orthoProj01;

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	CMatrix44f mmDrawView; //world to MM
	CMatrix44f mmDrawIMMView; //heightmap to MM
	CMatrix44f mmDrawDimView; //mm dims

	CMatrix44f mmDrawProj; //world to MM
	CMatrix44f mmDrawIMMProj; //heightmap to MM
	CMatrix44f mmDrawDimProj; //mm dims

	CMatrix44f mmDrawViewProj; //world to MM
	CMatrix44f mmDrawIMMViewProj; //heightmap to MM
	CMatrix44f mmDrawDimViewProj; //mm dims
};

struct UniformParamsBuffer {
	float3 rndVec3; //new every draw frame.
	uint32_t renderCaps; //various render booleans

	float4 timeInfo; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	float4 viewGeometry; //vsx, vsy, vpx, vpy
	float4 mapSize; //xz, xzPO2

	float4 fogColor; //fog color
	float4 fogParams; //fog {start, end, 0.0, scale}

	float4 teamColor[MAX_TEAMS]; //all team colors
};

class UniformConstants {
public:
	static UniformConstants& GetInstance() {
		static UniformConstants uniformConstantsInstance;
		return uniformConstantsInstance;
	};
	static bool Supported() {
		static bool supported = VBO::IsSupported(GL_UNIFORM_BUFFER) && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
		return supported;
	}
public:
	void Init();
	void Kill();
	void UpdateMatrices();
	void UpdateParams();
	void Update() {
		UpdateMatrices();
		UpdateParams();
	}
	void Bind();
private:

	template<typename TBuffType, typename TUpdateFunc>
	static void UpdateMapStandard(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize);

	template<typename TBuffType, typename TUpdateFunc>
	static void UpdateMapPersistent(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize);

	template<typename TBuffType, typename TUpdateFunc>
	void UpdateMap(VBO* vbo, TBuffType*& buffMap, const TUpdateFunc& updateFunc, const int vboSingleSize);

	static void InitVBO(VBO*& vbo, const int vboSingleSize);

	static intptr_t GetBufferOffset(const int vboSingleSize);
	static void UpdateMatricesImpl(UniformMatricesBuffer* updateBuffer);
	static void UpdateParamsImpl(UniformParamsBuffer* updateBuffer);

	static bool WantPersistentMapping();
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

	bool initialized = false;
};

#endif
