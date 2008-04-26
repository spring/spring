#ifndef PATHFINDER_H
#define PATHFINDER_H


#include "GlobalAI.h"

using std::vector;
using namespace NSMicroPather;

class CPathFinder: public Graph {
	public:
		CPathFinder(AIClasses* ai);
		~CPathFinder();

		void Init();
		void* XY2Node(int x, int y);
		void Node2XY(void* node, int* x, int* y);
		float3 Node2Pos(void* node);
		void* Pos2Node(float3 pos);

		void ClearPath();
		unsigned Checksum();
		float MakePath(vector<float3>* posPath, float3* startPos, float3* endPos, int radius);
		float FindBestPath(vector<float3>* posPath, float3* startPos, float myMaxRange, vector<float3>* possibleTargets);
		// added for convenience
		float FindBestPathToRadius(vector<float3>* posPath, float3* startPos, float radiusAroundTarget, float3* target);

		void CreateDefenseMatrix();

	//	virtual void PrintStateInfo(void* state) {
	//		state = state;
	//	}

		void PrintData(string s);
		MicroPather* micropather;
		bool* TestMoveArray;
		vector<bool*> MoveArrays;
		int NumOfMoveTypes;

		float* SlopeMap;
		float* HeightMap;
		int PathMapXSize;
		int PathMapYSize;
		int totalcells;
		double AverageHeight;

	private:
		std::vector<void*> path;
		float totalcost;
		float resmodifier;

		AIClasses* ai;
};


#endif
