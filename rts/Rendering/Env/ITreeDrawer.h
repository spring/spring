/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _I_TREE_DRAWER_H_
#define _I_TREE_DRAWER_H_

#include <vector>

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
	virtual void DrawShadowPass();
	virtual void Update() = 0;

	virtual void ResetPos(const float3& pos);
	virtual void AddTree(int treeID, int treeType, const float3& pos, float size);
	virtual void DeleteTree(int treeID, const float3& pos);
	virtual void AddFallingTree(int treeID, int treeType, const float3& pos, const float3& dir) {}

	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "RenderFeatureCreated") ||
			(eventName == "FeatureMoved") ||
			(eventName == "RenderFeatureDestroyed");
	}

	void RenderFeatureCreated(const CFeature* feature);
	void FeatureMoved(const CFeature* feature, const float3& oldpos);
	void RenderFeatureDestroyed(const CFeature* feature);

public:
	int treesX;
	int treesY;
	int nTrees;


public:
	float baseTreeDistance;
	bool drawTrees;

	struct TreeStruct {
	public:
		bool operator == (const TreeStruct& ts) const { return (id == ts.id); }
	public:
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
			, viewVector(UpVector)
		{}

		unsigned int dispList;
		unsigned int farDispList;

		int lastSeen;
		int lastSeenFar;

		float3 viewVector;

		// all trees within this tree-square
		std::vector<TreeStruct> trees;
	};

	std::vector<TreeSquareStruct> treeSquares;
	std::vector<GLuint> delDispLists;

private:
	void AddTrees();
};

extern ITreeDrawer* treeDrawer;

#endif // _I_TREE_DRAWER_H_

