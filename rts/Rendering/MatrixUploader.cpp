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
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/MatricesMemStorage.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Features/FeatureDrawer.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"

void MatrixUploader::InitVBO(const uint32_t newElemCount)
{
	matrixSSBO = VBO(GL_SHADER_STORAGE_BUFFER, false, false);
	matrixSSBO.Bind(GL_SHADER_STORAGE_BUFFER);
	matrixSSBO.New(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
	matrixSSBO.Unbind();

	matrixSSBO.BindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO.GetSize());
}

void MatrixUploader::Init()
{
	if (!MatrixUploader::Supported())
		return;

	InitVBO(elemCount0);
}

void MatrixUploader::KillVBO()
{
	if (matrixSSBO.GetIdRaw() > 0u) {
		if (matrixSSBO.bound)
			matrixSSBO.Unbind();

		matrixSSBO.UnbindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO.GetSize());
	}

	matrixSSBO = {};
}

void MatrixUploader::Kill()
{
	if (!MatrixUploader::Supported())
		return;

	KillVBO();
}

uint32_t MatrixUploader::GetMatrixElemCount() const
{
	return matrixSSBO.GetSize() / sizeof(CMatrix44f);
}

void MatrixUploader::Update()
{
	if (!MatrixUploader::Supported())
		return;

	SCOPED_TIMER("MatrixUploader::Update");

	//resize
	const uint32_t matrixElemCount = GetMatrixElemCount();
	const uint32_t matricesMemStorageCount = matricesMemStorage.GetSize();
	if (matricesMemStorageCount > matrixElemCount) {
		const uint32_t newElemCount = AlignUp(matricesMemStorageCount, elemIncreaseBy);
		LOG_L(L_INFO, "MatrixUploader::%s sizing matrixSSBO %s. New elements count = %u, matrixElemCount = %u, matricesMemStorageCount = %u", __func__, "up", newElemCount, matrixElemCount, matricesMemStorageCount);
		matrixSSBO.UnbindBufferRange(MATRIX_SSBO_BINDING_IDX);
		matrixSSBO.Bind();
		matrixSSBO.Resize(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
		matrixSSBO.Unbind();
		matrixSSBO.BindBufferRange(MATRIX_SSBO_BINDING_IDX);
	}

	//update on the GPU
	matrixSSBO.Bind();
#if 0 //unexpectedly expensive on idle run (NVidia & Windows). Needs triple buffering to perform
	auto* buff = matrixSSBO.MapBuffer(matricesMemStorage.GetData(), 0, GL_WRITE_ONLY); //matrices.size() always has the correct size no matter neededElemByteOffset
	memcpy(buff, matricesMemStorage.GetData().data(), matricesMemStorage.GetSize() * sizeof(CMatrix44f));
	matrixSSBO.UnmapBuffer();
#else
	matrixSSBO.SetBufferSubData(matricesMemStorage.GetData());
#endif
	matrixSSBO.Unbind();
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
