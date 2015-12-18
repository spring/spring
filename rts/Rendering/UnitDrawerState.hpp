/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDRAWER_STATE_H
#define UNITDRAWER_STATE_H

#include <vector>

class CUnitDrawer;
struct ISkyLight;

namespace Shader {
	struct IProgramObject;
}

enum {
	DRAWER_STATE_FFP = 0, // fixed-function path
	DRAWER_STATE_SSP = 1, // standard-shader path (ARB/GLSL)
	DRAWER_STATE_SEL = 2, // selected path
	DRAWER_STATE_CNT = 3,
};


struct IUnitDrawerState {
public:
	static IUnitDrawerState* GetInstance(bool haveARB, bool haveGLSL);
	static void FreeInstance(IUnitDrawerState* state) { delete state; }

	virtual ~IUnitDrawerState() {}

	virtual bool Init(const CUnitDrawer*) { return false; }
	virtual void Kill() {}

	virtual bool CanEnable(const CUnitDrawer*) const { return false; }
	virtual bool CanDrawDeferred() const { return false; }

	virtual void Enable(const CUnitDrawer*, bool) {}
	virtual void Disable(const CUnitDrawer*, bool) {}

	virtual void EnableTextures(const CUnitDrawer*) {}
	virtual void DisableTextures(const CUnitDrawer*) {}
	virtual void EnableShaders(const CUnitDrawer*) {}
	virtual void DisableShaders(const CUnitDrawer*) {}

	virtual void UpdateCurrentShader(const CUnitDrawer*, const ISkyLight*) const {}
	virtual void SetTeamColor(int team, float alpha) const {}

	static void SetBasicTeamColor(int team, float alpha);

	void SetActiveShader(bool shadowed, bool deferred) {
		if (shadowed) {
			modelShaders[MODEL_SHADER_ACTIVE] = (deferred? modelShaders[MODEL_SHADER_SHADOWED_DEFERRED]: modelShaders[MODEL_SHADER_SHADOWED_STANDARD]);
		} else {
			modelShaders[MODEL_SHADER_ACTIVE] = (deferred? modelShaders[MODEL_SHADER_NOSHADOW_DEFERRED]: modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]);
		}
	}

	enum ModelShaderProgram {
		MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with self-shadowing
		MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
		MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW (GLSL-only)

		MODEL_SHADER_ACTIVE            = 4, ///< currently active model shader
		MODEL_SHADER_COUNT             = 5,
	};

protected:
	// shared ARB and GLSL state managers
	void EnableCommon(const CUnitDrawer*, bool);
	void DisableCommon(const CUnitDrawer*, bool);
	void EnableTexturesCommon(const CUnitDrawer*);
	void DisableTexturesCommon(const CUnitDrawer*);

protected:
	std::vector<Shader::IProgramObject*> modelShaders;
};




struct UnitDrawerStateFFP: public IUnitDrawerState {
public:
	bool CanEnable(const CUnitDrawer*) const;

	void Enable(const CUnitDrawer*, bool);
	void Disable(const CUnitDrawer*, bool);

	void EnableTextures(const CUnitDrawer*);
	void DisableTextures(const CUnitDrawer*);

	void SetTeamColor(int team, float alpha) const;
};


struct UnitDrawerStateARB: public IUnitDrawerState {
public:
	bool Init(const CUnitDrawer*);
	void Kill();

	bool CanEnable(const CUnitDrawer*) const;

	void Enable(const CUnitDrawer*, bool);
	void Disable(const CUnitDrawer*, bool);

	void EnableTextures(const CUnitDrawer*);
	void DisableTextures(const CUnitDrawer*);
	void EnableShaders(const CUnitDrawer*);
	void DisableShaders(const CUnitDrawer*);

	void SetTeamColor(int team, float alpha) const;
};


struct UnitDrawerStateGLSL: public IUnitDrawerState {
public:
	bool Init(const CUnitDrawer*);
	void Kill();

	bool CanEnable(const CUnitDrawer*) const;
	bool CanDrawDeferred() const { return true; }

	void Enable(const CUnitDrawer*, bool);
	void Disable(const CUnitDrawer*, bool);

	void EnableTextures(const CUnitDrawer*);
	void DisableTextures(const CUnitDrawer*);
	void EnableShaders(const CUnitDrawer*);
	void DisableShaders(const CUnitDrawer*);

	void UpdateCurrentShader(const CUnitDrawer*, const ISkyLight*) const;
	void SetTeamColor(int team, float alpha) const;
};

#endif

