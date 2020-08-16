#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <array>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"

struct UniformConstantsBuffer {
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

	float4 timeInfo; //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	float4 viewGemetry; //vsx, vsy, vpx, vpy
	float4 mapSize; //xz, xzPO2
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
	UniformConstants() : ucbBuffer(nullptr), bound(false) {};
	void Init();
	void Kill();
	void Update();
	void Bind(const int bindIndex = BINDING_INDEX);
	void Unbind(const int bindIndex = BINDING_INDEX);
private:
	void UpdateMatrices(UniformConstantsBuffer* updateBuffer);
	void UpdateParams(UniformConstantsBuffer* updateBuffer);
private:
	static constexpr int BUFFERING = 3;
	static constexpr GLuint BINDING_INDEX = 0;
	static constexpr bool PERSISTENT_STORAGE = false;

	int buffOffset;
	int buffSize;

	int buffSlice;

	bool bound;

	VBO* ucbBuffer;
	std::array<VBO*, BUFFERING> ucbBuffers;
};

#endif