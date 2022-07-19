#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

#include "System/Matrix44f.h"
#include "System/SpringMath.h"
#include "System/TypeToStr.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Rendering/Models/ModelsMemStorageDefs.h"


class CUnit;
class CFeature;
class CProjectile;
struct UnitDef;
struct FeatureDef;
struct S3DModel;

template<typename T, typename Derived>
class TypedStorageBufferUploader {
public:
	static Derived& GetInstance() {
		static Derived instance;
		return instance;
	};
public:
	void Init() { static_cast<Derived*>(this)->InitDerived(); }
	void Kill() { static_cast<Derived*>(this)->KillDerived(); }
	void Update() { static_cast<Derived*>(this)->UpdateDerived(); }
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
protected:
	void InitImpl(uint32_t bindingIdx_, uint32_t elemCount0_, uint32_t elemCountIncr_, uint8_t type, bool coherent, uint32_t numBuffers);
	void KillImpl();

	virtual std::size_t GetDefElemOffsetImpl(const S3DModel* model) const = 0;
	virtual std::size_t GetDefElemOffsetImpl(const UnitDef* def) const = 0;
	virtual std::size_t GetDefElemOffsetImpl(const FeatureDef* def) const = 0;
	virtual std::size_t GetElemOffsetImpl(const CUnit* so) const = 0;
	virtual std::size_t GetElemOffsetImpl(const CFeature* so) const = 0;
	virtual std::size_t GetElemOffsetImpl(const CProjectile* p) const = 0;

	uint32_t GetElemsCount() const;
protected:
	uint32_t bindingIdx = -1u;
	uint32_t elemCount0 = 0;
	uint32_t elemCountIncr = 0;
	static constexpr const char* className = spring::TypeToCStr<Derived>();

	std::unique_ptr<IStreamBuffer<T>> ssbo;
};

class MatrixUploader : public TypedStorageBufferUploader<CMatrix44f, MatrixUploader> {
public:
	void InitDerived();
	void KillDerived();
	void UpdateDerived();
protected:
	std::size_t GetDefElemOffsetImpl(const S3DModel* model) const override;
	std::size_t GetDefElemOffsetImpl(const UnitDef* def) const override;
	std::size_t GetDefElemOffsetImpl(const FeatureDef* def) const override;
	std::size_t GetElemOffsetImpl(const CUnit* so) const override;
	std::size_t GetElemOffsetImpl(const CFeature* so) const override;
	std::size_t GetElemOffsetImpl(const CProjectile* p) const override;
private:
	static constexpr uint32_t MATRIX_SSBO_BINDING_IDX = 0;
	static constexpr uint32_t ELEM_COUNT0 = 1u << 13;
	static constexpr uint32_t ELEM_COUNTI = 1u << 12;
};

class ModelsUniformsUploader : public TypedStorageBufferUploader<ModelUniformData, ModelsUniformsUploader> {
public:
	void InitDerived();
	void KillDerived();
	void UpdateDerived();
protected:
	virtual std::size_t GetDefElemOffsetImpl(const S3DModel* model) const override;
	virtual std::size_t GetDefElemOffsetImpl(const UnitDef* def) const override;
	virtual std::size_t GetDefElemOffsetImpl(const FeatureDef* def) const override;
	virtual std::size_t GetElemOffsetImpl(const CUnit* so) const override;
	virtual std::size_t GetElemOffsetImpl(const CFeature* so) const override;
	virtual std::size_t GetElemOffsetImpl(const CProjectile* p) const override;
private:
	static constexpr uint32_t MATUNI_SSBO_BINDING_IDX = 1;
	static constexpr uint32_t ELEM_COUNT0 = 1u << 12;
	static constexpr uint32_t ELEM_COUNTI = 1u << 11;
};

#define matrixUploader MatrixUploader::GetInstance()
#define modelsUniformsUploader ModelsUniformsUploader::GetInstance()