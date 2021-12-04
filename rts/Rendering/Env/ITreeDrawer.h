/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _I_TREE_DRAWER_H_
#define _I_TREE_DRAWER_H_

#include <vector>

#include "Rendering/GL/myGL.h"
#include "System/EventClient.h"
#include "System/float3.h"
#include "System/Matrix44f.h"

constexpr          int TREE_SQUARE_SIZE =   64;
constexpr unsigned int   NUM_TREE_TYPES =    8; // number of variants per type (pine or bush)

class CFeature;
class ITreeDrawer : public CEventClient
{
public:
	ITreeDrawer();
	virtual ~ITreeDrawer();

	static ITreeDrawer* GetTreeDrawer();

	void SetupState() const;
	void ResetState() const;
	void Draw();
	void DrawShadow();
	void DrawTree(const CFeature* f, bool setupState, bool resetState);

	void ConfigNotify(const std::string& key, const std::string& value);

	virtual void DrawPass() = 0;
	virtual void DrawShadowPass() = 0;
	virtual void Update() override {}

	virtual void AddTree(int treeID, int treeType, const float3& pos, float size);
	virtual void DeleteTree(int treeID, int treeType, const float3& pos);
	virtual void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir) {}

	bool GetFullRead() const override { return true; }
	bool WantsEvent(const std::string& eventName) override {
		return
			(eventName == "RenderFeatureCreated") ||
			(eventName == "FeatureMoved") ||
			(eventName == "RenderFeatureDestroyed");
	}

	float GetBaseDistance() const { return baseTreeDistance; }
	float GetDrawDistance() const { return drawTreeDistance; }
	float IncrDrawDistance();
	float DecrDrawDistance();

	int NumTreesX() const { return treesX; }
	int NumTreesY() const { return treesY; }

	bool DefDrawTrees() const { return defDrawTrees; }
	bool LuaDrawTrees() const { return luaDrawTrees; }
	bool& WireFrameModeRef() { return wireFrameMode; }

	void HandleAction(int arg) {
		switch (arg) {
			case -1: { defDrawTrees = !defDrawTrees; luaDrawTrees = !luaDrawTrees; } break;
			case  0: { defDrawTrees =         false; luaDrawTrees =         false; } break;
			case  1: { defDrawTrees =          true; luaDrawTrees =         false; } break;
			case  2: { defDrawTrees =         false; luaDrawTrees =          true; } break;
			default: {                                                             } break;
		}
	}

	void RenderFeatureCreated(const CFeature* feature) override;
	void FeatureMoved(const CFeature* feature, const float3& oldpos) override;
	void RenderFeatureDestroyed(const CFeature* feature) override;

public:
	struct TreeStruct {
	public:
		bool operator == (const TreeStruct& ts) const { return (id == ts.id); }
		bool operator < (const TreeStruct& ts) const { return (type < ts.type); }
	public:
		int id;
		int type;

		CMatrix44f mat;
	};

	struct TreeSquareStruct {
		// all trees within this tree-square, split by type (bush or pine)
		// [0] stores trees with type < 8, [1] stores trees with type >= 8
		std::vector<TreeStruct> trees[2];
	};

	std::vector<TreeSquareStruct> treeSquares;

private:
	void AddTrees();

protected:
	float baseTreeDistance;
	float drawTreeDistance;

	int treesX;
	int treesY;
	int nTrees;

	bool defDrawTrees;
	bool luaDrawTrees;
	bool wireFrameMode;

};

extern ITreeDrawer* treeDrawer;

#endif // _I_TREE_DRAWER_H_

