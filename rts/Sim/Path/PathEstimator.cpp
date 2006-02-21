#include "StdAfx.h"
#include "PathEstimator.h"
#include "Game/UI/InfoConsole.h"
#include "Rendering/GL/myGL.h"
#include "FileSystem/FileHandler.h"
#include <math.h>
#include <fstream>
#ifndef _WIN32
#include <stdlib.h>
#include <sys/stat.h>
#endif
#include "Sim/Map/Ground.h"
#include "Net.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Rendering/glFont.h"
#include "Game/Camera.h"
#include "TimeProfiler.h"
#include <boost/filesystem/convenience.hpp>

#include "lib/minizip/zip.h"
#include "FileSystem/ArchiveZip.h"

#include "mmgr.h"

#define PATHDEBUG false


const unsigned int PATHDIR_LEFT = 0;		//+x
const unsigned int PATHDIR_LEFT_UP = 1;		//+x+z
const unsigned int PATHDIR_UP = 2;			//+z
const unsigned int PATHDIR_RIGHT_UP = 3;	//-x+z
const unsigned int PATHDIR_RIGHT = 4;		//-x
const unsigned int PATHDIR_RIGHT_DOWN = 5;	//-x-z
const unsigned int PATHDIR_DOWN = 6;		//-z
const unsigned int PATHDIR_LEFT_DOWN = 7;	//+x-z

const unsigned int PATHOPT_OPEN = 8;
const unsigned int PATHOPT_CLOSED = 16;
const unsigned int PATHOPT_FORBIDDEN = 32;
const unsigned int PATHOPT_BLOCKED = 64;
const unsigned int PATHOPT_SEARCHRELATED = (PATHOPT_OPEN | PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED);
const unsigned int PATHOPT_OBSOLETE = 128;

const unsigned int PATHESTIMATOR_VERSION = 34;

const float PATHCOST_INFINITY = 10000000;

const int SQUARES_TO_UPDATE = 600;


extern string stupidGlobalMapname;


