#ifndef KAIK_PATHFINDER_HDR
#define KAIK_PATHFINDER_HDR

#include <string>
#include <vector>
#include "MicroPather.h"

struct AIClasses;
using namespace NSMicroPather;

typedef std::vector<float3> F3Vec;

class CPathFinder: public Graph {
	public:
		CPathFinder(AIClasses* ai);
		~CPathFinder();

		void Init();
		void* XY2Node(int x, int y);
		void Node2XY(void* node, int* x, int* y);
		float3 Node2Pos(void* node);
		void* Pos2Node(float3 pos);

		unsigned Checksum();
		float MakePath(F3Vec& posPath, float3& startPos, float3& endPos, int radius);
		float FindBestPath(F3Vec& posPath, float3& startPos, float myMaxRange, F3Vec& possibleTargets);
		float FindBestPathToRadius(F3Vec& posPath, float3& startPos, float radiusAroundTarget, const float3& target);

		void CreateDefenseMatrix();

	//	virtual void PrintStateInfo(void* state) {
	//		state = state;
	//	}

		void PrintData(std::string s);
		MicroPather* micropather;
		bool* TestMoveArray;
		std::vector<bool*> MoveArrays;
		int NumOfMoveTypes;

		std::vector<float> SlopeMap;
		std::vector<float> HeightMap;
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
