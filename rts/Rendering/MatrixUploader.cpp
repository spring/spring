#include "MatrixUploader.h"

#include <limits>

#include "System/float4.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileTypes.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/GL/VBO.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Camera.h"


void ProxyProjectileListener::RenderProjectileCreated(const CProjectile* projectile)
{
	MatrixUploader::GetInstance().visibleProjectilesSet.emplace(projectile);
}

void ProxyProjectileListener::RenderProjectileDestroyed(const CProjectile* projectile)
{
	MatrixUploader::GetInstance().visibleProjectilesSet.erase(projectile);
}


//////////////////



void MatrixUploader::InitVBO(const uint32_t newElemCount)
{
	matrixSSBO = new VBO(GL_SHADER_STORAGE_BUFFER, false);
	matrixSSBO->Bind(GL_SHADER_STORAGE_BUFFER);
	matrixSSBO->New(newElemCount * sizeof(CMatrix44f), GL_STREAM_DRAW);
	matrixSSBO->Unbind();
}


void MatrixUploader::Init()
{
	if (initialized)
		return;

	if (!MatrixUploader::Supported())
		return;

	proxyProjectileListener = new ProxyProjectileListener{};
	InitVBO(elemCount0);
	matrices.reserve(elemCount0);
	initialized = true;
}

void MatrixUploader::KillVBO()
{
	if (matrixSSBO && matrixSSBO->bound)
		matrixSSBO->Unbind();

	spring::SafeDelete(matrixSSBO);
}

void MatrixUploader::Kill()
{
	if (!MatrixUploader::Supported())
		return;

	KillVBO();
	spring::SafeDelete(proxyProjectileListener);
}

template<typename TObj>
bool MatrixUploader::IsObjectVisible(const TObj* obj)
{
	if (losHandler->globalLOS[gu->myAllyTeam])
		return true;

	if constexpr (std::is_same<TObj, CProjectile>::value) //CProjectile has no IsInLosForAllyTeam()
		return losHandler->InLos(obj, gu->myAllyTeam);
	else //else is needed. Otherwise won't compile
		return obj->IsInLosForAllyTeam(gu->myAllyTeam);
}

template<typename TObj>
bool MatrixUploader::IsInView(const TObj* obj)
{
	constexpr float leewayRadius = 16.0f;
	if constexpr (std::is_same<TObj, CProjectile>::value)
		return camera->InView(obj->drawPos, leewayRadius + obj->GetDrawRadius());
	else
		return camera->InView(obj->drawMidPos, leewayRadius + obj->GetDrawRadius());
}


template<typename TObj>
void MatrixUploader::GetVisibleObjects()
{
	if constexpr (std::is_same<TObj, CUnit>::value) {
		visibleUnits.clear();
		for (const auto* obj : unitHandler.GetActiveUnits()) {
			if (!IsObjectVisible(obj))
				continue;

			if (!IsInView(obj))
				continue;

			visibleUnits.emplace_back(obj);
		}
		return;
	}

	if constexpr (std::is_same<TObj, CFeature>::value) {
		visibleFeatures.clear();
		for (const int fID : featureHandler.GetActiveFeatureIDs()) {
			const auto* obj = featureHandler.GetFeature(fID);
			if (!IsObjectVisible(obj))
				continue;

			if (!IsInView(obj))
				continue;

			visibleFeatures.emplace_back(obj);
		}
		return;
	}

	if constexpr (std::is_same<TObj, CProjectile>::value) {
		visibleProjectiles.clear();
		for (const auto* obj : visibleProjectilesSet) {
		//for (const int pID : visibleProjectilesSet) {
			//const auto* obj = projectileHandler.GetProjectileByUnsyncedID(pID);

			if (!obj->weapon)
				continue;

			//const CWeaponProjectile* wp = p->weapon ? static_cast<const CWeaponProjectile*>(p) : nullptr;
			//const WeaponDef* wd = p->weapon ? wp->GetWeaponDef() : nullptr;

			//if (!obj->synced)
				//continue;
			if (obj)

			if (!IsObjectVisible(obj))
				continue;

			if (!IsInView(obj))
				continue;

			visibleProjectiles.emplace_back(obj);
		}
	}

	static_assert("Wrong TObj in MatrixSSBO::GetVisibleObjects()");
}

