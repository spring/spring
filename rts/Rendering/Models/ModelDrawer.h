/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_DRAWER_HDR
#define MODEL_DRAWER_HDR

#include <map>
#include <set>

#include "System/EventClient.h"

namespace Shader {
	struct IProgramObject;
}

class CUnit;
class CFeature;
class CProjectile;
class IWorldObjectModelRenderer;

class IModelDrawer: public CEventClient {
public:
	static IModelDrawer* GetInstance();

	IModelDrawer(const std::string& name, int order, bool synced);
	virtual ~IModelDrawer();
	virtual void Draw();

	// NOTE: GML synchronization points
	virtual void RenderUnitCreated(const CUnit* u, int cloaked);
	virtual void RenderUnitDestroyed(const CUnit* u);
	virtual void RenderUnitCloakChanged(const CUnit* u, int cloaked);
	virtual void RenderFeatureCreated(const CFeature* f);
	virtual void RenderFeatureDestroyed(const CFeature* f);
	virtual void RenderProjectileCreated(const CProjectile* p);
	virtual void RenderProjectileDestroyed(const CProjectile* p);

	virtual bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "RenderUnitCreated" || eventName == "RenderUnitDestroyed") ||
			(eventName == "RenderUnitCloakChanged") ||
			(eventName == "RenderFeatureCreated" || eventName == "RenderFeatureDestroyed") ||
			(eventName == "RenderProjectileCreated" || eventName == "RenderProjectileDestroyed");
	}
	virtual bool GetFullRead() const { return true; }
	virtual int GetReadAllyTeam() const { return AllAccessTeam; }

protected:
	virtual bool LoadModelShaders() { return false; }
	virtual void PushDrawState(int modelType) {}
	virtual void PopDrawState(int modelType) {}

	std::map<int, std::vector<Shader::IProgramObject*> > shaders;

	std::vector<IWorldObjectModelRenderer*> opaqueModelRenderers;
	std::vector<IWorldObjectModelRenderer*> cloakedModelRenderers;
};



class CModelDrawerFFP: public IModelDrawer {
public:
	CModelDrawerFFP(const std::string& name, int order, bool synced);
	~CModelDrawerFFP() {}
};



class CModelDrawerARB: public IModelDrawer {
public:
	CModelDrawerARB(const std::string& name, int order, bool synced);
	~CModelDrawerARB();

protected:
	bool LoadModelShaders();
};



class CModelDrawerGLSL: public IModelDrawer {
public:
	CModelDrawerGLSL(const std::string& name, int order, bool synced);
	~CModelDrawerGLSL();

protected:
	bool LoadModelShaders();
	void PushDrawState(int modelType);
	void PopDrawState(int modelType);
};



extern IModelDrawer* modelDrawer;

#endif // MODEL_DRAWER_HDR
