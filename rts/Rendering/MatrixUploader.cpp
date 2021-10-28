#include "MatrixUploader.h"

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
#include "Rendering/Models/MatricesMemStorage.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"

void MatrixUploader::InitVBO()
{
	matrixSSBO = IStreamBuffer<CMatrix44f>::CreateInstance(GL_SHADER_STORAGE_BUFFER, elemCount0, "MatricesSSBO", IStreamBufferConcept::Types::SB_AUTODETECT, true);
	matrixSSBO->BindBufferRange(MATRIX_SSBO_BINDING_IDX);
}

bool MatrixUploader::Supported()
{
	static bool supported = enabled && VBO::IsSupported(GL_SHADER_STORAGE_BUFFER) && GLEW_ARB_shading_language_420pack; //UBO && UBO layout(binding=x)
	return supported;
}

void MatrixUploader::Init()
{
	if (!MatrixUploader::Supported())
		return;

	InitVBO();
}

void MatrixUploader::KillVBO()
{
	matrixSSBO->UnbindBufferRange(MATRIX_SSBO_BINDING_IDX);
	matrixSSBO = nullptr;
}

void MatrixUploader::Kill()
{
	if (!MatrixUploader::Supported())
		return;

	KillVBO();
}

uint32_t MatrixUploader::GetMatrixElemCount() const
{
	return matrixSSBO->GetByteSize() / sizeof(CMatrix44f);
}

void MatrixUploader::Update()
{
	if (!MatrixUploader::Supported())
		return;

	SCOPED_TIMER("MatrixUploader::Update");

	matrixSSBO->UnbindBufferRange(MATRIX_SSBO_BINDING_IDX);

	//resize
	const uint32_t matrixElemCount = GetMatrixElemCount();
	const uint32_t matricesMemStorageCount = matricesMemStorage.GetSize();
	if (matricesMemStorageCount > matrixElemCount) {
		const uint32_t newElemCount = AlignUp(matricesMemStorageCount, elemIncreaseBy);
		LOG_L(L_INFO, "MatrixUploader::%s sizing matrixSSBO %s. New elements count = %u, matrixElemCount = %u, matricesMemStorageCount = %u", __func__, "up", newElemCount, matrixElemCount, matricesMemStorageCount);
		matrixSSBO->Resize(newElemCount);
	}

	//update on the GPU
	auto* clientPtr = matricesMemStorage.GetData().data();
	auto* mappedPtr = matrixSSBO->Map(clientPtr);

	if (clientPtr != mappedPtr)
		memcpy(mappedPtr, clientPtr, matricesMemStorage.GetSize() * sizeof(CMatrix44f));

	matrixSSBO->Unmap(matricesMemStorage.GetSize());
	matrixSSBO->BindBufferRange(MATRIX_SSBO_BINDING_IDX);
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(const S3DModel* model) const
{
	if (model == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied nullptr S3DModel", __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return model->GetMatAlloc().GetOffset(false);
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(const UnitDef* def)  const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied nullptr UnitDef", __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return GetDefElemOffsetImpl(def->LoadModel());
}

std::size_t MatrixUploader::GetDefElemOffsetImpl(const FeatureDef* def)  const
{
	if (def == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied nullptr FeatureDef", __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	return GetDefElemOffsetImpl(def->LoadModel());
}

std::size_t MatrixUploader::GetUnitDefElemOffset(int32_t unitDefID)  const
{
	return GetDefElemOffsetImpl(unitDefHandler->GetUnitDefByID(unitDefID));
}

std::size_t MatrixUploader::GetFeatureDefElemOffset(int32_t featureDefID)  const
{
	return GetDefElemOffsetImpl(featureDefHandler->GetFeatureDefByID(featureDefID));
}

std::size_t MatrixUploader::GetElemOffsetImpl(const CUnit* unit)  const
{
	if (unit == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied nullptr CUnit", __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (std::size_t offset = CUnitDrawer::GetMatricesMemAlloc(unit).GetOffset(false); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied invalid CUnit (id:%d)", __func__, unit->id);
	return MatricesMemStorage::INVALID_INDEX;
}

std::size_t MatrixUploader::GetElemOffsetImpl(const CFeature* feature)  const
{
	if (feature == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied nullptr CFeature", __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (std::size_t offset = CFeatureDrawer::GetMatricesMemAlloc(feature).GetOffset(false); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}

	LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied invalid CFeature (id:%d)", __func__, feature->id);
	return MatricesMemStorage::INVALID_INDEX;
}

std::size_t MatrixUploader::GetElemOffsetImpl(const CProjectile* p) const
{
	if (p == nullptr) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied nullptr CProjectile", __func__);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (!p->synced) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied non-synced CProjectile (id:%d)", __func__, p->id);
		return MatricesMemStorage::INVALID_INDEX;
	}

	if (!p->weapon || !p->piece) {
		LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied non-weapon or non-piece CProjectile (id:%d)", __func__, p->id);
		return MatricesMemStorage::INVALID_INDEX;
	}
	/*
	if (std::size_t offset = p->GetMatAlloc().GetOffset(false); offset != MatricesMemStorage::INVALID_INDEX) {
		return offset;
	}
	*/

	LOG_L(L_ERROR, "[MatrixUploader::%s] Supplied invalid CProjectile (id:%d)", __func__, p->id);
	return MatricesMemStorage::INVALID_INDEX;
}

std::size_t MatrixUploader::GetUnitElemOffset(int32_t unitID)  const
{
	return GetElemOffsetImpl(unitHandler.GetUnit(unitID));
}

std::size_t MatrixUploader::GetFeatureElemOffset(int32_t featureID)  const
{
	return GetElemOffsetImpl(featureHandler.GetFeature(featureID));
}

std::size_t MatrixUploader::GetProjectileElemOffset(int32_t syncedProjectileID) const
{
	return GetElemOffsetImpl(projectileHandler.GetProjectileBySyncedID(syncedProjectileID));
}