/*
Constructor.
Loading precalculated data.
*/
CPathEstimator::CPathEstimator(CPathFinder* pf, unsigned int BSIZE, unsigned int mmOpt, string name) :
pathFinder(pf),
BLOCK_SIZE(BSIZE),
BLOCK_PIXEL_SIZE(BSIZE * SQUARE_SIZE),
BLOCKS_TO_UPDATE(SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1),
moveMathOptions(mmOpt)
{
	//Gives the changes in (x,z) when moved one step in given direction.
	//(Need to be placed befor pre-calculations)
	directionVector[PATHDIR_LEFT].x = 1;
	directionVector[PATHDIR_LEFT].y = 0;
	directionVector[PATHDIR_LEFT_UP].x = 1;
	directionVector[PATHDIR_LEFT_UP].y = 1;
	directionVector[PATHDIR_UP].x = 0;
	directionVector[PATHDIR_UP].y = 1;
	directionVector[PATHDIR_RIGHT_UP].x = -1;
	directionVector[PATHDIR_RIGHT_UP].y = 1;
	directionVector[PATHDIR_RIGHT].x = -1;
	directionVector[PATHDIR_RIGHT].y = 0;
	directionVector[PATHDIR_RIGHT_DOWN].x = -1;
	directionVector[PATHDIR_RIGHT_DOWN].y = -1;
	directionVector[PATHDIR_DOWN].x = 0;
	directionVector[PATHDIR_DOWN].y = -1;
	directionVector[PATHDIR_LEFT_DOWN].x = 1;
	directionVector[PATHDIR_LEFT_DOWN].y = -1;

	goalSqrOffset.x=BLOCK_SIZE/2;
	goalSqrOffset.y=BLOCK_SIZE/2;

	//Creates the block-map and the vertices-map.
	nbrOfBlocksX = gs->mapx / BLOCK_SIZE;
	nbrOfBlocksZ = gs->mapy / BLOCK_SIZE;
	nbrOfBlocks = nbrOfBlocksX * nbrOfBlocksZ;

	blockState = new BlockInfo[nbrOfBlocks];
	nbrOfVertices = moveinfo->moveData.size() * nbrOfBlocks * PATH_DIRECTION_VERTICES;
	vertex = new float[nbrOfVertices];
	openBlockBufferPointer=openBlockBuffer;

	int i;
	for(i = 0; i < nbrOfVertices; i++)
		vertex[i] = PATHCOST_INFINITY;

	//Initialize blocks.
	int x, z;
	for(z = 0; z < nbrOfBlocksZ; z++){
		for(x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			blockState[blocknr].cost = PATHCOST_INFINITY;
			blockState[blocknr].options = 0;
			blockState[blocknr].parentBlock.x = -1;
			blockState[blocknr].parentBlock.y = -1;
			blockState[blocknr].sqrCenter = new int2[moveinfo->moveData.size()];
		}
	}

	//Pre-read/calculate data.
	PrintLoadMsg("Reading estimate path costs");
	if(!ReadFile(name)) {
		//Generate text-message.
		char calcMsg[1000], buffer[10];
		strcpy(calcMsg, "Analyzing map accessability \"");
		SNPRINTF(buffer,10,"%d",BLOCK_SIZE);
		strcat(calcMsg, buffer);
		strcat(calcMsg, "\"");
		PrintLoadMsg(calcMsg);
		//Calculating block-center-offsets.
		for(z = 0; z < nbrOfBlocksZ; z++) {
			for(x = 0; x < nbrOfBlocksX; x++) {
				vector<MoveData*>::iterator mi;
				for(mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
					FindOffset(**mi, x, z);
				}
			}
		}

		//Calculating vectors.
		vector<MoveData*>::iterator mi;
		for(mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
			//Generate text-message.
			char calcMsg[10000];
			sprintf(calcMsg,"Calculating estimate path costs \"%i\" %i/%i",BLOCK_SIZE,(*mi)->pathType,moveinfo->moveData.size());
			PrintLoadMsg(calcMsg);
			if(net) net->Update();	//prevent timeout 
			//Calculate
			for(z = 0; z < nbrOfBlocksZ; z++) {
				for(x = 0; x < nbrOfBlocksX; x++) {
					CalculateVertices(**mi, x, z);
				}
			}
		}
		WriteFile(name);
	}

	//As all vertexes are bidirectional and having equal values
	//in both directions, only one value are needed to be stored.
	//This vector helps getting the right vertex.
	//(Need to be placed after pre-calculations)
	directionVertex[PATHDIR_LEFT] = PATHDIR_LEFT;
	directionVertex[PATHDIR_LEFT_UP] = PATHDIR_LEFT_UP;
	directionVertex[PATHDIR_UP] = PATHDIR_UP;
	directionVertex[PATHDIR_RIGHT_UP] = PATHDIR_RIGHT_UP;
	directionVertex[PATHDIR_RIGHT] = int(PATHDIR_LEFT) - PATH_DIRECTION_VERTICES;
	directionVertex[PATHDIR_RIGHT_DOWN] = int(PATHDIR_LEFT_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES) - PATH_DIRECTION_VERTICES;
	directionVertex[PATHDIR_DOWN] = int(PATHDIR_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES);
	directionVertex[PATHDIR_LEFT_DOWN] = int(PATHDIR_RIGHT_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES) + PATH_DIRECTION_VERTICES;

	pathCache=new CPathCache(nbrOfBlocksX,nbrOfBlocksZ);
}


/*
Destructor
Free all used memory.
*/
CPathEstimator::~CPathEstimator() {
	int i;
	for(i = 0; i < nbrOfBlocks; i++) {
		delete[] blockState[i].sqrCenter;
	}
	delete[] blockState;
	delete[] vertex;
	delete pathCache;
}

/*
Finds a square accessable by the given movedata within the given block.
*/
void CPathEstimator::FindOffset(const MoveData& moveData, int blockX, int blockZ) {
	//Block lower corner position.
	int lowerX = blockX * BLOCK_SIZE;
	int lowerZ = blockZ * BLOCK_SIZE;

	//Search for an accessable position.
	float best=100000000;
	int bestX=BLOCK_SIZE/2;
	int bestZ=BLOCK_SIZE/2;

	int x, z;
	for(z = 1; z < BLOCK_SIZE; z += 2){
		for(x = 1; x < BLOCK_SIZE; x += 2) {
			int dx=x-BLOCK_SIZE/2;
			int dz=z-BLOCK_SIZE/2;
			float cost=dx*dx+dz*dz+(BLOCK_SIZE*BLOCK_SIZE/8)/(0.001+moveData.moveMath->SpeedMod(moveData, lowerX+x, lowerZ+z));
			if(moveData.moveMath->IsBlocked2(moveData, lowerX+x, lowerZ+z) & (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN))
				cost+=1000000;
			if(cost<best){
				best=cost;
				bestX=x;
				bestZ=z;
			}
		}
	}

	//Store the offset found.
	blockState[blockZ * nbrOfBlocksX + blockX].sqrCenter[moveData.pathType].x = blockX * BLOCK_SIZE + bestX;
	blockState[blockZ * nbrOfBlocksX + blockX].sqrCenter[moveData.pathType].y = blockZ * BLOCK_SIZE + bestZ;
}


