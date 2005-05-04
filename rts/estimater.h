#ifndef AFX_SQUARESC2CESTIMATER_H__EF1D75A1_1924_11D5_AD55_0080ADA84DE3__INCLUDED_
#define AFX_SQUARESC2CESTIMATER_H__EF1D75A1_1924_11D5_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable:4786)

#include <vector>
#include <list>
#include "pathfinder.h"
#include "estimater.h"

#define BLOCK_SIZE 32

class CPathFinder;

class  CPathEstimater
{
 public:
	 void SetPathType(int type);
  CPathEstimater(CPathFinder* pf);
  ~CPathEstimater();

  void SetGoal(int x,int y);
  float EstimateCost(int x,int y);
  vector<int2> GetEstimatedPath(int x,int y);
 private:
  struct SquareCost {
    float nwcost;
    float ncost;
    float necost;
    float ecost;
  };
  SquareCost squareCosts[4][1024/BLOCK_SIZE][1024/BLOCK_SIZE];		//fix
  struct CoarseEstimate {
    float cost;
    int status;
  };
  CoarseEstimate squareEstimates[1024/BLOCK_SIZE][1024/BLOCK_SIZE];		//fix

  int2 centers[4][1024/BLOCK_SIZE][1024/BLOCK_SIZE];		//fix

	int pathType;

  void CalculateSquareCosts();
  void ResetEstimates();
  void CalculateEstimates();
  void TestSquare(int x,int y,float oldcost,int dir);

  int goalx;
  int goaly;
  CPathFinder* pf;
  CPathFinder::myPQ openSquares;
  std::vector<int> dirtyEstimates;
  bool initializing;
};


#endif

