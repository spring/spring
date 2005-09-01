#include "StdAfx.h"
#include "estimater.h"
#include "myGL.h"
#include <fstream>
#include "ReadMap.h"
#include "FileHandler.h"
#include <boost/filesystem/path.hpp>
//#include "mmgr.h"

using namespace std;

#define MAX_COST 10000000

CPathEstimater::CPathEstimater(CPathFinder* pf)
:pf(pf)
{
	pathType=0;
  initializing=true;
  CalculateSquareCosts();
  initializing=false;
}

CPathEstimater::~CPathEstimater()
{
  openSquares.DeleteAll();
}

void CPathEstimater::SetGoal(int x,int y)
{
  goalx=x;
  goaly=y;
  ResetEstimates();
//  CalculateEstimates();
}

static int xg=0;
static int yg=0;

static inline void fBSR(float &x, register unsigned long shiftAmount) {
  *(unsigned long*)&x-=shiftAmount<<23;
}

float CPathEstimater::EstimateCost(int x,int y)
{
  int max=abs(x-goalx);
  int min=abs(y-goaly);
  if(max<min){
    int temp=max;
    max=min;
    min=temp;
  }
  
  xg=x/BLOCK_SIZE;
  yg=y/BLOCK_SIZE;
  CalculateEstimates();
  return squareEstimates[yg][xg].cost+(max+min*0.41)*0.5;
/*/
  yg=(y-8)/16;
  xg=(x-8)/16;
  int yp=y-8-(yg*16);
  int xp=x-8-(xg*16);
  float np=(squareEstimates[yg+1][xg].cost*(16-xp)+squareEstimates[yg+1][xg+1].cost*xp);
  fBSR(np,4);
  float sp=(squareEstimates[yg][xg].cost*(16-xp)+squareEstimates[yg][xg+1].cost*xp);
  fBSR(sp,4);
  return (sp*(16-yp)+np*yp)/16;
/**/
}

