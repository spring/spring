#ifndef KAIK_PATHFINDER_HDR
#define KAIK_PATHFINDER_HDR

#include <string>
#include <vector>
#include "MicroPather.h"

struct AIClasses;
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
		float MakePath(std::vector<float3>* posPath, float3* startPos, float3* endPos, int radius);
		float FindBestPath(std::vector<float3>* posPath, float3* startPos, float myMaxRange, std::vector<float3>* possibleTargets);
		// added for convenience
		float FindBestPathToRadius(std::vector<float3>* posPath, float3* startPos, float radiusAroundTarget, float3* target);

		void CreateDefenseMatrix();

	//	virtual void PrintStateInfo(void* state) {
	//		state = state;
	//	}

		void PrintData(std::string s);
		MicroPather* micropather;
		bool* TestMoveArray;
		std::vector<bool*> MoveArrays;
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
