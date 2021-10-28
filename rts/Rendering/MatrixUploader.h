#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/StreamBuffer.h"


class CUnit;
class CFeature;
class CProjectile;
struct UnitDef;
struct FeatureDef;
struct S3DModel;

class MatrixUploader {
public:
	static constexpr bool enabled = true;
	static MatrixUploader& GetInstance() {
		static MatrixUploader instance;
		return instance;
	};
	static bool Supported();
public:
	void Init();
	void Kill();
	void Update();
public:
	// Defs
	std::size_t GetElemOffset(const UnitDef* def) const { return GetDefElemOffsetImpl(def); }
	std::size_t GetElemOffset(const FeatureDef* def) const { return GetDefElemOffsetImpl(def); }
	std::size_t GetElemOffset(const S3DModel* model) const { return GetDefElemOffsetImpl(model); }
	std::size_t GetUnitDefElemOffset(int32_t unitDefID) const;
	std::size_t GetFeatureDefElemOffset(int32_t featureDefID) const;

	// Objs
	std::size_t GetElemOffset(const CUnit* unit) const { return GetElemOffsetImpl(unit); }
	std::size_t GetElemOffset(const CFeature* feature) const { return GetElemOffsetImpl(feature); }
	std::size_t GetElemOffset(const CProjectile* proj) const { return GetElemOffsetImpl(proj); }
	std::size_t GetUnitElemOffset(int32_t unitID) const;
	std::size_t GetFeatureElemOffset(int32_t featureID) const;
	std::size_t GetProjectileElemOffset(int32_t syncedProjectileID) const;
private:
	std::size_t GetDefElemOffsetImpl(const S3DModel* model) const;
	std::size_t GetDefElemOffsetImpl(const UnitDef* def) const;
	std::size_t GetDefElemOffsetImpl(const FeatureDef* def) const;
	std::size_t GetElemOffsetImpl(const CUnit* so) const;
	std::size_t GetElemOffsetImpl(const CFeature* so) const;
	std::size_t GetElemOffsetImpl(const CProjectile* p) const;
private:
	void KillVBO();
	void InitVBO();
	uint32_t GetMatrixElemCount() const;
private:
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 0;
	static constexpr uint32_t elemCount0 = 1u << 13;
	static constexpr uint32_t elemIncreaseBy = 1u << 12;
private:
	std::unique_ptr<IStreamBuffer<CMatrix44f>> matrixSSBO;
};

#define matrixUploader MatrixUploader::GetInstance()