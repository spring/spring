#ifndef MULTIPATH_H
#define MULTIPATH_H
#if !defined(AFX_MULTIPATHFINDER_H__EF1D75A1_1924_11D5_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_MULTIPATHFINDER_H__EF1D75A1_1924_11D5_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#pragma warning(disable:4786)

#include <windows.h>		// Header File For Windows

#include <map>
#include <vector>
#include "MoveInfo.h"

class CTerrainReader;
class CPathFinder;
class CPathEstimater;
class CPathEstimater2;
class CBuilding;
class CUnit;
using namespace std;

class CMultiPath
{
 public:
	void Draw();
	void DeletePath(int pathnum);
	float3 GetNextWaypoint(int pathnum);
	float3 GetNextWaypointSub(int pathnum);
	int RequestPath(float3 start,float3 goal,int pathType,int unitSize,CUnit* caller);

	void ValidatePath(float3& start, float3& end);
	float GetPathLength(const float3& start, const float3& goal, int pathType);
	float GetApproximatePathLength(const float3& start, const float3& goal, int pathType);
	void TerrainChanged(int x1, int y1, int x2, int y2);

	static CMultiPath* Instance();
  virtual ~CMultiPath();
	
	CPathFinder* pf;

 private:
  CMultiPath();
  
	CPathEstimater* pe;
	CPathEstimater2* pe2;

	struct PathRequest {
		vector<int2> roughPath;
		vector<int2> roughPath2;
		vector<int2> detailPath;
		MoveData* movedata;
		int2 currentPos;
		float3 goal;
		CUnit* caller;
	};
	map<int,PathRequest> requests;
	int firstFreeRequest;
public:
	void Update(void);
	void UnitMoved(int oldSquare, int newSquare, int size);
};

extern CMultiPath* pathfinder;

#endif // !defined(AFX_PATHFINDER_H__EF1D75A1_1924_11D5_AD55_0080ADA84DE3__INCLUDED_)


#endif /* MULTIPATH_H */
