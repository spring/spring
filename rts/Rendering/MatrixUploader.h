#ifndef MATRIX_UPLOADER_H
#define MATRIX_UPLOADER_H

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"

class MatrixUploader {
public:
	static constexpr bool enabled = true;
	static constexpr bool checkInView = false;
	static MatrixUploader& GetInstance() {
		static MatrixUploader instance;
		return instance;
	};
	static bool Supported() {
		static bool supported = enabled && VBO::IsSupported(GL_UNIFORM_BUFFER) && VBO::IsSupported(GL_SHADER_STORAGE_BUFFER) && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
		return supported;
	}
public:
	void Init();
	void Kill();
	void UpdateAndBind();
public:
	uint32_t GetUnitDefElemOffset(int32_t unitDefID);
	uint32_t GetFeatureDefElemOffset(int32_t featureDefID);
	uint32_t GetUnitElemOffset(int32_t unitID);
	uint32_t GetFeatureElemOffset(int32_t featureID);
private:
	template<typename TObj>
	bool IsObjectVisible(const TObj* obj);

	template<typename TObj>
	bool IsInView(const TObj* obj);

	template<typename TObj>
	void GetVisibleObjects(std::unordered_map<int, const TObj*>& visibleObjects);
private:
	void KillVBO();
	void InitVBO(const uint32_t newElemCount);
	uint32_t GetMatrixElemCount();

	bool UpdateObjectDefs();

	template<typename TObj>
	void UpdateVisibleObjects();
private:
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 0;
	static constexpr uint32_t elemCount0 = 1u << 13;
	static constexpr uint32_t elemIncreaseBy = 1u << 12;
private:
	bool initialized = false;
	uint32_t elemUpdateOffset = 0u; // a index offset separating constant part of the buffer from varying part

	std::unordered_map<int32_t, std::string> unitDefToModel;
	std::unordered_map<int32_t, std::string> featureDefToModel;
	std::unordered_map<std::string, uint32_t> modelToOffsetMap;

	std::unordered_map<int32_t, uint32_t> unitIDToOffsetMap;
	std::unordered_map<int32_t, uint32_t> featureIDToOffsetMap;
	std::unordered_map<int32_t, uint32_t> weaponIDToOffsetMap;

	std::vector<CMatrix44f> matrices;

	VBO* matrixSSBO;
};

#endif //MATRIX_UPLOADER_H