void CPathEstimater::CalculateSquareCosts()
{
	CFileHandler ifs("bagge.pth");
	int mapSeed;
	if(ifs.FileExists())
		ifs.Read((char*)&mapSeed,4);
	if(!ifs.FileExists() || mapSeed!=readmap->mapheader.mapid){
		for(int y=0;y<g.mapy/BLOCK_SIZE;++y){
			for(int x=0;x<g.mapx/BLOCK_SIZE;++x){
				float cost=MAX_COST;
				int2 tc;
				for(int y2=1;y2<BLOCK_SIZE-1;++y2){
					for(int x2=1;x2<BLOCK_SIZE-1;++x2){
						float offCenter=(abs(x2-BLOCK_SIZE/2)+abs(y2-BLOCK_SIZE/2))*0.10;
						if(pf->SquareCost(x*BLOCK_SIZE+x2,y*BLOCK_SIZE+y2)<1)
							offCenter*=0.5;
						float ccost=pf->SquareCost(x*BLOCK_SIZE+x2,y*BLOCK_SIZE+y2)+offCenter*offCenter;
						if(ccost<cost){
							tc.x=x*BLOCK_SIZE+x2;
							tc.y=y*BLOCK_SIZE+y2;
							cost=ccost;
						}
					}
				}
				centers[pathType][y][x]=tc;
			}
		}

		for(y=0;y<g.mapy/BLOCK_SIZE;++y){
			for(int x=0;x<g.mapx/BLOCK_SIZE;++x){
				squareCosts[pathType][y][x].nwcost=2000;
				squareCosts[pathType][y][x].ncost=1280;
				squareCosts[pathType][y][x].necost=2000;
				squareCosts[pathType][y][x].ecost=1280;
			}
		}

		for(y=0;y<g.mapy/BLOCK_SIZE;++y){
			char tmp[500]="Precalculating path costs ";
			char tmp2[50];
			itoa(y,tmp2,10);
			strcat(tmp,tmp2);
			PrintLoadMsg(tmp);
			for(int x=0;x<g.mapx/BLOCK_SIZE;++x){
				int mx=centers[pathType][y][x].x;
				int my=centers[pathType][y][x].y;
				if(x!=0 && y!=g.mapy/BLOCK_SIZE-1){
					pf->CreateConstraint(x*BLOCK_SIZE-BLOCK_SIZE,y*BLOCK_SIZE,x*BLOCK_SIZE+BLOCK_SIZE,y*BLOCK_SIZE+BLOCK_SIZE*2);
					squareCosts[pathType][y][x].nwcost=pf->GetPathCost(mx,my,centers[pathType][y+1][x-1].x,centers[pathType][y+1][x-1].y,2000);
					pf->ResetConstraint();
				}
				if(y!=g.mapy/BLOCK_SIZE-1){
					pf->CreateConstraint(x*BLOCK_SIZE,y*BLOCK_SIZE,x*BLOCK_SIZE+BLOCK_SIZE,y*BLOCK_SIZE+BLOCK_SIZE*2);
					squareCosts[pathType][y][x].ncost=pf->GetPathCost(mx,my,centers[pathType][y+1][x].x,centers[pathType][y+1][x].y,2000);
					pf->ResetConstraint();
				}
				if(y!=g.mapy/BLOCK_SIZE-1 && x!=g.mapx/BLOCK_SIZE-1){
					pf->CreateConstraint(x*BLOCK_SIZE,y*BLOCK_SIZE,x*BLOCK_SIZE+BLOCK_SIZE*2,y*BLOCK_SIZE+BLOCK_SIZE*2);
					squareCosts[pathType][y][x].necost=pf->GetPathCost(mx,my,centers[pathType][y+1][x+1].x,centers[pathType][y+1][x+1].y,2000);
					pf->ResetConstraint();
				}
				if(x!=g.mapx/BLOCK_SIZE-1){
					pf->CreateConstraint(x*BLOCK_SIZE,y*BLOCK_SIZE,x*BLOCK_SIZE+BLOCK_SIZE*2,y*BLOCK_SIZE+BLOCK_SIZE);
					squareCosts[pathType][y][x].ecost=pf->GetPathCost(mx,my,centers[pathType][y][x+1].x,centers[pathType][y][x+1].y,2000);
					pf->ResetConstraint();
				}
			}
		}

		boost::filesytem::path fn("bagge.pth");
		ofstream ofs(fn.native_file_string(),ios::out|ios::binary);
		ofs.write((char*)&readmap->mapheader.mapid,4);
		for(y=0;y<g.mapy/BLOCK_SIZE;++y){
			for(int x=0;x<g.mapx/BLOCK_SIZE;++x){
//				char center=centers[y][x].x-x*16+(centers[y][x].y-y*16)*16;
//				ofs.write(&center,1);
				ofs.write((char*)&centers[pathType][y][x].x,4);
				ofs.write((char*)&centers[pathType][y][x].y,4);
				if(squareCosts[pathType][y][x].nwcost<0)
					MessageBox(0,"","Negative path cost ??",0);
				ofs.write((char*)&squareCosts[pathType][y][x].nwcost,4);
				if(squareCosts[pathType][y][x].ncost<0)
					MessageBox(0,"","Negative path cost ??",0);
				ofs.write((char*)&squareCosts[pathType][y][x].ncost,4);
				if(squareCosts[pathType][y][x].necost<0)
					MessageBox(0,"","Negative path cost ??",0);
				ofs.write((char*)&squareCosts[pathType][y][x].necost,4);
				if(squareCosts[pathType][y][x].ecost<0)
					MessageBox(0,"","Negative path cost ??",0);
				ofs.write((char*)&squareCosts[pathType][y][x].ecost,4);
			}
		}
		
	} else {
		PrintLoadMsg("Loading path costs");
		for(int y=0;y<g.mapy/BLOCK_SIZE;++y){
			for(int x=0;x<g.mapx/BLOCK_SIZE;++x){
//				char center;
//				ifs.read(&center,1);
//				centers[y][x].x=center%16+x*16;
//				centers[y][x].y=center/16+y*16;
				ifs.Read((char*)&centers[pathType][y][x].x,4);
				ifs.Read((char*)&centers[pathType][y][x].y,4);
				ifs.Read((char*)&squareCosts[pathType][y][x].nwcost,4);
				ifs.Read((char*)&squareCosts[pathType][y][x].ncost,4);
				ifs.Read((char*)&squareCosts[pathType][y][x].necost,4);
				ifs.Read((char*)&squareCosts[pathType][y][x].ecost,4);
			}
		}
	}
	for(int y=0;y<g.mapy/BLOCK_SIZE;++y){
		for(int x=0;x<g.mapx/BLOCK_SIZE;++x){
			squareEstimates[y][x].cost=MAX_COST;
			squareEstimates[y][x].status=0;
		}
	}
}