/*
Calculate all vertices connected from given block.
(Which is 4 out of 8 vertices connected to the block.)
*/
void CPathEstimator::CalculateVertices(const MoveData& moveData, int blockX, int blockZ) {
	unsigned int dir;
	for(dir = 0; dir < PATH_DIRECTION_VERTICES; dir++)
		CalculateVertex(moveData, blockX, blockZ, dir);
}


/*
Calculate requested vertex.
*/
void CPathEstimator::CalculateVertex(const MoveData& moveData, int parentBlockX, int parentBlockZ, unsigned int direction) {
	//Initial calculations.
	int parentBlocknr = parentBlockZ * nbrOfBlocksX + parentBlockX;
	int childBlockX = parentBlockX + directionVector[direction].x;
	int childBlockZ = parentBlockZ + directionVector[direction].y;
	int vertexNbr = moveData.pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + parentBlocknr * PATH_DIRECTION_VERTICES + direction;

	//Outside map?
	if(childBlockX < 0 || childBlockZ < 0
	|| childBlockX >= nbrOfBlocksX || childBlockZ >= nbrOfBlocksZ) {
		vertex[vertexNbr] = PATHCOST_INFINITY;
		return;
	}

	//Starting position.
	int parentXSquare = blockState[parentBlocknr].sqrCenter[moveData.pathType].x;
	int parentZSquare = blockState[parentBlocknr].sqrCenter[moveData.pathType].y;
	float3 startPos = SquareToFloat3(parentXSquare, parentZSquare);

	//Goal position.
	int childBlocknr = childBlockZ * nbrOfBlocksX + childBlockX;
	int childXSquare = blockState[childBlocknr].sqrCenter[moveData.pathType].x;
	int childZSquare = blockState[childBlocknr].sqrCenter[moveData.pathType].y;
	float3 goalPos = SquareToFloat3(childXSquare, childZSquare);

	//PathFinder definiton.
	CRangedGoalWithCircularConstraint pfDef(startPos, goalPos, 0, 1.1f,2);

	//Path
	Path path;

	//Performs the search.
	SearchResult result = pathFinder->GetPath(moveData, startPos, pfDef, path, false, true,10000,false);

	//Store the result.
	if(result == Ok) {
		vertex[vertexNbr] = path.pathCost;
	} else {
		vertex[vertexNbr] = PATHCOST_INFINITY;
	}
}


/*
Mark affected blocks as obsolete.
*/
void CPathEstimator::MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned z2) {
	//Finding the upper and lower corner of the rectangular area.
	int lowerX, upperX, lowerZ, upperZ;
	if(x1 < x2) {
		lowerX = x1 / BLOCK_SIZE-1;
		upperX = x2 / BLOCK_SIZE;
	} else {
		lowerX = x2 / BLOCK_SIZE-1;
		upperX = x1 / BLOCK_SIZE;
	}
	if(z1 < z2) {
		lowerZ = z1 / BLOCK_SIZE-1;
		upperZ = z2 / BLOCK_SIZE;
	} else {
		lowerZ = z2 / BLOCK_SIZE-1;
		upperZ = z1 / BLOCK_SIZE;
	}

	//Error-check.
	upperX = min(upperX, nbrOfBlocksX - 1);
	upperZ = min(upperZ, nbrOfBlocksZ - 1);
	if(lowerX<0) lowerX=0;
	if(lowerZ<0) lowerZ=0;

	//Marking the blocks inside the rectangle.
	//Enqueing them from upper to lower becourse of
	//the placement of the bi-directional vertices.
	for(int z = upperZ; z >= lowerZ; z--){
		for(int x = upperX; x >= lowerX; x--) {
			if(!(blockState[z * nbrOfBlocksX + x].options & PATHOPT_OBSOLETE)){
				vector<MoveData*>::iterator mi;
				for(mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
					SingleBlock sb;
					sb.block.x = x;
					sb.block.y = z;
					sb.moveData = *mi;
					needUpdate.push_back(sb);
					blockState[z * nbrOfBlocksX + x].options |= PATHOPT_OBSOLETE;
				}
			}
		}
	}
}


