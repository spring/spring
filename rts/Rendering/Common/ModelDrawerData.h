#pragma once

#include <vector>
#include <array>
#include <functional>

#include <unordered_map>

#include "System/EventClient.h"
#include "System/EventHandler.h"
#include "System/ContainerUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Threading/ThreadPool.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Env/IWater.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/CameraHandler.h"

class CModelDrawerDataConcept : public CEventClient {
public:
	CModelDrawerDataConcept(const std::string& ecName, int ecOrder)
		: CEventClient(ecName, ecOrder, false)
	{
		if (modelDrawDist == 0.0f)
			SetModelDrawDist(static_cast<float>(configHandler->GetInt("UnitLodDist")));
	};
	virtual ~CModelDrawerDataConcept() {
		eventHandler.RemoveClient(this);
		autoLinkedEvents.clear();
		modelDrawDist = 0.0f; //force re-read of UnitLodDist
	};
public:
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }
public:
	static void SetModelDrawDist(float dist) {
		modelDrawDist    = dist;
	}
public:
	// lenghts & distances
	static float inline modelDrawDist    = 0.0f;
};


template <typename T>
class CModelDrawerDataBase : public CModelDrawerDataConcept
{
public:
	using ObjType = T;
public:
	CModelDrawerDataBase(const std::string& ecName, int ecOrder, bool& mtModelDrawer_);
	virtual ~CModelDrawerDataBase() override;
public:
	virtual void Update() = 0;
protected:
	virtual bool IsAlpha(const T* co) const = 0;
private:
	void AddObject(const T* co, bool add); //never to be called directly! Use UpdateObject() instead!
protected:
	void DelObject(const T* co, bool del);
	void UpdateObject(const T* co, bool init);
protected:
	void UpdateCommon();
	virtual void UpdateObjectDrawFlags(CSolidObject* o) const = 0;
private:
	void UpdateObjectSMMA(const T* o);
	void UpdateObjectUniforms(const T* o);
public:
	const std::vector<T*>& GetUnsortedObjects() const { return unsortedObjects; }
	const ModelRenderContainer<T>& GetModelRenderer(int modelType) const { return modelRenderers[modelType]; }

	const ScopedMatricesMemAlloc& GetObjectMatricesMemAlloc(const T* o) const {
		const auto it = matricesMemAllocs.find(const_cast<T*>(o));
		return (it != matricesMemAllocs.end()) ? it->second : ScopedMatricesMemAlloc::Dummy();
	}
	ScopedMatricesMemAlloc& GetObjectMatricesMemAlloc(const T* o) { return matricesMemAllocs[const_cast<T*>(o)]; }
private:
	static constexpr int MMA_SIZE0 = 2 << 16;
protected:
	std::array<ModelRenderContainer<T>, MODELTYPE_CNT> modelRenderers;

	std::vector<T*> unsortedObjects;
	std::unordered_map<T*, ScopedMatricesMemAlloc> matricesMemAllocs;

	bool& mtModelDrawer;
};

using CUnitDrawerDataBase = CModelDrawerDataBase<CUnit>;
using CFeatureDrawerDataBase = CModelDrawerDataBase<CFeature>;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline CModelDrawerDataBase<T>::CModelDrawerDataBase(const std::string& ecName, int ecOrder, bool& mtModelDrawer_)
	: CModelDrawerDataConcept(ecName, ecOrder)
	, mtModelDrawer(mtModelDrawer_)
{
	matricesMemAllocs.reserve(MMA_SIZE0);
}

template<typename T>
inline CModelDrawerDataBase<T>::~CModelDrawerDataBase()
{
	unsortedObjects.clear();
	matricesMemAllocs.clear();
}

template<typename T>
inline void CModelDrawerDataBase<T>::AddObject(const T* co, bool add)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)].AddObject(o);
	}

	if (!add)
		return;

	unsortedObjects.emplace_back(o);

	const uint32_t numMatrices = (o->model ? o->model->numPieces : 0) + 1u;
	matricesMemAllocs.emplace(o, ScopedMatricesMemAlloc(numMatrices));

	modelsUniformsStorage.GetObjOffset(co);
}

template<typename T>
inline void CModelDrawerDataBase<T>::DelObject(const T* co, bool del)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)].DelObject(o);
	}

	if (del && spring::VectorErase(unsortedObjects, o)) {
		matricesMemAllocs.erase(o);
		modelsUniformsStorage.GetObjOffset(co);
	}
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObject(const T* co, bool init)
{
	DelObject(co, false);
	AddObject(co, init );
}


template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObjectSMMA(const T* o)
{
	ScopedMatricesMemAlloc& smma = GetObjectMatricesMemAlloc(o);
	smma[0] = o->GetTransformMatrix();

	for (int i = 0; i < o->localModel.pieces.size(); ++i) {
		auto& lmp = o->localModel.pieces[i];

		if (unlikely(!lmp.scriptSetVisible)) {
			smma[i + 1] = CMatrix44f::Zero();
			continue;
		}

		smma[i + 1] = lmp.GetModelSpaceMatrix();
	}
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObjectUniforms(const T* o)
{
	auto& uni = modelsUniformsStorage.GetObjUniformsArray(o);
	uni.drawFlag = o->drawFlag;

	if (gu->spectatingFullView || o->IsInLosForAllyTeam(gu->myAllyTeam)) {
		uni.speed = o->speed;
		uni.maxHealth = o->maxHealth;
		uni.health = o->health;
	}
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateCommon()
{
	const auto updateBody = [this](int k) {
		T* o = unsortedObjects[k];
		UpdateObjectDrawFlags(o);

		if (o->alwaysUpdateMat || (o->drawFlag > DrawFlags::SO_NODRAW_FLAG && o->drawFlag < DrawFlags::SO_FARTEX_FLAG))
			this->UpdateObjectSMMA(o);

		this->UpdateObjectUniforms(o);
	};

	if (mtModelDrawer) {
		for_mt(0, unsortedObjects.size(), [&updateBody](int k) {
			updateBody(k);
		});
	}
	else {
		for (int k = 0; k < unsortedObjects.size(); ++k)
			updateBody(k);
	}
}