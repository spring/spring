/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _I_TREE_DRAWER_H_
#define _I_TREE_DRAWER_H_

#include "Rendering/GL/myGL.h"
#include "System/EventClient.h"
#include "System/float3.h"

const int TREE_SQUARE_SIZE = 64;
static const float MID_TREE_DIST_FACTOR = 1.0f;
static const float FADE_TREE_DIST_FACTOR = 1.5f;
static const float FAR_TREE_DIST_FACTOR = 2.0f;

class ITreeDrawer : public CEventClient
{
public:
	ITreeDrawer();
	virtual ~ITreeDrawer();

	static ITreeDrawer* GetTreeDrawer();

	void Draw(bool drawReflection);
	virtual void Draw(float treeDistance, bool drawReflection) = 0;
	virtual void DrawGrass() {}
	virtual void DrawShadowGrass() {}
	virtual void Update() = 0;

	virtual void ResetPos(const float3& pos) = 0;

	virtual void AddTree(int treeID, int treeType, const float3& pos, float size) = 0;
	virtual void DeleteTree(int treeID, const float3& pos) = 0;

	virtual void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir) {}
	virtual void AddGrass(const float3& pos) {}
	virtual void RemoveGrass(int x, int z) {}
	virtual void DrawShadowPass();

	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "RenderFeatureMoved") ||
			(eventName == "RenderFeatureDestroyed");
	}
	void RenderFeatureMoved(const CFeature* feature, const float3& oldpos, const float3& newpos);
	void RenderFeatureDestroyed(const CFeature* feature);

	std::vector<GLuint> delDispLists;

	float baseTreeDistance;
	bool drawTrees;

	struct TreeStruct {
		int id;
		int type;

		float3 pos;
	};
	struct TreeSquareStruct {
		TreeSquareStruct()
			: dispList(0)
			, farDispList(0)
			, lastSeen(0)
			, lastSeenFar(0)
		{}

		unsigned int dispList;
		unsigned int farDispList;

		int lastSeen;
		int lastSeenFar;

		float3 viewVector;
		std::map<int, TreeStruct> trees;
	};

private:
	void AddTrees();
};

extern ITreeDrawer* treeDrawer;

#endif // _I_TREE_DRAWER_H_

