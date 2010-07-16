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

	IModelDrawer(const std::string&, int, bool);
	virtual ~IModelDrawer();
	virtual void Draw();

	// NOTE: GML synchronization points
	virtual void RenderUnitCreated(const CUnit*, int cloaked);
	virtual void RenderUnitDestroyed(const CUnit*);
	virtual void RenderUnitCloakChanged(const CUnit*, int cloaked);
	virtual void RenderFeatureCreated(const CFeature*);
	virtual void RenderFeatureDestroyed(const CFeature*);
	virtual void RenderProjectileCreated(const CProjectile*);
	virtual void RenderProjectileDestroyed(const CProjectile*);

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
	virtual void PushDrawState(int) {}
	virtual void PopDrawState(int) {}

	std::map<int, std::vector<Shader::IProgramObject*> > shaders;

	std::vector<IWorldObjectModelRenderer*> opaqueModelRenderers;
	std::vector<IWorldObjectModelRenderer*> cloakedModelRenderers;
};



class CModelDrawerFFP: public IModelDrawer {
public:
	CModelDrawerFFP(const std::string&, int, bool);
	~CModelDrawerFFP() {}
};



class CModelDrawerARB: public IModelDrawer {
public:
	CModelDrawerARB(const std::string&, int, bool);
	~CModelDrawerARB();

protected:
	bool LoadModelShaders();
};



class CModelDrawerGLSL: public IModelDrawer {
public:
	CModelDrawerGLSL(const std::string&, int, bool);
	~CModelDrawerGLSL();

protected:
	bool LoadModelShaders();
	void PushDrawState(int);
	void PopDrawState(int);
};



extern IModelDrawer* modelDrawer;

#endif
