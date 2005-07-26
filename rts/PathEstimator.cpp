#include "PathEstimator.h"
#include "InfoConsole.h"
#include "myGL.h"
#include "FileHandler.h"
#include <math.h>
#include <fstream>
#ifdef unix
#include <stdlib.h>
#endif

#define PATHDEBUG false


const unsigned int PATHDIR_LEFT = 0;		//+x
const unsigned int PATHDIR_LEFT_UP = 1;		//+x+z
const unsigned int PATHDIR_UP = 2;			//+z
const unsigned int PATHDIR_RIGHT_UP = 3;	//-x+z
const unsigned int PATHDIR_RIGHT = 4;		//-x
const unsigned int PATHDIR_RIGHT_DOWN = 5;	//-x-z
const unsigned int PATHDIR_DOWN = 6;		//-z
const unsigned int PATHDIR_LEFT_DOWN = 7;	//+x-z
const unsigned int PATHDIR_ALL = 7;

const unsigned int PATHOPT_OPEN = 8;
const unsigned int PATHOPT_CLOSED = 16;
const unsigned int PATHOPT_FORBIDDEN = 32;
const unsigned int PATHOPT_BLOCKED = 64;
const unsigned int PATHOPT_SEARCHRELATED = (PATHOPT_OPEN | PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED);
const unsigned int PATHOPT_OBSOLETE = 128;

const unsigned int PATHESTIMATOR_VERSION = 14;

const float PATHCOST_INFINITY = 1e9;

const int SQUARES_TO_UPDATE = 100;


extern string stupidGlobalMapname;	//I hate this one...


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
	if(gs->mapx % BLOCK_SIZE != 0)
		nbrOfBlocksX += 1;
	nbrOfBlocksZ = gs->mapy / BLOCK_SIZE;
	if(gs->mapy % BLOCK_SIZE != 0)
		nbrOfBlocksZ += 1;
	nbrOfBlocks = nbrOfBlocksX * nbrOfBlocksZ;
	blockState = new BlockInfo[nbrOfBlocks];
	nbrOfVertices = moveinfo->moveData.size() * nbrOfBlocks * PATH_DIRECTION_VERTICES;
	vertex = new float[nbrOfVertices];
	int i;
	for(i = 0; i < nbrOfVertices; i++)	//Debug
		vertex[i] = PATHCOST_INFINITY;

	//Initialize blocks.
	int x, z;
	for(z = 0; z < nbrOfBlocksZ; z++)
		for(x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			blockState[blocknr].cost = PATHCOST_INFINITY;
			blockState[blocknr].options = 0;
			blockState[blocknr].parentBlock.x = -1;
			blockState[blocknr].parentBlock.y = -1;
			blockState[blocknr].sqrOffset = new int2[moveinfo->moveData.size()];
		}

	//Pre-read/calculate data.
	PrintLoadMsg("Reading estimate path costs");
	if(!ReadFile(name)) {
		//Generate text-message.
		char calcMsg[1000], buffer[10];
		strcpy(calcMsg, "Analyzing map accessability \"");
		snprintf(buffer, 10, "%d", BLOCK_SIZE);
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
			char calcMsg[10000], buffer[100];
			strcpy(calcMsg, "Calculating estimate path costs \"");
			snprintf(buffer, 100, "%d", BLOCK_SIZE);
			strcat(calcMsg, buffer);
			strcat(calcMsg, "\" ");
			snprintf(buffer, 10, "%d", (*mi)->pathType);
			strcat(calcMsg, buffer);
			strcat(calcMsg, "/");
			snprintf(buffer, 10, "%d", moveinfo->moveData.size());
			strcat(calcMsg, buffer);
			PrintLoadMsg(calcMsg);
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
}


/*
Destructor
Free all used memory.
*/
CPathEstimator::~CPathEstimator() {
	int i;
	for(i = 0; i < nbrOfBlocks; i++) {
		delete[] blockState[i].sqrOffset;
	}
	delete[] blockState;
	delete[] vertex;
}

const unsigned int CPathEstimator::MAX_SEARCHED_BLOCKS;

