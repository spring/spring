/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDRAWER_STATE_H
#define UNITDRAWER_STATE_H

#include <vector>

class CUnitDrawer;
struct ISkyLight;

namespace Shader {
	struct IProgramObject;
}

struct IUnitDrawerState {
public:
	static IUnitDrawerState* GetInstance(bool haveARB, bool haveGLSL);
	static void FreeInstance(IUnitDrawerState* state) { delete state; }

	virtual bool Init(const CUnitDrawer*) { return false; }
	virtual void Kill() {}

	virtual bool CanEnable(const CUnitDrawer*) const { return false; }

	virtual void Enable(const CUnitDrawer*) {}
	virtual void Disable(const CUnitDrawer*) {}

	virtual void EnableTextures(const CUnitDrawer*) {}
	virtual void DisableTextures(const CUnitDrawer*) {}
	virtual void EnableShaders(const CUnitDrawer*) {}
	virtual void DisableShaders(const CUnitDrawer*) {}

	virtual void UpdateCurrentShader(const CUnitDrawer*, const ISkyLight*) const {}
	virtual void SetTeamColor(int team, float alpha = 1.0f) const {}

	enum ModelShaderProgram {
		MODEL_SHADER_BASIC  = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOW = 1, ///< model shader (V+F) with self-shadowing
		MODEL_SHADER_ACTIVE = 2, ///< currently active model shader
		MODEL_SHADER_LAST   = 3,
	};

protected:
	// shared ARB and GLSL state managers
	void EnableCommon(const CUnitDrawer*);
	void DisableCommon(const CUnitDrawer*);
	void EnableTexturesCommon(const CUnitDrawer*);
	void DisableTexturesCommon(const CUnitDrawer*);

protected:
	std::vector<Shader::IProgramObject*> modelShaders;
};




struct UnitDrawerStateFFP: public IUnitDrawerState {
public:
	bool CanEnable(const CUnitDrawer*) const;

	void Enable(const CUnitDrawer*);
	void Disable(const CUnitDrawer*);

	void EnableTextures(const CUnitDrawer*);
	void DisableTextures(const CUnitDrawer*);

	void SetTeamColor(int team, float alpha = 1.0f) const;
};


struct UnitDrawerStateARB: public IUnitDrawerState {
public:
	bool Init(const CUnitDrawer*);
	void Kill();

	bool CanEnable(const CUnitDrawer*) const;

	void Enable(const CUnitDrawer*);
	void Disable(const CUnitDrawer*);

	void EnableTextures(const CUnitDrawer*);
	void DisableTextures(const CUnitDrawer*);
	void EnableShaders(const CUnitDrawer*);
	void DisableShaders(const CUnitDrawer*);

	void SetTeamColor(int team, float alpha = 1.0f) const;
};


struct UnitDrawerStateGLSL: public IUnitDrawerState {
public:
	bool Init(const CUnitDrawer*);
	void Kill();

	bool CanEnable(const CUnitDrawer*) const;

	void Enable(const CUnitDrawer*);
	void Disable(const CUnitDrawer*);

	void EnableTextures(const CUnitDrawer*);
	void DisableTextures(const CUnitDrawer*);
	void EnableShaders(const CUnitDrawer*);
	void DisableShaders(const CUnitDrawer*);

	void UpdateCurrentShader(const CUnitDrawer*, const ISkyLight*) const;
	void SetTeamColor(int team, float alpha = 1.0f) const;
};

#endif

