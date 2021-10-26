#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <array>
#include <cstdint>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/StreamBuffer.h"
//#include "Rendering/GL/VBO.h"

struct UniformMatricesBuffer {
	CMatrix44f screenView;
	CMatrix44f screenProj;
	CMatrix44f screenViewProj;

	CMatrix44f cameraView;
	CMatrix44f cameraProj;
	CMatrix44f cameraViewProj;
	CMatrix44f cameraBillboardView;

	CMatrix44f cameraViewInv;
	CMatrix44f cameraProjInv;
	CMatrix44f cameraViewProjInv;

	CMatrix44f shadowView;
	CMatrix44f shadowProj;
	CMatrix44f shadowViewProj;

	CMatrix44f reflectionView;
	CMatrix44f reflectionProj;
	CMatrix44f reflectionViewProj;

	CMatrix44f orthoProj01;

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	CMatrix44f mmDrawView; //world to MM
	CMatrix44f mmDrawProj; //world to MM
	CMatrix44f mmDrawViewProj; //world to MM

	CMatrix44f mmDrawIMMView; //heightmap to MM
	CMatrix44f mmDrawIMMProj; //heightmap to MM
	CMatrix44f mmDrawIMMViewProj; //heightmap to MM

	CMatrix44f mmDrawDimView; //mm dims
	CMatrix44f mmDrawDimProj; //mm dims
	CMatrix44f mmDrawDimViewProj; //mm dims
};

struct UniformParamsBuffer {
	float3 rndVec3; //new every draw frame.
	uint32_t renderCaps; //various render booleans

	float4 timeInfo;     //gameFrame, gameSeconds, drawFrame, frameTimeOffset
	float4 viewGeometry; //vsx, vsy, vpx, vpy
	float4 mapSize;      //xz, xzPO2
	float4 mapHeight;    //height minCur, maxCur, minInit, maxInit

	float4 fogColor;  //fog color
	float4 fogParams; //fog {start, end, 0.0, scale}

	float4 sunDir;

	float4 sunAmbientModel;
	float4 sunAmbientMap;
	float4 sunDiffuseModel;
	float4 sunDiffuseMap;
	float4 sunSpecularModel;
	float4 sunSpecularMap;

	float4 shadowDensity; // {ground, units, 0.0, 0.0}

	float4 windInfo; // windx, windy, windz, windStrength
	float2 mouseScreenPos; //x, y. Screen space.
	uint32_t mouseStatus; // bits 0th to 32th: LMB, MMB, RMB, offscreen, mmbScroll, locked
	uint32_t mouseUnused;
	float4 mouseWorldPos; //x,y,z; w=0 -- offmap. Ignores water, doesn't ignore units/features under the mouse cursor

	float4 teamColor[MAX_TEAMS]; //all team colors
};

class UniformConstants {
public:
	static UniformConstants& GetInstance() {
		static UniformConstants uniformConstantsInstance;
		return uniformConstantsInstance;
	};
	static bool Supported();
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
	static void UpdateMatricesImpl(UniformMatricesBuffer* updateBuffer);
	static void UpdateParamsImpl(UniformParamsBuffer* updateBuffer);
private:
	static constexpr int BUFFERING = 3;

	static constexpr int UBO_MATRIX_IDX = 0;
	static constexpr int UBO_PARAMS_IDX = 1;

	std::pair<UniformMatricesBuffer*, uint32_t> umbMap;
	std::pair<UniformParamsBuffer*  , uint32_t> upbMap;

	std::unique_ptr<IStreamBuffer<UniformMatricesBuffer>> umbSBT;
	std::unique_ptr<IStreamBuffer<UniformParamsBuffer  >> upbSBT;

	bool initialized = false;
};

#endif