uint32_t MatrixUploader::GetMatrixElemCount()
{
	return matrixSSBO->GetSize() / sizeof(CMatrix44f);
}


void MatrixUploader::UpdateAndBind()
{
	if (!MatrixUploader::Supported())
		return;

	GetVisibleObjects<CUnit>();
	GetVisibleObjects<CFeature>();
	GetVisibleObjects<CProjectile>();

	std::vector<uint16_t> elemOffset;

	matrices.clear();

	for (const auto* obj : visibleUnits) {
		const int elemBeginIndex = std::distance(matrices.cbegin(), matrices.cend());
		matrices.emplace_back(obj->GetTransformMatrix(false, losHandler->globalLOS[gu->myAllyTeam]));
		const auto& lm = obj->localModel;
		for (const auto& lmp : lm.pieces) {
			matrices.emplace_back(lmp.GetModelSpaceMatrix());
		}
		const int elemEndIndex = std::distance(matrices.cbegin(), matrices.cend());
	}

	for (const auto* obj : visibleFeatures) {
		const int elemBeginIndex = std::distance(matrices.cbegin(), matrices.cend());
		matrices.emplace_back(obj->GetTransformMatrix(false, losHandler->globalLOS[gu->myAllyTeam]));
		const auto& lm = obj->localModel;
		for (const auto& lmp : lm.pieces) {
			matrices.emplace_back(lmp.GetModelSpaceMatrix());
		}
		const int elemEndIndex = std::distance(matrices.cbegin(), matrices.cend());
	}

	for (const auto* obj : visibleProjectiles) {
		const int elemBeginIndex = std::distance(matrices.cbegin(), matrices.cend());
		matrices.emplace_back(obj->GetTransformMatrix(obj->GetProjectileType() == WEAPON_MISSILE_PROJECTILE));
		const int elemEndIndex = std::distance(matrices.cbegin(), matrices.cend());
	}
	LOG_L(L_INFO, "MatrixUploader::%s matrices.size = [%u]", __func__, static_cast<uint32_t>(matrices.size()));

	matrixSSBO->UnbindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO->GetSize());

	//resize
	const uint32_t matrixElemCount = GetMatrixElemCount();
	int newElemCount = 0;

	if (matrices.size() > matrixElemCount) {
		newElemCount = matrixElemCount + std::max(elemIncreaseBy, static_cast<uint32_t>(matrices.size()));
		LOG_L(L_INFO, "MatrixUploader::%s sizing matrixSSBO %s. New elements count = %d", __func__, "up", newElemCount);
	}
	/* don't size down to avoid stuttering (for now). TODO: make averaging
	else if (matrices.size() < matrixElemCount / elemSizingDownDiv && matrixElemCount / elemSizingDownDiv >= elemCount0) {
		newElemCount = std::max(elemCount0, static_cast<uint32_t>(matrices.size()));
		LOG_L(L_INFO, "MatrixUploader::%s sizing matrixSSBO %s. New elements count = %d", __func__, "down", newElemCount);
	}
	*/

	if (newElemCount > 0) {
		// No reason to resize and keep GPU data intact, just recreate a VBO
		KillVBO();
		InitVBO(newElemCount);
	}

	//update on the GPU
	matrixSSBO->Bind(GL_SHADER_STORAGE_BUFFER);
	auto* buff = matrixSSBO->MapBuffer(0, matrices.size() * sizeof(CMatrix44f), GL_WRITE_ONLY);
	memcpy(buff, matrices.data(), matrices.size() * sizeof(CMatrix44f));
	matrixSSBO->UnmapBuffer();
	matrixSSBO->Unbind();

	matrixSSBO->BindBufferRange(GL_SHADER_STORAGE_BUFFER, MATRIX_SSBO_BINDING_IDX, 0, matrixSSBO->GetSize());
}