void CPathEstimater::ResetEstimates()
{
  openSquares.DeleteAll();
  while(!dirtyEstimates.empty()){
    squareEstimates[0][dirtyEstimates.back()].cost=MAX_COST;
    squareEstimates[0][dirtyEstimates.back()].status=0;
    dirtyEstimates.pop_back();
  }
  int startSquareX=goalx/BLOCK_SIZE;
  int startSquareY=goaly/BLOCK_SIZE;
  CPathFinder::OpenSquare* os=new CPathFinder::OpenSquare;
  os->x=startSquareX;
  os->y=startSquareY;
  os->cost=0;
  openSquares.push(os);
  squareEstimates[startSquareY][startSquareX].cost=0;
  dirtyEstimates.push_back(startSquareY*g.mapx/BLOCK_SIZE+startSquareX);
}

void CPathEstimater::CalculateEstimates()
{
  //  cout << "calculating square estimates\n";
  while(!openSquares.empty() && !(squareEstimates[yg][xg].status&1)/**/){
    CPathFinder::OpenSquare* os=openSquares.top();
    openSquares.pop();
    int x=os->x;
    int y=os->y;
    if(!(squareEstimates[y][x].status&1)){
      TestSquare(x+1,y,os->cost,0);
      TestSquare(x-1,y,os->cost,1);
      TestSquare(x,y+1,os->cost,2);
      TestSquare(x,y-1,os->cost,3);
      
      TestSquare(x+1,y+1,os->cost,4);
      TestSquare(x-1,y+1,os->cost,5);
      TestSquare(x+1,y-1,os->cost,6);
      TestSquare(x-1,y-1,os->cost,7);
      
      /*int max=abs(centers[y][x].x-goalx);
      int min=abs(centers[y][x].y-goaly);
      if(max<min){
				int temp=max;
				max=min;
				min=temp;
      }
      squareEstimates[y][x].cost-=(max+min*0.41);
      /**/squareEstimates[y][x].status|=1;
    }
    delete os;
  }
}

void CPathEstimater::TestSquare(int x,int y,float oldcost,int dir)
{
  if((x&(0xffffffff^(g.mapx/BLOCK_SIZE-1))) || (y&(0xffffffff^(g.mapy/BLOCK_SIZE-1)))) //checka så att vi är inom området
    return;
  float scost=oldcost;
  switch(dir){
  case 0:
    scost+=squareCosts[pathType][y][x-1].ecost;
    break;
  case 1:
    scost+=squareCosts[pathType][y][x].ecost;
    break;
  case 2:
    scost+=squareCosts[pathType][y-1][x].ncost;
    break;
  case 3:
    scost+=squareCosts[pathType][y][x].ncost;
    break;
  case 4:
    scost+=squareCosts[pathType][y-1][x-1].necost;
    break;
  case 5:
    scost+=squareCosts[pathType][y-1][x+1].nwcost;
    break;
  case 6:
    scost+=squareCosts[pathType][y][x].nwcost;
    break;
  case 7:
    scost+=squareCosts[pathType][y][x].necost;
    break;
  }
  if(squareEstimates[y][x].cost<=scost)
    return;
  CPathFinder::OpenSquare* os=new CPathFinder::OpenSquare;
  os->x=x;
  os->y=y;
  os->cost=scost;
  openSquares.push(os);
  squareEstimates[y][x].cost=scost;
  squareEstimates[y][x].status=dir*8;
  dirtyEstimates.push_back(y*g.mapx/BLOCK_SIZE+x);
}

vector<int2> CPathEstimater::GetEstimatedPath(int x,int y)
{
  list<int2> path;
  xg=x/BLOCK_SIZE;
  yg=y/BLOCK_SIZE;
  CalculateEstimates();
  int gsx=goalx/BLOCK_SIZE;
  int gsy=goaly/BLOCK_SIZE;
  x=xg;
  y=yg;
  while((x!=gsx || y!=gsy) && path.size()<100){
    path.push_front(centers[pathType][y][x]);

    switch(squareEstimates[y][x].status&0xfffffffe){
    case 0:
      x-=1;
      break;
    case 8:
      x+=1;
      break;
    case 16:
      y-=1;
      break;
    case 24: 
      y+=1;
      break;
    case 32:
      x-=1;
      y-=1;
      break;
    case 40:
      x+=1;
      y-=1;
      break;
    case 48:
      x-=1;
      y++;
      break;
    case 56:
      x+=1;
      y++;
      break;
    }
  }
	int2 g;
	g.x=goalx;
	g.y=goaly;
	path.push_front(g);
	vector<int2> path2;
	list<int2>::iterator pi;
	for(pi=path.begin();pi!=path.end();++pi){
		path2.push_back(*pi);
	}
  return path2;
}

void CPathEstimater::SetPathType(int type)
{
	pathType=type;
}
