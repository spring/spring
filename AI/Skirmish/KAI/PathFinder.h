#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "GlobalAI.h"

using namespace NSMicroPather;

class CPathFinder {
	public:
//		CR_DECLARE(CPathFinder);
		CPathFinder(AIClasses *ai);
		virtual ~CPathFinder();
//		void PostLoad();

		void Init();
		void ClearPath();
		unsigned Checksum();
	
		unsigned XY2Node_U(int x, int y);
		void Node2XY_U(unsigned node, int* x, int* y);
		float3 Node2Pos_U(unsigned node);
		unsigned Pos2Node_U(float3* pos);

		// remade the pathfinder interface, edit other functions as appropriate (there are duplicates):
		float PathToPos(vector<float3>* pathToPos, float3 startPos, float3 target);
		float PathToPosRadius(vector<float3>* pathToPos, float3 startPos, float3 target, float radius);
		float PathToSet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets);
		float PathToSet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float threatCutoff);
		float PathToSetRadius(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float radius);
		float PathToSetRadius(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float radius, float threatCutoff);
		bool PathExists(float3 startPos, float3 target);
		bool PathExistsToAny(float3 startPos, vector<float3> targets);
		float ManeuverToPos(float3* destination, float3 startPos, float3 target);
		float ManeuverToPosRadius(float3* destination, float3 startPos, float3 target, float radius);
		float ManeuverToPosRadiusAndCanFire(float3* destination, float3 startPos, float3 target, float radius);
		float PathToPrioritySet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets);
		float PathToPrioritySet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float threatCutoff);
		void CreateDefenseMatrix();

		MicroPather* micropather;
		vector<bool> TestMoveArray;
		//vector<bool*> MoveArrays;	
		vector<KAIMoveType> MoveTypes;
		// list of all the used slope types. Sorted, most restricted first.
		list<float> slopeTypes;
		// list of all ground units water tolerance. Sorted, most restricted first.
		list<float> groundUnitWaterLines;
		// list of all water units ground tolerance. Sorted, most restricted first.
		list<float> waterUnitGroundLines;
		// list of all units crush strengths. Sorted, most restricted first.
		list<float> crushStrengths;

		int NumOfMoveTypes;

		vector<float> SlopeMap;
		vector<float> HeightMap;
		vector<unsigned> canMoveIntMaskArray;
		int PathMapXSize;
		int PathMapYSize;
		int totalcells;
		double AverageHeight;
		vector<int> CumulativeSlopeMeter;

	private:
		// moved here for debug purposes
		float FindBestPath(vector<float3>* posPath, float3* startPos, float myMaxRange, vector<float3>* possibleTargets, float cutoff = 0);
		// added for convenience:
		float FindBestPathToRadius(vector<float3>* posPath, float3* startPos, float radiusAroundTarget, float3* target, float cutoff = 0);

		// time group number for paths
		int patherTime;

		std::vector<unsigned> path;
		float totalcost;

		float resmodifier;
		AIClasses* ai;
};


#endif