/*
Updating some obsolete blocks.
Using FIFO-principle.
*/
void CPathEstimator::Update() {
	pathCache->Update();
	int counter = 0;
	while(!needUpdate.empty() && counter < BLOCKS_TO_UPDATE) {
		//Next block in line.
		SingleBlock sb = needUpdate.front();
		needUpdate.pop_front();
		int blocknr = sb.block.y * nbrOfBlocksX + sb.block.x;

		//Check if already updated.
		if(!(blockState[blocknr].options & PATHOPT_OBSOLETE))
			continue;

		//Update the block.
		FindOffset(*sb.moveData, sb.block.x, sb.block.y);
		CalculateVertices(*sb.moveData, sb.block.x, sb.block.y);

		//Mark as updated.
		if(sb.moveData==moveinfo->moveData.back()){
			blockState[blocknr].options &= ~PATHOPT_OBSOLETE;
		}

		//One block updated.
		counter++;
	}
}


/*
Storing data and doing some top-administration.
*/
IPath::SearchResult CPathEstimator::GetPath(const MoveData& moveData, float3 start, const CPathFinderDef& peDef, Path& path, unsigned int maxSearchedBlocks) {
//	START_TIME_PROFILE;
	start.CheckInBounds();
	//Clear path.
	path.path.clear();
	path.pathCost = PATHCOST_INFINITY;

	//Initial calculations.
	maxBlocksToBeSearched = std::min(maxSearchedBlocks, (unsigned int) MAX_SEARCHED_BLOCKS);
	startBlock.x = (int)(start.x / BLOCK_PIXEL_SIZE);
	startBlock.y = (int)(start.z / BLOCK_PIXEL_SIZE);
	startBlocknr = startBlock.y * nbrOfBlocksX + startBlock.x;
	int2 goalBlock;
	goalBlock.x=peDef.goalSquareX/BLOCK_SIZE;
	goalBlock.y=peDef.goalSquareZ/BLOCK_SIZE;

	CPathCache::CacheItem* ci=pathCache->GetCachedPath(startBlock,goalBlock,peDef.sqGoalRadius,moveData.pathType);
	if(ci){
//		info->AddLine("Using cached path %i",BLOCK_SIZE);
		path=ci->path;
/*		if(BLOCK_SIZE==8){
			END_TIME_PROFILE("Estimater 8");
		}else{
			END_TIME_PROFILE("Estimater 32");
		}*/
		return ci->result;
	}
//	info->AddLine("----Creating new path %i",BLOCK_SIZE);

	//Search
	SearchResult result = InitSearch(moveData, peDef);

	//If successful, generate path.
	if(result == Ok || result == GoalOutOfRange) {
		FinishSearch(moveData, path);
		pathCache->AddPath(&path,result,startBlock,goalBlock,peDef.sqGoalRadius,moveData.pathType);		//only add succesfull paths to the cache
		if(PATHDEBUG) {
			*info << "PE: Search completed.\n";
			*info << "Tested blocks: " << testedBlocks << "\n";
			*info << "Open blocks: " << (float)(openBlockBufferPointer - openBlockBuffer) << "\n";
			*info << "Path length: " << (int)(path.path.size()) << "\n";
			*info << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if(PATHDEBUG) {
			*info << "PE: Search failed!\n";
			*info << "Tested blocks: " << testedBlocks << "\n";
			*info << "Open blocks: " << (float)(openBlockBufferPointer - openBlockBuffer) << "\n";
		}
	}
/*	if(BLOCK_SIZE==8){
		END_TIME_PROFILE("Estimater 8");
	}else{
		END_TIME_PROFILE("Estimater 32");
	}*/
	return result;
}


/*
Making some initial calculations and preparations.
*/
IPath::SearchResult CPathEstimator::InitSearch(const MoveData& moveData, const CPathFinderDef& peDef) {
	//Starting square is inside goal area?
	int xSquare = blockState[startBlocknr].sqrCenter[moveData.pathType].x;
	int zSquare = blockState[startBlocknr].sqrCenter[moveData.pathType].y;
	if(peDef.IsGoal(xSquare, zSquare))
		return CantGetCloser;

	//Cleaning the system from last search.
	ResetSearch();

	//Marks and store the start-block.
	blockState[startBlocknr].options |= PATHOPT_OPEN;
	blockState[startBlocknr].cost = 0;
	dirtyBlocks.push_back(startBlocknr);

	//Adding the starting block to the open-blocks-queue.
	OpenBlock* ob = openBlockBufferPointer = openBlockBuffer;
	ob->cost = 0;
	ob->currentCost = 0;
	ob->block = startBlock;
	ob->blocknr = startBlocknr;
	openBlocks.push(ob);

	//Mark starting point as best found position.
	goalBlock = startBlock;
	goalHeuristic = peDef.Heuristic(xSquare, zSquare);

	//Gets goal square offset.
	goalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);

	//Performs the search.
	SearchResult result = DoSearch(moveData, peDef);

	//If no improvements are found, then return CantGetCloser instead.
	if(goalBlock.x == startBlock.x && goalBlock.y == startBlock.y)
		return CantGetCloser;
	else
		return result;
}


