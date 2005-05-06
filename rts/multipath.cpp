#include "StdAfx.h"
// MultiPath.cpp: implementation of the CMultiPath class.
//
//////////////////////////////////////////////////////////////////////

#include "multipath.h"
#include <ostream>
#include "PathEstimator.h"
//#include "PathEstimater2.h"
#include "PathFinder.h"
#include "myGL.h"
#include "Ground.h"
#include "TimeProfiler.h"
#include "InfoConsole.h"
#include "multipath.h"
#include "Unit.h"
#include "UnitDefHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define BLOCKING_COST 100000000 

using namespace std;

//#define DISABLE_PATH

CMultiPath::CMultiPath()
{
	pf=CPathFinder::Instance();

#ifndef DISABLE_PATH
	pe=new CPathEstimater(pf);
	pe2=new CPathEstimater2(pf,pe);
#endif
	firstFreeRequest=1;
}

CMultiPath::~CMultiPath()
{
#ifndef DISABLE_PATH
	delete pe2;
	delete pe;
#endif
  delete pf;
}

CMultiPath* pathfinder=0;

CMultiPath* CMultiPath::Instance()
{
  if(pathfinder==0)
    pathfinder=new CMultiPath;
  return pathfinder;
}

int CMultiPath::RequestPath(float3 start,float3 goal,int pathType,int unitSize,CUnit* caller)
{
START_TIME_PROFILE

	ValidatePath(start,goal);

	PathRequest pr;
	pr.currentPos.x=start.x/(SQUARE_SIZE);
	pr.currentPos.y=start.z/(SQUARE_SIZE);
	pr.goal=goal;
	pr.goal.y=0;
	if(caller){
		pr.movedata=caller->unitDef->movedata;
	}else{
		pr.movedata=moveinfo->moveData[0];
	}

	pr.caller=caller;
	float sqdist=(start-goal).SqLength2D();

#ifndef DISABLE_PATH

	if(sqdist>100000){
		pe2->SetPathType(pr.movedata->pathType);
		pe2->SetGoal(goal.x/(SQUARE_SIZE),goal.z/(SQUARE_SIZE));
		pr.roughPath2=pe2->GetEstimatedPath(start.x/(SQUARE_SIZE),start.z/(SQUARE_SIZE));
		if(pr.roughPath2.size()>1)
			pr.roughPath2.pop_back();
		if(pr.roughPath2.size()>1)
			pr.roughPath2.pop_back();
		if(pr.roughPath2.size()>1)
			pr.roughPath2.pop_back();
	} else if(sqdist>10000){
		pe->SetPathType(pr.movedata->pathType);
		pe->SetGoal(goal.x/(SQUARE_SIZE),goal.z/(SQUARE_SIZE));
		pr.roughPath=pe->GetEstimatedPath(start.x/(SQUARE_SIZE),start.z/(SQUARE_SIZE));
		if(pr.roughPath.size()>1)
			pr.roughPath.pop_back();
		if(pr.roughPath.size()>1)
			pr.roughPath.pop_back();
	} else {
		pf->SetPathType(pr.movedata);
		pf->GetPath(start.x/(SQUARE_SIZE),start.z/(SQUARE_SIZE),goal.x/(SQUARE_SIZE),goal.z/(SQUARE_SIZE),30);
		pr.detailPath=pf->currentPath;
		if(caller)
			for(vector<int2>::iterator wi=pf->currentPath.begin();wi!=pf->currentPath.end();++wi)
				++pf->penalityMap[wi->y*g.mapx+wi->x];
	}
#endif

	requests[firstFreeRequest]=pr;

END_TIME_PROFILE("Pathfinder");

	return firstFreeRequest++;
}

float3 CMultiPath::GetNextWaypoint(int pathnum)
{
START_TIME_PROFILE

	float3 ret=GetNextWaypointSub(pathnum);

END_TIME_PROFILE("Pathfinder");

	return ret;
}

