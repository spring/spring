#ifndef UNIFORM_CONSTANTS_H
#define UNIFORM_CONSTANTS_H

#include <array>
#include <cstdint>
#include <sstream>
#include <map>

#include "System/float3.h"
#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/creg/creg.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/StreamBuffer.h"

#include "fmt/format.h"

struct UniformMatricesBuffer {
	CR_DECLARE_STRUCT(UniformMatricesBuffer)

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
	CR_DECLARE_STRUCT(UniformParamsBuffer)

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

	const std::string& GetGLSLDefinition(int idx) const { return glslDefinitions[idx]; }
private:
	static void UpdateMatricesImpl(UniformMatricesBuffer* updateBuffer);
	static void UpdateParamsImpl(UniformParamsBuffer* updateBuffer);

	template<typename T>
	static std::string SetGLSLDefinition(int binding);
public:
	static constexpr int UBO_MATRIX_IDX = 0;
	static constexpr int UBO_PARAMS_IDX = 1;
private:
	static constexpr int BUFFERING = 3;

	std::unique_ptr<IStreamBuffer<UniformMatricesBuffer>> umbSBT;
	std::unique_ptr<IStreamBuffer<UniformParamsBuffer  >> upbSBT;

	bool initialized = false;

	std::array<std::string, 2> glslDefinitions;
};

#endif

template<typename T>
inline std::string UniformConstants::SetGLSLDefinition(int binding)
{
	const T dummy{};

	std::map<uint32_t, std::pair<std::string, std::string>> membersMap;
	for (const auto& member : dummy.GetClass()->members) {
		membersMap[member.offset] = std::make_pair(std::string{ member.name }, member.type->GetName());
	}

	std::ostringstream output;

	output << fmt::format("layout(std140, binding = {}) uniform {} {{\n", binding, dummy.GetClass()->name); // {{ - escaped {

	for (const auto& [offset, info] : membersMap) {
		const auto& [name, tname] = info;

		std::string glslType;
		if (tname.rfind("CMatrix44f") != std::string::npos)
			glslType = "mat4";
		else if (tname.rfind("float4") != std::string::npos)
			glslType = "vec4";
		else if (tname.rfind("float3") != std::string::npos)
			glslType = "vec3";
		else if (tname.rfind("float2") != std::string::npos)
			glslType = "vec2";
		else if (tname.rfind("int") != std::string::npos)
			glslType = "uint";

		std::string arrayMods;

		const size_t bro = tname.rfind("[");
		const size_t brc = tname.rfind("]");

		if (bro != std::string::npos && brc != std::string::npos) {
			arrayMods = tname.substr(bro, brc - bro + 1);
		}

		assert(!glslType.empty());

		output << fmt::format("\t{} {}{};\n", glslType, name, arrayMods);
	}

	output << "};\n";

	return output.str();
}