/*
Finds a square accessable by the given movedata within the given block.
*/
void CPathEstimator::FindOffset(const MoveData& moveData, int blockX, int blockZ) {
	//Block lower corner position.
	int lowerX = blockX * BLOCK_SIZE;
	int lowerZ = blockZ * BLOCK_SIZE;

	//Search for an accessable position.
	float best=100000000;
	int bestX=0;
	int bestZ=0;

	int x, z;
	for(z = 0; z < BLOCK_SIZE; z += 2){
		for(x = 0; x < BLOCK_SIZE; x += 2) {
			if(!moveData.moveMath->IsBlocked(moveData, moveMathOptions, lowerX + x, lowerZ + z)){
				int dx=x-BLOCK_SIZE/2;
				int dz=z-BLOCK_SIZE/2;
				float cost=dx*dx+dz*dz+1/(0.001+moveData.moveMath->SpeedMod(moveData, lowerX+x, lowerZ+z));
				if(cost<best){
					best=cost;
					bestX=x;
					bestZ=z;
				}
			}
		}
	}

	//Store the offset found.
	blockState[blockZ * nbrOfBlocksX + blockX].sqrOffset[moveData.pathType].x = bestX;
	blockState[blockZ * nbrOfBlocksX + blockX].sqrOffset[moveData.pathType].y = bestZ;
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
	int parentXSquare = parentBlockX * BLOCK_SIZE + blockState[parentBlocknr].sqrOffset[moveData.pathType].x;
	int parentZSquare = parentBlockZ * BLOCK_SIZE + blockState[parentBlocknr].sqrOffset[moveData.pathType].y;
	float3 startPos = SquareToFloat3(parentXSquare, parentZSquare);

	//Goal position.
	int childBlocknr = childBlockZ * nbrOfBlocksX + childBlockX;
	int childXSquare = childBlockX * BLOCK_SIZE + blockState[childBlocknr].sqrOffset[moveData.pathType].x;
	int childZSquare = childBlockZ * BLOCK_SIZE + blockState[childBlocknr].sqrOffset[moveData.pathType].y;
	float3 goalPos = SquareToFloat3(childXSquare, childZSquare);

	//PathFinder definiton.
	CRangedGoalWithCircularConstraintPFD pfDef(startPos, goalPos, 0);

	//Path
	Path path;

	//Performs the search.
	SearchResult result = pathFinder->GetPath(moveData, startPos, pfDef, path, moveMathOptions, true);

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
		lowerX = x1 / BLOCK_SIZE;
		upperX = x2 / BLOCK_SIZE + 1;
	} else {
		lowerX = x2 / BLOCK_SIZE;
		upperX = x1 / BLOCK_SIZE + 1;
	}
	if(z1 < z2) {
		lowerZ = z1 / BLOCK_SIZE;
		upperZ = z2 / BLOCK_SIZE + 1;
	} else {
		lowerZ = z2 / BLOCK_SIZE;
		upperZ = z1 / BLOCK_SIZE + 1;
	}

	//Error-check.
	upperX = min(upperX, nbrOfBlocksX - 1);
	upperZ = min(upperZ, nbrOfBlocksZ - 1);

	//Marking the blocks inside the rectangle.
	//Enqueing them from upper to lower becourse of
	//the placement of the bi-directional vertices.
	int x, z;
	for(z = upperZ; z >= lowerZ; z--)
		for(x = upperX; x >= lowerX; x--) {
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


/*
Updating some obsolete blocks.
Using FIFO-principle.
*/
void CPathEstimator::Update() {
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
		blockState[blocknr].options &= ~PATHOPT_OBSOLETE;

		//One block updated.
		counter++;
	}
}