float3 CMultiPath::GetNextWaypointSub(int pathnum)
{
	map<int,PathRequest>::iterator pri;
	if((pri=(requests.find(pathnum)))==requests.end())
		return float3(0,-1,0);

	PathRequest& pr=pri->second;

#ifdef DISABLE_PATH
	return pr.goal;
#endif

	pf->SetPathType(pr.movedata);
	if(pr.detailPath.empty()){
		if(pr.roughPath.empty()){
			if(pr.roughPath2.empty()){
				return pr.goal;
			}
			int2* lastNode=&pr.roughPath2.back();

			pe->SetPathType(pr.movedata->pathType);
			pe->SetGoal(lastNode->x,lastNode->y);
			vector<int2> tempPath=pe->GetEstimatedPath(pr.currentPos.x,pr.currentPos.y);
			unsigned int ignore=8;
			if(pr.roughPath2.size()<=1)
				ignore=0;
			if(tempPath.size()>ignore){
				for(vector<int2>::iterator wi=tempPath.begin()+ignore;wi!=tempPath.end();++wi)
					pr.roughPath.push_back(*wi);
				pr.roughPath.pop_back();
			}
			pr.roughPath2.pop_back();
			if(pr.roughPath2.size()>1)
				pr.roughPath2.pop_back();
			if(pr.roughPath.empty())
				return float3(pr.goal.x,-2,pr.goal.z);
		}
		int2* lastNode=&pr.roughPath.back();
		
		pf->GetPath(pr.currentPos.x,pr.currentPos.y,lastNode->x,lastNode->y,20);
		unsigned int ignore=8;
		if(pr.roughPath2.empty() && pr.roughPath.size()==1)
			ignore=0;
		if(pf->currentPath.size()>ignore){
			for(vector<int2>::iterator wi=pf->currentPath.begin()+ignore;wi!=pf->currentPath.end();++wi){
				pr.detailPath.push_back(*wi);
				if(pr.caller)
					++pf->penalityMap[wi->y*g.mapx+wi->x];
			}
			pr.currentPos=pr.detailPath[0];
			if(pr.caller)
				--pf->penalityMap[pr.detailPath.back().y*g.mapx+pr.detailPath.back().x];
			pr.detailPath.pop_back();
		}
		pr.roughPath.pop_back();
		if(pr.roughPath.size()>1)
			pr.roughPath.pop_back();
		if(pr.detailPath.empty())
			return float3(pr.goal.x,-2,pr.goal.z);
	}
	if(!pr.detailPath.empty()){
		int2 wp=pr.detailPath.back();
		pr.detailPath.pop_back();
		if(pr.caller)
			--pf->penalityMap[wp.y*g.mapx+wp.x];
		float3 wp2(wp.x*(SQUARE_SIZE)+SQUARE_SIZE/2,20,wp.y*(SQUARE_SIZE)+SQUARE_SIZE/2);
		return wp2;
	}
	return float3(0,-1,0);
}

void CMultiPath::DeletePath(int pathnum)
{
	map<int,PathRequest>::iterator pri;
	if((pri=(requests.find(pathnum)))!=requests.end()){
		PathRequest* pr=&pri->second;
		if(pr->caller){
			for(vector<int2>::iterator wi=pr->detailPath.begin();wi!=pr->detailPath.end();++wi)
				--pf->penalityMap[wi->y*g.mapx+wi->x];			
		}
		requests.erase(pri);
	}
}

void CMultiPath::Draw()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	map<int,PathRequest>::iterator pri;
	for(pri=requests.begin();pri!=requests.end();pri++){
		PathRequest& pr=pri->second;
		vector<int2>::iterator pi;
		glBegin(GL_LINE_STRIP);
		glColor4f(0,0,0,1);
		glVertex3f(pr.goal.x,ground->GetHeight(pr.goal.x,pr.goal.z)+5,pr.goal.z);
		glColor4f(0,0,1,1);
		for(pi=pr.roughPath2.begin();pi!=pr.roughPath2.end();++pi){
			glVertex3f((pi->x)*(SQUARE_SIZE)+SQUARE_SIZE/2,ground->GetHeight(pi->x*(SQUARE_SIZE)+SQUARE_SIZE/2,pi->y*(SQUARE_SIZE)+SQUARE_SIZE/2)+5,pi->y*(SQUARE_SIZE)+SQUARE_SIZE/2);
		}
		glColor4f(0,1,0,1);
		for(pi=pr.roughPath.begin();pi!=pr.roughPath.end();++pi){
			glVertex3f((pi->x)*(SQUARE_SIZE)+SQUARE_SIZE/2,ground->GetHeight(pi->x*(SQUARE_SIZE)+SQUARE_SIZE/2,pi->y*(SQUARE_SIZE)+SQUARE_SIZE/2)+5,pi->y*(SQUARE_SIZE)+SQUARE_SIZE/2);
		}
		glColor4f(1,0,0,1);
		for(pi=pr.detailPath.begin();pi!=pr.detailPath.end();++pi){
			glVertex3f((pi->x)*(SQUARE_SIZE)+SQUARE_SIZE/2,ground->GetHeight(pi->x*(SQUARE_SIZE)+SQUARE_SIZE/2,pi->y*(SQUARE_SIZE)+SQUARE_SIZE/2)+5,pi->y*(SQUARE_SIZE)+SQUARE_SIZE/2);
		}
		glColor4f(1,1,1,1);
		if(pr.caller)
			glVertexf3(pr.caller->midPos);
		glEnd();
	}
}

