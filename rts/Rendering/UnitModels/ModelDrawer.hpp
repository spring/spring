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
class IModelDrawer: public CEventClient {
public:
	IModelDrawer(const std::string&, int, bool);
	virtual ~IModelDrawer();
	virtual void Draw();

	static IModelDrawer* GetInstance();

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
	virtual void PushRenderState(int) {}
	virtual void PopRenderState(int) {}
	virtual void DrawModels(const std::set<const CUnit*>&);
	virtual void DrawModels(const std::set<const CFeature*>&);
	virtual void DrawModels(const std::set<const CProjectile*>&);
	virtual void DrawModel(const CUnit*) {}
	virtual void DrawModel(const CFeature*) {}
	virtual void DrawModel(const CProjectile*) {}


	std::map<int, std::vector<Shader::IProgramObject*> > shaders;

	typedef std::set<const CUnit*>                       UnitSet;
	typedef std::set<const CUnit*>::const_iterator       UnitSetIt;
	typedef std::set<const CFeature*>                    FeatureSet;
	typedef std::set<const CFeature*>::const_iterator    FeatureSetIt;
	typedef std::set<const CProjectile*>                 ProjectileSet;
	typedef std::set<const CProjectile*>::const_iterator ProjectileSetIt;

	typedef std::map<int, UnitSet>                       UnitRenderBin;
	typedef std::map<int, UnitSet>::const_iterator       UnitRenderBinIt;
	typedef std::map<int, FeatureSet>                    FeatureRenderBin;
	typedef std::map<int, FeatureSet>::const_iterator    FeatureRenderBinIt;
	typedef std::map<int, ProjectileSet>                 ProjectileRenderBin;
	typedef std::map<int, ProjectileSet>::const_iterator ProjectileRenderBinIt;

	std::map<int, UnitRenderBin>       opaqueUnits;
	std::map<int, FeatureRenderBin>    opaqueFeatures;     // these use GridVisibility for culling
	std::map<int, ProjectileRenderBin> opaqueProjectiles;  // (synced && (piece || weapon)) projectiles only
	std::map<int, UnitRenderBin>       cloakedUnits;
	std::map<int, FeatureRenderBin>    cloakedFeatures;    // these don't exist
	std::map<int, ProjectileRenderBin> cloakedProjectiles; // these don't exist
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