/*
Performs the actual search.
*/
IPath::SearchResult CPathEstimator::DoSearch(const MoveData& moveData, const CPathFinderDef& peDef) {
	bool foundGoal = false;
	while(!openBlocks.empty() && (openBlockBufferPointer - openBlockBuffer) < (maxBlocksToBeSearched - 8)) {
		//Get the open block with lowest cost.
		OpenBlock* ob = openBlocks.top();
		openBlocks.pop();

		//Check if the block has been marked as unaccessable during it's time in queue.
		if(blockState[ob->blocknr].options & (PATHOPT_BLOCKED | PATHOPT_CLOSED | PATHOPT_FORBIDDEN))
			continue;

		//Check if the goal is reached.
		int xBSquare = blockState[ob->blocknr].sqrCenter[moveData.pathType].x;
		int zBSquare = blockState[ob->blocknr].sqrCenter[moveData.pathType].y;
		int xGSquare = ob->block.x * BLOCK_SIZE + goalSqrOffset.x;
		int zGSquare = ob->block.y * BLOCK_SIZE + goalSqrOffset.y;
		if(peDef.IsGoal(xBSquare, zBSquare) || peDef.IsGoal(xGSquare, zGSquare)) {
			goalBlock = ob->block;
			goalHeuristic = 0;
			foundGoal = true;
			break;
		}

		//Test the 8 surrounding blocks.
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT);
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT_UP);
		TestBlock(moveData, peDef, *ob, PATHDIR_UP);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT_UP);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT_DOWN);
		TestBlock(moveData, peDef, *ob, PATHDIR_DOWN);
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT_DOWN);

		//Mark this block as closed.
		blockState[ob->blocknr].options |= PATHOPT_CLOSED;
	}

	//Returning search-result.
	if(foundGoal)
		return Ok;

	//Could not reach the goal.
	if(openBlockBufferPointer - openBlockBuffer >= (maxBlocksToBeSearched - 8))
		return GoalOutOfRange;

	//Search could not reach the goal, due to the unit being locked in.
	if(openBlocks.empty())
		return GoalOutOfRange;

	//Below shall never be runned.
	*info << "ERROR: CPathEstimator::DoSearch() - Unhandled end of search!\n";
	return Error;
}


