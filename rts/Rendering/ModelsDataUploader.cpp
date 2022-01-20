#include "ModelsDataUploader.h"

#include <limits>
#include <cassert>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/TimeProfiler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"


////////////////////////////////////////////////////////////////////


template<typename T, typename Derived>
inline bool TypedStorageBufferUploader<T, Derived>::Supported()
{
	static bool supported = VBO::IsSupported(GL_SHADER_STORAGE_BUFFER) && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
	return supported;
}

template<typename T, typename Derived>
void TypedStorageBufferUploader<T, Derived>::InitImpl(uint32_t bindingIdx_, uint32_t elemCount0_, uint32_t elemCountIncr_, uint8_t type, bool coherent, uint32_t numBuffers)
{
	if (!Supported())
		return;

	bindingIdx = bindingIdx_;
	elemCount0 = elemCount0_;
	elemCountIncr = elemCountIncr_;

	assert(bindingIdx < -1u);

	ssbo = IStreamBuffer<T>::CreateInstance(GL_SHADER_STORAGE_BUFFER, elemCount0, std::string(className), static_cast<IStreamBufferConcept::Types>(type), true, coherent, numBuffers);
	ssbo->BindBufferRange(bindingIdx);
}

template<typename T, typename Derived>
void TypedStorageBufferUploader<T, Derived>::KillImpl()
{
	if (!Supported())
		return;

	ssbo->UnbindBufferRange(bindingIdx);
	ssbo = nullptr;
}

template<typename T, typename Derived>
inline uint32_t TypedStorageBufferUploader<T, Derived>::GetElemsCount() const
{
	return ssbo->GetByteSize() / sizeof(T);
}

template<typename T, typename Derived>
std::size_t TypedStorageBufferUploader<T, Derived>::GetUnitDefElemOffset(int32_t unitDefID) const
{
	return GetDefElemOffsetImpl(unitDefHandler->GetUnitDefByID(unitDefID));
}

template<typename T, typename Derived>
std::size_t TypedStorageBufferUploader<T, Derived>::GetFeatureDefElemOffset(int32_t featureDefID) const
{
	return GetDefElemOffsetImpl(featureDefHandler->GetFeatureDefByID(featureDefID));
}

template<typename T, typename Derived>
std::size_t TypedStorageBufferUploader<T, Derived>::GetUnitElemOffset(int32_t unitID) const
{
	return GetElemOffsetImpl(unitHandler.GetUnit(unitID));
}

template<typename T, typename Derived>
std::size_t TypedStorageBufferUploader<T, Derived>::GetFeatureElemOffset(int32_t featureID) const
{
	return GetElemOffsetImpl(featureHandler.GetFeature(featureID));
}

template<typename T, typename Derived>
std::size_t TypedStorageBufferUploader<T, Derived>::GetProjectileElemOffset(int32_t syncedProjectileID) const
{
	return GetElemOffsetImpl(projectileHandler.GetProjectileBySyncedID(syncedProjectileID));
}

void MatrixUploader::InitDerived()
{
	const auto sbType = globalRendering->supportPersistentMapping
		? IStreamBufferConcept::Types::SB_PERSISTENTMAP
		: IStreamBufferConcept::Types::SB_BUFFERSUBDATA;

	InitImpl(MATRIX_SSBO_BINDING_IDX, ELEM_COUNT0, ELEM_COUNTI, sbType, true, MatricesMemStorage::BUFFERING);
	if (ssbo->GetBufferImplementation() == IStreamBufferConcept::Types::SB_PERSISTENTMAP && !ssbo->IsValid()) {
		// some potatoe driver overestimated its support for SB_PERSISTENTMAP
		// Redo with good old SB_BUFFERSUBDATA
		KillImpl();
		InitImpl(MATRIX_SSBO_BINDING_IDX, ELEM_COUNT0, ELEM_COUNTI, IStreamBufferConcept::Types::SB_BUFFERSUBDATA, true, MatricesMemStorage::BUFFERING);
	}
}

void MatrixUploader::KillDerived()
{
	KillImpl();
}