/*
Storing data and doing some top-administration.
*/
IPath::SearchResult CPathEstimator::GetPath(const MoveData& moveData, float3 start, const CPathEstimatorDef& peDef, Path& path, unsigned int maxSearchedBlocks) {
	start.CheckInBounds();
	//Clear path.
	path.path.clear();
	path.pathCost = PATHCOST_INFINITY;

	//Initial calculations.
	maxBlocksToBeSearched = min(maxSearchedBlocks, MAX_SEARCHED_BLOCKS);
	startBlock.x = (int) (start.x / BLOCK_PIXEL_SIZE);
	startBlock.y = (int) (start.z / BLOCK_PIXEL_SIZE);
	startBlocknr = startBlock.y * nbrOfBlocksX + startBlock.x;

	//Search
	SearchResult result = InitSearch(moveData, peDef);

	//If successful, generate path.
	if(result == Ok || result == GoalOutOfRange) {
		FinishSearch(moveData, path);
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
	return result;
}


/*
Making some initial calculations and preparations.
*/
IPath::SearchResult CPathEstimator::InitSearch(const MoveData& moveData, const CPathEstimatorDef& peDef) {
	//Starting square is inside goal area?
	int xSquare = startBlock.x * BLOCK_SIZE + blockState[startBlocknr].sqrOffset[moveData.pathType].x;
	int zSquare = startBlock.y * BLOCK_SIZE + blockState[startBlocknr].sqrOffset[moveData.pathType].y;
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
IPath::SearchResult CPathEstimator::DoSearch(const MoveData& moveData, const CPathEstimatorDef& peDef) {
	bool foundGoal = false;
	while(!openBlocks.empty()
	&& (openBlockBufferPointer - openBlockBuffer) < (maxBlocksToBeSearched - 8)) {
		//Get the open block with lowest cost.
		OpenBlock* ob = openBlocks.top();
		openBlocks.pop();

		//Check if the block has been marked as unaccessable during it's time in queue.
		if(blockState[ob->blocknr].options & (PATHOPT_BLOCKED | PATHOPT_CLOSED | PATHOPT_FORBIDDEN))
			continue;

		//Check if the block has become obsolete (due to update) during it's time in queue.
		if(blockState[ob->blocknr].cost != ob->cost)
			continue;

		//Check if the goal is reached.
		int xBSquare = ob->block.x * BLOCK_SIZE + blockState[ob->blocknr].sqrOffset[moveData.pathType].x;
		int zBSquare = ob->block.y * BLOCK_SIZE + blockState[ob->blocknr].sqrOffset[moveData.pathType].y;
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
void CPathEstimator::TestBlock(const MoveData& moveData, const CPathEstimatorDef &peDef, OpenBlock& parentOpenBlock, unsigned int direction) {
	testedBlocks++;
	
	//Initial calculations of the new block.
	int2 block;
	block.x = parentOpenBlock.block.x + directionVector[direction].x;
	block.y = parentOpenBlock.block.y + directionVector[direction].y;
	int blocknr = block.y * nbrOfBlocksX + block.x;
	int vertexNbr = moveData.pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[direction];

	//Outside map?
	if(block.x < 0 || block.x >= nbrOfBlocksX
	|| block.y < 0 || block.y >= nbrOfBlocksZ
	|| vertexNbr < 0 || vertexNbr >= nbrOfVertices)
		return;

	int xSquare = block.x * BLOCK_SIZE + blockState[blocknr].sqrOffset[moveData.pathType].x;
	int zSquare = block.y * BLOCK_SIZE + blockState[blocknr].sqrOffset[moveData.pathType].y;

	//Check if the block is unavailable.
	if(blockState[blocknr].options & (PATHOPT_FORBIDDEN | PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return;

	//Check if the block is blocked or out of constraints.
	if(vertex[vertexNbr] >= PATHCOST_INFINITY || !peDef.WithinConstraints(xSquare, zSquare)) {
		blockState[blocknr].options |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(blocknr);
		return;
	}

	//Evaluate this node.
	float heuristicCost = peDef.Heuristic(xSquare, zSquare);
	float blockCost = vertex[vertexNbr];
	float currentCost = parentOpenBlock.currentCost + blockCost;
	float cost = currentCost + heuristicCost;

	//Check if the block is already in queue and better, then keep it.
	if(blockState[blocknr].options & PATHOPT_OPEN) {
		if(blockState[blocknr].cost <= cost)
			return;
		blockState[blocknr].options &= ~PATHDIR_ALL;
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
		int xGSquare = block.x * BLOCK_SIZE + goalSqrOffset.x;
		int zGSquare = block.y * BLOCK_SIZE + goalSqrOffset.y;
		//In first case trying to use a by goal defined offset...
		if(!moveData.moveMath->IsBlocked(moveData, moveMathOptions, xGSquare, zGSquare)) {
			float3 pos = SquareToFloat3(xGSquare, zGSquare);
			pos.y = moveData.moveMath->yLevel(xGSquare, zGSquare);
			path.path.push_back(pos);
		}
		//...if not possible, then use offset defined by the block.
		else {
			int xBSquare = block.x * BLOCK_SIZE + blockState[blocknr].sqrOffset[moveData.pathType].x;
			int zBSquare = block.y * BLOCK_SIZE + blockState[blocknr].sqrOffset[moveData.pathType].y;
			float3 pos = SquareToFloat3(xBSquare, zBSquare);
			pos.y = moveData.moveMath->yLevel(xBSquare, zBSquare);
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
		blockState[dirtyBlocks.back()].options &= ~PATHOPT_SEARCHRELATED;
		dirtyBlocks.pop_back();
	}
	testedBlocks = 0;
}


/*
Trying to read offset and vertices data from file.
Return false if failed.
TODO: Read-error-check.
*/
bool CPathEstimator::ReadFile(string name) {
	//Open file.
	string filename = string("maps\\") + stupidGlobalMapname.substr(0, stupidGlobalMapname.find('.') + 1) + name;
	CFileHandler file(filename.c_str());
	if(file.FileExists()) {
		//Check hash.
		unsigned int filehash = 0;
		unsigned int hash=Hash();
//		info->AddLine("%i",hash);
		file.Read(&filehash, 4);
		if(filehash != hash)
			return false;

		//Read block-center-offset data.
		int blocknr;
		for(blocknr = 0; blocknr < nbrOfBlocks; blocknr++) {
			file.Read(blockState[blocknr].sqrOffset, moveinfo->moveData.size() * sizeof(int2));
		}

		//Read vertices data.
		file.Read(vertex, nbrOfVertices * sizeof(float));
		
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
	//Open file.
	string filename = string("maps\\") + stupidGlobalMapname.substr(0, stupidGlobalMapname.find('.') + 1) + name;
	ofstream file(filename.c_str(), ios::out | ios::binary);
	if(file.good()) {
		//Write hash.
		unsigned int hash = Hash();
		file.write((char*)&hash, 4);

		//Write block-center-offsets.
		int blocknr;
		for(blocknr = 0; blocknr < nbrOfBlocks; blocknr++) {
			file.write((char*)blockState[blocknr].sqrOffset, moveinfo->moveData.size() * sizeof(int2));
		}

		//Write vertices.
		file.write((char*)vertex, nbrOfVertices * sizeof(float));
	}
	file.close();
}


/*
Gives a hash-code identifying the dataset of this estimator.
*/
unsigned int CPathEstimator::Hash() {
	return readmap->mapChecksum + moveinfo->moveInfoChecksum + BLOCK_SIZE + moveMathOptions + PATHESTIMATOR_VERSION;
}



//////////////////////////////////////////
// CRangedGoalWithCircularConstraintPFD //
//////////////////////////////////////////

/*
Constructor
Calculating the center and radius of the constrainted area.
*/
CRangedGoalWithCircularConstraintPFD::CRangedGoalWithCircularConstraintPFD(float3 start, float3 goal, float goalRadius) :
CRangedGoalPFD(goal, goalRadius),
start(start) {
	halfWay = (start + goal) / 2;
	searchRadius = start.distance2D(halfWay)*1.3 + SQUARE_SIZE*2;
}


/*
Tests if a square is inside is the circular constrainted area.
*/
bool CRangedGoalWithCircularConstraintPFD::WithinConstraints(int xSquare, int zSquare) const {
	return (SquareToFloat3(xSquare, zSquare).distance2D(halfWay) <= searchRadius);
}



////////////////////
// CRangedGoalPED //
////////////////////

/*
Give the relative offset of the goal inside the goal block
in a grid with given blocksize.
*/
int2 CRangedGoalPED::GoalSquareOffset(int blockSize) const {
	int blockPixelSize = blockSize * SQUARE_SIZE;
	int2 offset;
	offset.x = ((int)goal.x % blockPixelSize) / SQUARE_SIZE;
	offset.y = ((int)goal.z % blockPixelSize) / SQUARE_SIZE;
	return offset;
}
