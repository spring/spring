/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MODEL_DRAWER_HDR
#define MODEL_DRAWER_HDR

#include <map>

#include "System/EventClient.h"

namespace Shader {
	struct IProgramObject;
}


class CUnit;
class CFeature;
class CProjectile;
class IModelDrawer: public CEventClient {
public:
	IModelDrawer(const std::string&, int, bool);
	virtual ~IModelDrawer();
	virtual void Draw();

	static IModelDrawer* GetInstance();

	// NOTE: listen to Unit*Cloaked for tracking transparent objects?
	// NOTE: GML synchronization points
	virtual void UnitCreated(const CUnit*, const CUnit*);
	virtual void UnitDestroyed(const CUnit*, const CUnit*);
	virtual void FeatureCreated(const CFeature*);
	virtual void FeatureDestroyed(const CFeature*);
	virtual void ProjectileCreated(const CProjectile*);
	virtual void ProjectileDestroyed(const CProjectile*);

	virtual bool WantsEvent(const std::string& eventName) {
		return
			(eventName ==       "UnitCreated" || eventName ==       "UnitDestroyed") ||
			(eventName ==    "FeatureCreated" || eventName ==    "FeatureDestroyed") ||
			(eventName == "ProjectileCreated" || eventName == "ProjectileDestroyed");
	}
	virtual bool GetFullRead() const { return true; }
	virtual int GetReadAllyTeam() const { return AllAccessTeam; }

protected:
	virtual bool LoadModelShaders() { return false; }
	virtual void PushRenderState(int) {}
	virtual void PopRenderState(int) {}
	virtual void DrawModels(const std::set<const CUnit*>&);
	virtual void DrawModels(const std::set<const CFeature*>&);
	virtual void DrawModels(const std::set<const CProjectile*>&);
	virtual void DrawModel(const CUnit*) {}
	virtual void DrawModel(const CFeature*) {}
	virtual void DrawModel(const CProjectile*) {}


	std::map<int, std::vector<Shader::IProgramObject*> > shaders;

	std::map<int, std::set<const CUnit*> > renderableUnits;
	std::map<int, std::set<const CFeature*> > renderableFeatures;
	std::map<int, std::set<const CProjectile*> > renderableProjectiles;
};



class CModelDrawerFFP: public IModelDrawer {
public:
	CModelDrawerFFP(const std::string&, int, bool);
	void Draw() {}
};



class CModelDrawerARB: public IModelDrawer {
public:
	CModelDrawerARB(const std::string&, int, bool);
	void Draw() {}
};



class CModelDrawerGLSL: public IModelDrawer {
public:
	CModelDrawerGLSL(const std::string&, int, bool);
	void Draw();

	void UnitCreated(const CUnit*, const CUnit*);
	void UnitDestroyed(const CUnit*, const CUnit*);
	void FeatureCreated(const CFeature*);
	void FeatureDestroyed(const CFeature*);
	void ProjectileCreated(const CProjectile*);
	void ProjectileDestroyed(const CProjectile*);

protected:
	bool LoadModelShaders();
	void PushRenderState(int);
	void PopRenderState(int);
	void DrawModels(const std::set<const CUnit*>&);
	void DrawModels(const std::set<const CFeature*>&);
	void DrawModels(const std::set<const CProjectile*>&);

	void DrawModel(const CUnit*);
	void DrawModel(const CFeature*);
	void DrawModel(const CProjectile*);
};


extern IModelDrawer* modelDrawer;

#endif