void CMultiPath::ValidatePath(float3& start, float3& end)
{
#ifdef DISABLE_PATH
	return;
#endif

	if(start.x<0 || start.z<0 || start.z>g.mapy*SQUARE_SIZE || start.x>g.mapx*SQUARE_SIZE){
		info->AddLine("Invalid start of path %.0f %.0f",start.x,start.z);
		start.x=100;
		start.z=100;
	}
	if(end.x<0 || end.z<0 || end.z>g.mapy*SQUARE_SIZE || end.x>g.mapx*SQUARE_SIZE){
		info->AddLine("Invalid end of path %.0f %.0f",end.x,end.z);
		end.x=100;
		end.z=100;
	}
	if(start.x<SQUARE_SIZE*2) start.x=SQUARE_SIZE*2;
	if(start.z<SQUARE_SIZE*2) start.z=SQUARE_SIZE*2;
	if(end.x<SQUARE_SIZE*2) end.x=SQUARE_SIZE*2;
	if(end.z<SQUARE_SIZE*2) end.z=SQUARE_SIZE*2;

	if(start.x>(g.mapx-2)*SQUARE_SIZE) start.x=(g.mapx-2)*SQUARE_SIZE;
	if(start.z>(g.mapy-2)*SQUARE_SIZE) start.z=(g.mapy-2)*SQUARE_SIZE;
	if(end.x>(g.mapx-2)*SQUARE_SIZE) end.x=(g.mapx-2)*SQUARE_SIZE;
	if(end.z>(g.mapy-2)*SQUARE_SIZE) end.z=(g.mapy-2)*SQUARE_SIZE;

	int x=start.x/SQUARE_SIZE;
	int y=start.z/SQUARE_SIZE;
	if(pf->SquareCost(x+y*g.mapx)>900){
		float close=10000;
		int nx=x;
		int ny=y;
		for(int sy=y-1;sy<=y+1;++sy){
			for(int sx=x-1;sx<=x+1;++sx){
				if(pf->SquareCost(sx+sy*g.mapx)<900 && (start-float3(sx*SQUARE_SIZE+SQUARE_SIZE/2,0,sy*SQUARE_SIZE+SQUARE_SIZE/2)).SqLength2D()<close){
					nx=sx;
					ny=sy;
					close=(start-float3(sx*SQUARE_SIZE+SQUARE_SIZE/2,0,sy*SQUARE_SIZE+SQUARE_SIZE/2)).SqLength2D();
				}
			}
		}
		start=float3(nx*SQUARE_SIZE+SQUARE_SIZE/2,start.y,ny*SQUARE_SIZE+SQUARE_SIZE/2);
	}
}

float CMultiPath::GetPathLength(const float3& start, const float3& goal, int pathType)
{
#ifdef DISABLE_PATH
	return 100;
#endif

	int p=RequestPath(start,goal,pathType,4,0);
	float length=0;
	float3 node=GetNextWaypoint(p);
	float3 lastNode=node;
	if(node.y==-1){
		info->AddLine("Error getting path length %.0f",node.y);
		return 10;
	}
	while(node.y!=-1 && (node.y==-2 || (node-goal).SqLength2D()>150)){
		node=GetNextWaypoint(p);
		if(node.y!=-2){
			length+=lastNode.distance2D(node)*pf->SquareCost(node.x/8+(node.z/8)*g.mapx);	
			lastNode=node;
		}
	}
	DeletePath(p);
	return length/8;
}

float CMultiPath::GetApproximatePathLength(const float3& start, const float3& goal, int pathType)
{
	return GetPathLength(start,goal,pathType);
}

void CMultiPath::TerrainChanged(int x1, int y1, int x2, int y2)
{
	pe->TerrainChanged(x1,y1,x2,y2);
}

void CMultiPath::Update(void)
{
START_TIME_PROFILE

	pe->Update();
	pe2->Update();

END_TIME_PROFILE("Pathfinder Update");
}

void CMultiPath::UnitMoved(int oldSquare, int newSquare, int size)
{
	if(oldSquare!=-1){
		for(int y=-size;y<size;++y){
			for(int x=-size;x<size;++x){
				int square=oldSquare+y*g.mapx+x;
				if(square<0 || square>=g.mapx*g.mapy)
					continue;
				pf->penalityMap[square]-=3;
			}
		}
	}
	if(newSquare!=-1){
		for(int y=-size;y<size;++y){
			for(int x=-size;x<size;++x){
				int square=newSquare+y*g.mapx+x;
				if(square<0 || square>=g.mapx*g.mapy)
					continue;
				pf->penalityMap[square]+=3;
			}
		}
	}
}