void MatrixUploader::UpdateDerived()
{
	if (!Supported())
		return;

	SCOPED_TIMER("MatrixUploader::Update");
	ssbo->UnbindBufferRange(bindingIdx);

	//resize
	const uint32_t elemCount = GetElemsCount();
	const uint32_t storageElemCount = matricesMemStorage.GetSize();
	if (storageElemCount > elemCount) {
		const uint32_t newElemCount = AlignUp(storageElemCount, elemCountIncr);
		LOG_L(L_DEBUG, "[%s::%s] sizing SSBO %s. New elements count = %u, elemCount = %u, storageElemCount = %u", className, __func__, "up", newElemCount, elemCount, storageElemCount);
		ssbo->Resize(newElemCount);

		if (ssbo->GetBufferImplementation() == IStreamBufferConcept::Types::SB_PERSISTENTMAP && !ssbo->IsValid()) {
			KillImpl();
			InitImpl(MATRIX_SSBO_BINDING_IDX, newElemCount, ELEM_COUNTI, IStreamBufferConcept::Types::SB_BUFFERSUBDATA, true, MatricesMemStorage::BUFFERING);
		}

		matricesMemStorage.SetAllDirty(); //Resize doesn't copy the data
	}

	//update on the GPU
	const CMatrix44f* clientPtr = matricesMemStorage.GetData().data();

	constexpr bool ENABLE_UPLOAD_OPTIMIZATION = true;
	if (ssbo->GetBufferImplementation() == IStreamBufferConcept::Types::SB_PERSISTENTMAP && ENABLE_UPLOAD_OPTIMIZATION) {
		const auto stt = matricesMemStorage.GetDirtyMap().begin();
		const auto fin = matricesMemStorage.GetDirtyMap().end();

		auto beg = matricesMemStorage.GetDirtyMap().begin();
		auto end = matricesMemStorage.GetDirtyMap().begin();

		static const auto dirtyPred = [](uint8_t m) -> bool { return m > 0u; };
		while (beg != fin) {
			beg = std::find_if    (beg, fin, dirtyPred);
			end = std::find_if_not(beg, fin, dirtyPred);

			if (beg != fin) {
				const uint32_t offs = static_cast<uint32_t>(std::distance(stt, beg));
				const uint32_t size = static_cast<uint32_t>(std::distance(beg, end));

				CMatrix44f* mappedPtr = ssbo->Map(clientPtr, offs, size);
				memcpy(mappedPtr, clientPtr + offs, size * sizeof(CMatrix44f));
				ssbo->Unmap();

				std::transform(beg, end, beg, [](uint8_t v) { return (v - 1); }); //make it less dirty
			}

			beg = end; //rewind
		}
	}
	else {
		const CMatrix44f* clientPtr = matricesMemStorage.GetData().data();
		CMatrix44f* mappedPtr = ssbo->Map(clientPtr, 0, storageElemCount);

		if (!ssbo->HasClientPtr())
			memcpy(mappedPtr, clientPtr, storageElemCount * sizeof(CMatrix44f));

		ssbo->Unmap();
	}
	ssbo->BindBufferRange(bindingIdx);
	ssbo->SwapBuffer();
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(const S3DModel* model) const
{
	if (model == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr S3DModel", className, __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return model->GetMatAlloc().GetOffset(false);
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(const UnitDef* def) const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr UnitDef", className, __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return GetDefElemOffsetImpl(def->LoadModel());
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(const FeatureDef* def) const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr FeatureDef", className, __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return GetDefElemOffsetImpl(def->LoadModel());
}

std::size_t MatrixUploader::GetElemOffsetImpl(const CUnit* unit) const
{
	if (unit == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CUnit", className, __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (std::size_t offset = CUnitDrawer::GetMatricesMemAlloc(unit).GetOffset(false); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CUnit (id:%d)", className, __func__, unit->id);
	return MatricesMemStorage::INVALID_INDEX;
}

std::size_t MatrixUploader::GetElemOffsetImpl(const CFeature* feature) const
{
	if (feature == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CFeature", className, __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (std::size_t offset = CFeatureDrawer::GetMatricesMemAlloc(feature).GetOffset(false); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CFeature (id:%d)", className, __func__, feature->id);
	return MatricesMemStorage::INVALID_INDEX;
}

std::size_t MatrixUploader::GetElemOffsetImpl(const CProjectile* p) const
{
	if (p == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CProjectile", className, __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (!p->synced) {
		LOG_L(L_ERROR, "[%s::%s] Supplied non-synced CProjectile (id:%d)", className, __func__, p->id);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (!p->weapon || !p->piece) {
		LOG_L(L_ERROR, "[%s::%s] Supplied non-weapon or non-piece CProjectile (id:%d)", className, __func__, p->id);
		return MatricesMemStorage::INVALID_INDEX;
	}
	/*
	if (std::size_t offset = p->GetMatAlloc().GetOffset(false); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}
	*/

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CProjectile (id:%d)", className, __func__, p->id);
	return MatricesMemStorage::INVALID_INDEX;
}

void ModelsUniformsUploader::InitDerived()
{
	InitImpl(MATUNI_SSBO_BINDING_IDX, ELEM_COUNT0, ELEM_COUNTI, IStreamBufferConcept::Types::SB_BUFFERSUBDATA, true, 3);
}

void ModelsUniformsUploader::KillDerived()
{
	KillImpl();
}

void ModelsUniformsUploader::UpdateDerived()
{
	if (!Supported())
		return;

	SCOPED_TIMER("ModelsUniformsUploader::Update");
	ssbo->UnbindBufferRange(bindingIdx);

	//resize
	const uint32_t elemCount = GetElemsCount();
	const uint32_t storageElemCount = modelsUniformsStorage.Size();
	if (storageElemCount > elemCount) {
		const uint32_t newElemCount = AlignUp(storageElemCount, elemCountIncr);
		LOG_L(L_DEBUG, "[%s::%s] sizing SSBO %s. New elements count = %u, elemCount = %u, storageElemCount = %u", className, __func__, "up", newElemCount, elemCount, storageElemCount);
		ssbo->Resize(newElemCount);
	}

	//update on the GPU
	const ModelUniformData* clientPtr = modelsUniformsStorage.GetData().data();
	ModelUniformData* mappedPtr = ssbo->Map(clientPtr, 0, storageElemCount);

	if (!ssbo->HasClientPtr())
		memcpy(mappedPtr, clientPtr, storageElemCount * sizeof(ModelUniformData));

	ssbo->Unmap();
	ssbo->BindBufferRange(bindingIdx);
	ssbo->SwapBuffer();
}

std::size_t ModelsUniformsUploader::GetDefElemOffsetImpl(const S3DModel* model) const
{
	assert(false);
	LOG_L(L_ERROR, "[%s::%s] Invalid call", className, __func__);
	return ModelsUniformsStorage::INVALID_INDEX;
}

std::size_t ModelsUniformsUploader::GetDefElemOffsetImpl(const UnitDef* def) const
{
	assert(false);
	LOG_L(L_ERROR, "[%s::%s] Invalid call", className, __func__);
	return ModelsUniformsStorage::INVALID_INDEX;
}

std::size_t ModelsUniformsUploader::GetDefElemOffsetImpl(const FeatureDef* def) const
{
	assert(false);
	LOG_L(L_ERROR, "[%s::%s] Invalid call", className, __func__);
	return ModelsUniformsStorage::INVALID_INDEX;
}


std::size_t ModelsUniformsUploader::GetElemOffsetImpl(const CUnit* unit) const
{
	if (unit == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CUnit", className, __func__);
		return ModelsUniformsStorage::INVALID_INDEX;
	}

	if (std::size_t offset = modelsUniformsStorage.GetObjOffset(unit); offset != std::size_t(-1)) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CUnit (id:%d)", className, __func__, unit->id);
	return ModelsUniformsStorage::INVALID_INDEX;
}

std::size_t ModelsUniformsUploader::GetElemOffsetImpl(const CFeature* feature) const
{
	if (feature == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CFeature", className, __func__);
		return ModelsUniformsStorage::INVALID_INDEX;
	}

	if (std::size_t offset = modelsUniformsStorage.GetObjOffset(feature); offset != std::size_t(-1)) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CFeature (id:%d)", className, __func__, feature->id);
	return ModelsUniformsStorage::INVALID_INDEX;
}

std::size_t ModelsUniformsUploader::GetElemOffsetImpl(const CProjectile* p) const
{
	if (p == nullptr) {
		LOG_L(L_ERROR, "[%s::%s] Supplied nullptr CProjectile", className, __func__);
		return ModelsUniformsStorage::INVALID_INDEX;
	}

	if (std::size_t offset = modelsUniformsStorage.GetObjOffset(p); offset != std::size_t(-1)) {
		return offset;
	}

	LOG_L(L_ERROR, "[%s::%s] Supplied invalid CProjectile (id:%d)", className, __func__, p->id);
	return ModelsUniformsStorage::INVALID_INDEX;
}