/*
Test the accessability of a block and it's value
and possibly add it to the open-blocks-queue.
*/
void CPathEstimator::TestBlock(const MoveData& moveData, const CPathFinderDef &peDef, OpenBlock& parentOpenBlock, unsigned int direction) {
	testedBlocks++;
	
	//Initial calculations of the new block.
	int2 block;
	block.x = parentOpenBlock.block.x + directionVector[direction].x;
	block.y = parentOpenBlock.block.y + directionVector[direction].y;
	int vertexNbr = moveData.pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + parentOpenBlock.blocknr * PATH_DIRECTION_VERTICES + directionVertex[direction];

	//Outside map?
	if(/*block.x < 0 || block.x >= nbrOfBlocksX		//the blocks should never be able to become wrong due to the infinite vertices at the edges
	|| block.y < 0 || block.y >= nbrOfBlocksZ
	||*/ vertexNbr < 0 || vertexNbr >= nbrOfVertices)
		return;

	int blocknr = block.y * nbrOfBlocksX + block.x;
	float blockCost = vertex[vertexNbr];
	if(blockCost >= PATHCOST_INFINITY)
		return;

	//Check if the block is unavailable.
	if(blockState[blocknr].options & (PATHOPT_FORBIDDEN | PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return;

	int xSquare = blockState[blocknr].sqrCenter[moveData.pathType].x;
	int zSquare = blockState[blocknr].sqrCenter[moveData.pathType].y;

	//Check if the block is blocked or out of constraints.
	if(!peDef.WithinConstraints(xSquare, zSquare)) {
		blockState[blocknr].options |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(blocknr);
		return;
	}

	//Evaluate this node.
	float heuristicCost = peDef.Heuristic(xSquare, zSquare);
	float currentCost = parentOpenBlock.currentCost + blockCost;
	float cost = currentCost + heuristicCost;

	//Check if the block is already in queue and better, then keep it.
	if(blockState[blocknr].options & PATHOPT_OPEN) {
		if(blockState[blocknr].cost <= cost)
			return;
		blockState[blocknr].options &= 255-7;
	}

	//Looking for improvements.
	if(heuristicCost < goalHeuristic) {
		goalBlock = block;
		goalHeuristic = heuristicCost;
	}

	//Store this block as open.
	OpenBlock* ob = ++openBlockBufferPointer;
	ob->block = block;
	ob->blocknr = blocknr;
	ob->cost = cost;
	ob->currentCost = currentCost;
	openBlocks.push(ob);
	
	//Mark the block as open, and it's parent.
	blockState[blocknr].cost = cost;
	blockState[blocknr].options |= (direction | PATHOPT_OPEN);
	blockState[blocknr].parentBlock = parentOpenBlock.block;
	dirtyBlocks.push_back(blocknr);
}


/*
Recreate the path taken to the goal.
*/
void CPathEstimator::FinishSearch(const MoveData& moveData, Path& path) {
	int2 block = goalBlock;
	while(block.x != startBlock.x || block.y != startBlock.y) {
		int blocknr = block.y * nbrOfBlocksX + block.x;
/*		int xGSquare = block.x * BLOCK_SIZE + goalSqrOffset.x;
		int zGSquare = block.y * BLOCK_SIZE + goalSqrOffset.y;
		//In first case trying to use a by goal defined offset...
		if(!moveData.moveMath->IsBlocked(moveData, moveMathOptions, xGSquare, zGSquare)) {
			float3 pos = SquareToFloat3(xGSquare, zGSquare);
			pos.y = moveData.moveMath->yLevel(xGSquare, zGSquare);
			path.path.push_back(pos);
		}
		//...if not possible, then use offset defined by the block.
		else */{
			int xBSquare = blockState[blocknr].sqrCenter[moveData.pathType].x;
			int zBSquare = blockState[blocknr].sqrCenter[moveData.pathType].y;
			float3 pos = SquareToFloat3(xBSquare, zBSquare);
			path.path.push_back(pos);
		}

		//Next step backwards.
		block = blockState[blocknr].parentBlock;
	}

	//Additional information.
	path.pathCost = blockState[goalBlock.y * nbrOfBlocksX + goalBlock.x].cost - goalHeuristic;
	path.pathGoal = path.path.front();
}


/*
Cleaning lists from last search.
*/
void CPathEstimator::ResetSearch() {
	while(!openBlocks.empty())
		openBlocks.pop();
	while(!dirtyBlocks.empty()) {
		blockState[dirtyBlocks.back()].cost = PATHCOST_INFINITY;
		blockState[dirtyBlocks.back()].parentBlock.x = -1;
		blockState[dirtyBlocks.back()].parentBlock.y = -1;
		blockState[dirtyBlocks.back()].options &= 128;//~PATHOPT_SEARCHRELATED;
		dirtyBlocks.pop_back();
	}
	testedBlocks = 0;
}


/*
Trying to read offset and vertices data from file.
Return false if failed.
TODO: Read-error-check.
*/
bool CPathEstimator::ReadFile(string name) 
{

	unsigned int hash=Hash();
	char hashString[50];
	sprintf(hashString,"%u",hash);

	string filename = string("maps/paths/") + stupidGlobalMapname.substr(0, stupidGlobalMapname.find_last_of('.') + 1) + hashString + "." + name + ".zip";
	CArchiveZip file(filename);
	if (!file.IsOpen()) 
		return false;

	int fh = file.OpenFile("pathinfo");

	if (fh) {
		unsigned int filehash = 0;
 		//Check hash.
//		info->AddLine("%i",hash);
		file.ReadFile(fh, &filehash, 4);
		if(filehash != hash)
			return false;

		//Read block-center-offset data.
		int blocknr;
		for(blocknr = 0; blocknr < nbrOfBlocks; blocknr++) {
			file.ReadFile(fh, blockState[blocknr].sqrCenter, moveinfo->moveData.size() * sizeof(int2));
		}

		//Read vertices data.
		file.ReadFile(fh, vertex, nbrOfVertices * sizeof(float));
		
		//File read successful.
		return true;
	} else {
		return false;
	}
}


/*
Trying to write offset and vertices data to file.
*/
void CPathEstimator::WriteFile(string name) {
	// We need this directory to exist
	boost::filesystem::path f("./maps/paths");
	if (!boost::filesystem::exists(f))
		boost::filesystem::create_directories(f);

	unsigned int hash = Hash();
	char hashString[50];
	sprintf(hashString,"%u",hash);

	string filename = string("maps/paths/") + stupidGlobalMapname.substr(0, stupidGlobalMapname.find('.') + 1) + hashString + "." + name + ".zip";
	zipFile file = zipOpen(filename.c_str(), APPEND_STATUS_CREATE);

	if (file) {
		zipOpenNewFileInZip(file, "pathinfo", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);
		
		//Write hash.
		unsigned int hash = Hash();
		zipWriteInFileInZip(file, (void*)&hash, 4);

		//Write block-center-offsets.
		int blocknr;
		for(blocknr = 0; blocknr < nbrOfBlocks; blocknr++) {
			zipWriteInFileInZip(file, (void*)blockState[blocknr].sqrCenter, moveinfo->moveData.size() * sizeof(int2));
			//file.write((char*)blockState[blocknr].sqrCenter, moveinfo->moveData.size() * sizeof(int2));
		}

		//Write vertices.
		zipWriteInFileInZip(file, (void*)vertex, nbrOfVertices * sizeof(float));
		//file.write((char*)vertex, nbrOfVertices * sizeof(float));

		zipCloseFileInZip(file);
		zipClose(file, NULL);
	}
}


/*
Gives a hash-code identifying the dataset of this estimator.
*/
unsigned int CPathEstimator::Hash() {
	return readmap->mapChecksum + moveinfo->moveInfoChecksum + BLOCK_SIZE + moveMathOptions + PATHESTIMATOR_VERSION;
}

void CPathEstimator::Draw(void)
{
	MoveData* md=moveinfo->GetMoveDataFromName("TANKSH2");
	if(!selectedUnits.selectedUnits.empty() && (*selectedUnits.selectedUnits.begin())->unitDef->movedata)
		md=(*selectedUnits.selectedUnits.begin())->unitDef->movedata;

	glDisable(GL_TEXTURE_2D);
	glColor3f(1,1,0);
	float blue=BLOCK_SIZE==32?1:0;
/*	glBegin(GL_LINES);
	for(int z = 0; z < nbrOfBlocksZ; z++) {
		for(int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x=(blockState[blocknr].sqrCenter[md->pathType].x)*8;
			p1.z=(blockState[blocknr].sqrCenter[md->pathType].y)*8;
			p1.y=ground->GetHeight(p1.x,p1.z)+10;

			glColor3f(1,1,blue);
			glVertexf3(p1);
			glVertexf3(p1-UpVector*10);
			for(int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++){
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if(obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x=(blockState[obblocknr].sqrCenter[md->pathType].x)*8;
					p2.z=(blockState[obblocknr].sqrCenter[md->pathType].y)*8;
					p2.y=ground->GetHeight(p2.x,p2.z)+10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost=vertex[vertexNbr];

					glColor3f(1/(sqrt(cost/BLOCK_SIZE)),1/(cost/BLOCK_SIZE),blue);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

	}
	glEnd();/**/
/*	glEnable(GL_TEXTURE_2D);
	for(int z = 0; z < nbrOfBlocksZ; z++) {
		for(int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x=(blockState[blocknr].sqrCenter[md->pathType].x)*SQUARE_SIZE;
			p1.z=(blockState[blocknr].sqrCenter[md->pathType].y)*SQUARE_SIZE;
			p1.y=ground->GetHeight(p1.x,p1.z)+10;

			glColor3f(1,1,blue);
			for(int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++){
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if(obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x=(blockState[obblocknr].sqrCenter[md->pathType].x)*SQUARE_SIZE;
					p2.z=(blockState[obblocknr].sqrCenter[md->pathType].y)*SQUARE_SIZE;
					p2.y=ground->GetHeight(p2.x,p2.z)+10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost=vertex[vertexNbr];

					glColor3f(1,1/(cost/BLOCK_SIZE),blue);

					p2=(p1+p2)/2;
					if(camera->pos.distance(p2)<500){
						glPushMatrix();
						glTranslatef3(p2);
						glScalef(5,5,5);
						font->glWorldPrint("%.0f",cost);
						glPopMatrix();
					}
				}
			}
		}
	}
*/
	if(BLOCK_SIZE==8)
		glColor3f(0.2,0.7,0.2);
	else
		glColor3f(0.2,0.2,0.7);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	for(OpenBlock*  ob=openBlockBuffer;ob!=openBlockBufferPointer;++ob){
		int blocknr = ob->blocknr;
		float3 p1;
		p1.x=(blockState[blocknr].sqrCenter[md->pathType].x)*SQUARE_SIZE;
		p1.z=(blockState[blocknr].sqrCenter[md->pathType].y)*SQUARE_SIZE;
		p1.y=ground->GetHeight(p1.x,p1.z)+15;

		float3 p2;
		int obx=blockState[ob->blocknr].parentBlock.x;
		int obz=blockState[ob->blocknr].parentBlock.y;
		int obblocknr =  obz * nbrOfBlocksX + obx;

		if(obblocknr>=0){
			p2.x=(blockState[obblocknr].sqrCenter[md->pathType].x)*SQUARE_SIZE;
			p2.z=(blockState[obblocknr].sqrCenter[md->pathType].y)*SQUARE_SIZE;
			p2.y=ground->GetHeight(p2.x,p2.z)+15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}
	glEnd();
/*	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1,0,blue,0.7);
	glAlphaFunc(GL_GREATER,0.05);
	int a=0;
	for(OpenBlock*  ob=openBlockBuffer;ob!=openBlockBufferPointer;++ob){
		int blocknr = ob->blocknr;
		float3 p1;
		p1.x=(ob->block.x * BLOCK_SIZE + blockState[blocknr].sqrCenter[md->pathType].x)*SQUARE_SIZE;
		p1.z=(ob->block.y * BLOCK_SIZE + blockState[blocknr].sqrCenter[md->pathType].y)*SQUARE_SIZE;
		p1.y=ground->GetHeight(p1.x,p1.z)+15;

		if(camera->pos.distance(p1)<500){
			glPushMatrix();
			glTranslatef3(p1);
			glScalef(5,5,5);
			font->glWorldPrint("%.0f %.0f",ob->cost,ob->currentCost);
			glPopMatrix();
		}
		++a;
	}
	glDisable(GL_BLEND);*/
}

float3 CPathEstimator::FindBestBlockCenter(const MoveData* moveData, float3 pos)
{
	int pathType=moveData->pathType;
	CRangedGoalWithCircularConstraint rangedGoal(pos,pos, 0,0,SQUARE_SIZE*BLOCK_SIZE*SQUARE_SIZE*BLOCK_SIZE*4);
	IPath::Path path;

	std::vector<float3> startPos;

	int xm=(int)(pos.x/(SQUARE_SIZE*BLOCK_SIZE));
	int ym=(int)(pos.z/(SQUARE_SIZE*BLOCK_SIZE));

	for(int y=max(0,ym-1);y<=min(nbrOfBlocksZ-1,ym+1);++y){
		for(int x=max(0,xm-1);x<=min(nbrOfBlocksX-1,xm+1);++x){
			startPos.push_back(float3(blockState[y*nbrOfBlocksX+x].sqrCenter[pathType].x*SQUARE_SIZE,0,blockState[y*nbrOfBlocksX+x].sqrCenter[pathType].y*SQUARE_SIZE));
		}
	}
	IPath::SearchResult result = pathFinder->GetPath(*moveData, startPos, rangedGoal, path);
	if(result == IPath::Ok && !path.path.empty()) {
		return path.path.back();
	}
	return ZeroVector;
}
