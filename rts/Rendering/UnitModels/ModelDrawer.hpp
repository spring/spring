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
	virtual void UnitCreated(const CUnit*, const CUnit*);
	virtual void UnitDestroyed(const CUnit*, const CUnit*);
	virtual void UnitCloaked(const CUnit*);
	virtual void UnitDecloaked(const CUnit*);
	virtual void FeatureCreated(const CFeature*);
	virtual void FeatureDestroyed(const CFeature*);
	virtual void ProjectileCreated(const CProjectile*);
	virtual void ProjectileDestroyed(const CProjectile*);

	virtual bool WantsEvent(const std::string& eventName) {
		return
			(eventName ==       "UnitCreated" || eventName ==       "UnitDestroyed") ||
			(eventName ==       "UnitCloaked" || eventName ==       "UnitDecloaked") ||
			(eventName ==    "FeatureCreated" || eventName ==    "FeatureDestroyed") ||
			(eventName == "ProjectileCreated" || eventName == "ProjectileDestroyed");
	}
	virtual bool GetFullRead() const { return true; }
	virtual int GetReadAllyTeam() const { return AllAccessTeam; }

protected:
	virtual bool LoadModelShaders() { return false; }
	virtual void PushDrawState(int) {}
	virtual void PopDrawState(int) {}

	std::map<int, std::vector<Shader::IProgramObject*> > shaders;

	std::vector<IWorldObjectModelRenderer*> opaqueModels;
	std::vector<IWorldObjectModelRenderer*> cloakedModels;
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
