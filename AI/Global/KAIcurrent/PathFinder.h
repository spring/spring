#pragma once
#include "GlobalAI.h"

using namespace micropather;
class CPathFinder
{
public:
	CPathFinder(AIClasses *ai);
	virtual ~CPathFinder();
	void Init();
	void ClearPath();
	unsigned Checksum();
	
	unsigned XY2Node_U(int x, int y);
	void Node2XY_U(unsigned node, int* x, int* y);
	float3 Node2Pos_U(unsigned node);
	unsigned Pos2Node_U(float3* pos);

//remade the pathfinder interface, edit other functions as appropriate (there are duplicates):
	float CPathFinder::PathToPos(vector<float3>* pathToPos, float3 startPos, float3 target);
	float CPathFinder::PathToPosRadius(vector<float3>* pathToPos, float3 startPos, float3 target, float radius);
	float CPathFinder::PathToSet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets);
	float CPathFinder::PathToSet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float threatCutoff);
	float CPathFinder::PathToSetRadius(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float radius);
	float CPathFinder::PathToSetRadius(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float radius, float threatCutoff);
	bool  CPathFinder::PathExists(float3 startPos, float3 target);
	bool CPathFinder::PathExistsToAny(float3 startPos, vector<float3> targets);
	float CPathFinder::ManeuverToPos(float3* destination, float3 startPos, float3 target);
	float CPathFinder::ManeuverToPosRadius(float3* destination, float3 startPos, float3 target, float radius);
	float CPathFinder::ManeuverToPosRadiusAndCanFire(float3* destination, float3 startPos, float3 target, float radius);
	float CPathFinder::PathToPrioritySet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets);
	float CPathFinder::PathToPrioritySet(vector<float3>* pathToPos, float3 startPos, vector<float3>* possibleTargets, float threatCutoff);
	void CreateDefenseMatrix();

	MicroPather* micropather;
	bool* TestMoveArray;
	//vector<bool*> MoveArrays;	
	vector<KAIMoveType> MoveTypes;
	list<float> slopeTypes; // A list of all the used slope types. Sorted, most restricted first.
	list<float> groundUnitWaterLines; // A list of all ground units water tolerance. Sorted, most restricted first.
	list<float> waterUnitGroundLines; // A list of all water units ground tolerance. Sorted, most restricted first.
	list<float> crushStrengths; // A list of all units crush strengths. Sorted, most restricted first.

	int NumOfMoveTypes;

	float* SlopeMap;
	float* HeightMap;
	unsigned* canMoveIntMaskArray;
	int PathMapXSize;
	int PathMapYSize;
	int totalcells;
	double AverageHeight;
	vector<int> CumulativeSlopeMeter;
private:

	//moved here for debug purposes
	float FindBestPath(vector<float3> *posPath, float3 *startPos, float myMaxRange, vector<float3> *possibleTargets, float cutoff = 0);
	//added for convenience:
	float FindBestPathToRadius(vector<float3> *posPath, float3 *startPos, float radiusAroundTarget, float3 *target, float cutoff = 0);
//	float CPathFinder::MakePath (float3 mypos, float3 pos);

	int patherTime; // The time group number for paths
	
	std::vector<unsigned> path;
	float totalcost;
	
	float resmodifier;
	AIClasses *ai;	

};
