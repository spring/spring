/*
Copyright (c) 2000-2005 Lee Thomason (www.grinninglizard.com)

Grinning Lizard Utilities.

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

/*
Updated and changed by Tournesol (on the TA Spring client).
May (currenty) only be used to compile KAI (AI for TA Spring).
Take care when you use it!

This notice may not be removed or altered from any source 
*/
//#define USE_ASSERTIONS

#ifdef _MSC_VER
#pragma warning( disable : 4786 )	// Debugger truncating names.
#pragma warning( disable : 4530 )	// Exception handler isn't used
#endif

#include "micropather.h"

using namespace micropather;

class OpenQueueBH
{
	public:
	
	OpenQueueBH() :
	ai(ai), size(0), heapArray(0)
	{
	}
	
	void setData(AIClasses *ai, PathNode** heapArray)
	{
		this->ai = ai;
		this->heapArray = heapArray;
	}

	void clear()
	{
		size = 0;
	}
	~OpenQueueBH() {}

	void Push(PathNode* pNode)
	{
		//assert(pNode->inOpen == 0);
		pNode->inOpen = 1;
		//L("Push: " << size);
		if(size)
		{
			// Insert the node at the last index
			/*
			for(int m = 1; m <= size; m++)
			{
				L("heapArray[m]->myIndex: " << heapArray[m]->myIndex << ", heapArray[m]: " << heapArray[m]);
			}*/
			size++;
			heapArray[size] = pNode; // Not realy needed
			pNode->myIndex =  size; // Self
			//pNode->inOpen = 1;
			/*
			for(int m = 1; m <= size; m++)
			{
				L("heapArray[m]->myIndex: " << heapArray[m]->myIndex << ", heapArray[m]: " << heapArray[m] );
			}*/
			//assert(heapArray[size]->myIndex == size);
			/*
			for(int m = 1; m <= size; m++)
			{
				assert(heapArray[m]->myIndex == m);
			}*/
			
			//float totalCostpNode = pNode->totalCost;
			// Heapify now
			//# define Parent = [i/2]
			// Left = [2i]
			// Right = [2i+1]
			int i = size;
			//L("while " << i);
			//PathNode* parent
			//if(i > 1)
			//{
				//pNode = heapArray[i];
				//int ishift = i >> 1;
				//PathNode* parent = heapArray[ishift];
				//while((i > 1) && (parent->totalCost > pNode->totalCost))
				//while((i > 1) && (heapArray[i >> 1]->totalCost > pNode->totalCost))
				while((i > 1) && (heapArray[i >> 1]->totalCost > heapArray[i]->totalCost))
				{
					// Swap them:
					//L("Swap them: " << i);
					PathNode* temp = heapArray[i >> 1];
					//PathNode* temp = parent;
					heapArray[i >> 1] = heapArray[i];
					//heapArray[ishift] = pNode;
					heapArray[i] = temp;
					
					//assert(temp->myIndex == (i >> 1));
					//assert(heapArray[i >> 1]->myIndex == i);
					
					temp->myIndex = i;
					//parent->myIndex = ishift;
					heapArray[i >> 1]->myIndex = i >> 1;
					
					//assert(heapArray[i]->myIndex == i);
					//assert(heapArray[i >> 1]->myIndex == (i >> 1));
					
					i >>= 1;
					//pNode = parent;
					//pNode = temp;
					//parent = heapArray[ishift];
				}
			//}
		}
		else
		{
			// The tree was empty
			//L("The tree was empty: " );
			//assert(false);
			size++;
			heapArray[1] = pNode;
			//pNode->inOpen = 1;
			//L("The tree was empty: 1" );
			pNode->myIndex = size;
			//L("The tree was empty: done" );
			//pNode->prev = 0;
		}
		/*
		for(int i = 1; i <= size; i++)
		{
			assert(heapArray[i]->myIndex == i);
		}*/
	}

	void Update(PathNode* pNode)
	{
		//L("Update: " << size);
		//assert(pNode->inOpen == 1);
		if(size > 1)
		{
			// Heapify now
			//# define Parent = [i/2]
			// Left = [2i]
			// Right = [2i+1]
			int i = pNode->myIndex;
			while(i > 1 && (heapArray[i >> 1]->totalCost > heapArray[i]->totalCost))
			{
				// Swap them:
				PathNode* temp = heapArray[i >> 1];
				heapArray[i >> 1] = heapArray[i];
				heapArray[i] = temp;
				
				//assert( temp->myIndex == (i >> 1));
				//assert(heapArray[i >> 1]->myIndex == i);
				temp->myIndex = i;
				heapArray[i >> 1]->myIndex =  i >> 1;
				i >>= 1;
			}
		}
		else
		{
			// The tree have only one member
		}
		/*
		for(int i = 1; i <= size; i++)
		{
			assert(heapArray[i]->myIndex == i);
		}*/
	}
	
	PathNode* Pop()
	{
		//L("Pop: " << size);
		//assert(size);
		
		// Get the first one:
		PathNode *min = heapArray[1];
		//assert(min->myIndex == 1);
		//assert(min->inOpen == 1);
		min->inOpen = 0;
		heapArray[1] = heapArray[size];
		size--;
		if(size == 0)
		{
			return min;	
		} 
		/*else if(size == 1)
		{
			heapArray[1]->myIndex = 1;
		}*/
		heapArray[1]->myIndex = 1;
		//# define Parent(x) (x >> 1)
		# define Left(x)  (x << 1)
		# define Right(x) ((x << 1) + 1)
		
		// Fix the heap
		int index = 1;
		int smalest = 1;
		{
			LOOPING_HEAPIFY:
			index = smalest;
			int left = Left(index);
			int right = Right(index);
			//L("left: " << left << ", index: " << index);
			//if(left <= size) L("heapArray[left]->totalCost: " << heapArray[left]->totalCost << ", heapArray[index]->totalCost: " << heapArray[index]->totalCost);
			//if(left <= size && heapArray[left]->totalCost < heapArray[index]->totalCost)
			if(left <= size && heapArray[left]->totalCost < heapArray[index]->totalCost)
				smalest = left;
			else
				smalest = index;
			//L("right: " << right << ", smalest: " << smalest);
			//if(right <= size) L("heapArray[right]->totalCost: " << heapArray[right]->totalCost << ", heapArray[smalest]->totalCost: " << heapArray[smalest]->totalCost);
			if(right <= size && heapArray[right]->totalCost < heapArray[smalest]->totalCost)
				smalest = right;
			//L("index: " << index << ", smalest: " << smalest);
			if(smalest != index)
			{
				// Swap them:
				PathNode* temp = heapArray[index];
				heapArray[index] = heapArray[smalest];
				heapArray[smalest] = temp;
				//heapArray[index] = (PathNode*)((int)heapArray[index] ^ (int)heapArray[smalest]);
				//heapArray[smalest] = (PathNode*)((int)heapArray[smalest] ^ (int)heapArray[index]);
				//heapArray[index] = (PathNode*)((int)heapArray[index] ^ (int)heapArray[smalest]);
				//assert( temp->myIndex == index);
				//assert( heapArray[index]->myIndex == smalest);
				
				temp->myIndex = smalest;
				//heapArray[smalest]->myIndex = smalest;
				heapArray[index]->myIndex = index;
				goto LOOPING_HEAPIFY;
			}
		
		}
		/*
		for(int i = 1; i <= size; i++)
		{
			assert(heapArray[i]->myIndex == i);
		}*/
		
		return min;
	}
	bool Empty()	{ return size == 0; }
	
	private:
	PathNode** heapArray;
	AIClasses *ai; // for debugging
	int size;
};

OpenQueueBH openQueueBH;


// ---------------------------------------------------

// Logger hack: 
//int popTime;
//int pushTime;
//int updateTime;
//int loopTime;
MicroPather::MicroPather(AIClasses *ai, unsigned allocate )
	: ALLOCATE( allocate ),
	  pathNodeMem( 0 ),
	  availMem( 0 ),
	  frame( 0 ),
	  checksum( 0 ),
	  ai( ai ),
	  hasStartedARun( false )
{
	AllocatePathNode();
	openQueueBH.setData(ai, heapArrayMem);
	
	//popTime = ai->math->GetNewTimerGroupNumber("MicroPather::Pop()");
	//pushTime = ai->math->GetNewTimerGroupNumber("MicroPather::Push()");
	//updateTime = ai->math->GetNewTimerGroupNumber("MicroPather::Update()");
	//loopTime = ai->math->GetNewTimerGroupNumber("MicroPather::FindBestPathStandard()");
}

// Make sure that costArray dont contain values below 1.0 (for speed), and below 0.0 (for eternal loop )
void MicroPather::SetMapData(unsigned int *canMoveIntMaskArray, float *costArray, int mapSizeX, int mapSizeY, unsigned int canMoveBitMask)
{
	this->canMoveIntMaskArray = canMoveIntMaskArray;
	this->canMoveBitMask = canMoveBitMask;
	this->costArray = costArray;
	this->mapSizeX = mapSizeX;
	this->mapSizeY = mapSizeY;
	
	if(mapSizeY * mapSizeX  > (int)ALLOCATE)
	{
		L("Error: 'mapSizeY * mapSizeX  > ALLOCATE' in pather");
		// Stop running:
		assert(!(mapSizeY * mapSizeX  > (int)ALLOCATE));
	}
	//L("pather: mapSizeX: " << mapSizeX << ", mapSizeY: " << mapSizeY << ", ALLOCATE: " << ALLOCATE);
	// Tournesol: make a fixed offset array
	// ***
	// *X*
	// ***

	// Smart ordering (do not change) ;)
	offsets[0] = - 1;
	offsets[1] = 1;
	offsets[2] = + mapSizeX;
	offsets[3] = - mapSizeX;
	offsets[4] = - mapSizeX -1;
	offsets[5] = - mapSizeX +1;
	offsets[6] = + mapSizeX -1;
	offsets[7] = + mapSizeX +1;
}

MicroPather::~MicroPather()
{
	free(pathNodeMem);
	free(heapArrayMem);
}

void MicroPather::Reset()
{
	L("Reseting pather, frame is: " << frame);
	for(unsigned i = 0; i < ALLOCATE; i++)
	{
		PathNode* directNode = &pathNodeMem[i];
		directNode->Reuse(0);
	}
	frame = 1;	
}

// Must only be called once...
PathNode* MicroPather::AllocatePathNode() 
{
	PathNode* result;
	if ( availMem == 0 ) {
	
		PathNode* newBlock = (PathNode*) malloc( sizeof(PathNode) * ALLOCATE);

		L("pathNodeMem: " << newBlock);
		
		pathNodeMem = newBlock;
		L(" sizeof(PathNode): " <<  sizeof(PathNode));
		availMem = ALLOCATE;
		
		L("availMem == " << (sizeof(PathNode) * ALLOCATE));
		// Make all the nodes in one go (step one)
		for(unsigned i = 0; i < ALLOCATE; i++)
		{
			result = &pathNodeMem[i];
			//--availMem;
			result->Init( 0, FLT_BIG, 0 );
			result->isEndNode = 0;
		}
		result = newBlock;

		heapArrayMem = (PathNode**) malloc( sizeof(PathNode*) * ALLOCATE);
		L("heapArrayMem: " << heapArrayMem);
	}
	else
	{
		// This is bad....
		bool AllocatePathNodeCalledTwice = false;
		assert(AllocatePathNodeCalledTwice);
	}
	//assert( availMem );
	return result;
}

void MicroPather::GoalReached( PathNode* node, unsigned start, unsigned end, vector< unsigned > &path )
{
	//path->clear(); // Not needed

	// We have reached the goal.
	// How long is the path? Used to allocate the vector which is returned.
	int count = 1;
	PathNode* it = node;
	while( it->parent )
	{
		++count;
		it = it->parent;
	}			

	// Now that the path has a known length, allocate
	// and fill the vector that will be returned.
	if ( count < 3 )
	{
		// Handle the short, special case.
		path.resize(2);
		path[0] = start;
		path[1] = end;
	}
	else
	{
		path.resize(count);

		path[0] = start;
		path[count-1] = end;

		count-=2;
		it = node->parent;

		while ( it->parent )
		{
			path[count] = (((unsigned long)it)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
			it = it->parent;
			--count;
		}
	}
	//checksum = 0;
	#ifdef DEBUG_PATH
	printf( "Path: " );
	int counter=0;
	#endif
	
	// The checksum is usefull (not)....
	/*
	for ( unsigned k=0; k<path->size(); ++k )
	{
		checksum += ((int)((*path)[k])) << (k%8);

		#ifdef DEBUG_PATH
		graph->PrintStateInfo( (*path)[k] );
		printf( " " );
		++counter;
		if ( counter == 8 )
		{
			printf( "\n" );
			counter = 0;
		}
		#endif
	}
	*/
	#ifdef DEBUG_PATH
	printf( "Cost=%.1f Checksum %d\n", node->costFromStart, checksum );
	#endif
}

float MicroPather::LeastCostEstimateLocal( int nodeStartIndex)
{
		int yStart = nodeStartIndex / mapSizeX;
		int xStart = nodeStartIndex - yStart * mapSizeX; // nodeStartIndex%mapSizeX;
		
		int dx = abs(xStart - xEndNode);
		int dy = abs(yStart - yEndNode);
		int strait = abs(dx - dy);
		//return (strait + 0.4 * strait + 1.41f * min( dx, dy));
		
		//float sum = (strait + 1.41f * min( dx, dy));
		//L("LeastCostEstimate: xStart:, " << xStart << ", dx: " << dx << ", yStart" << yStart << ", dy: " << dy  << ", strait: " << strait << ",  min( dx, dy):" <<  min( dx, dy) <<  ", sum: " << sum );
		return (strait + 1.41f * min( dx, dy));
}

void MicroPather::FixStartEndNode( unsigned& startNode, unsigned& endNode )
{
	FixNode(startNode);
	// No node can be at the edge !
	int y = endNode / mapSizeX;
	int x = endNode - y * mapSizeX;
	// No node can be at the edge !
	if(x == 0)
		x = 1;
	else if(x == mapSizeX)
		x = mapSizeX -1;
	
	if(y == 0)
		y = 1;
	else if(y == mapSizeY)
		y = mapSizeY -1;
	xEndNode = x;
	yEndNode = y;
	endNode = (unsigned)(y*mapSizeX+x);
}

void MicroPather::FixNode(unsigned& node)
{
	int y = node / mapSizeX;
	int x = node - y * mapSizeX;
	//assert(node <= mapSizeX * mapSizeY);
	
	// No node can be at the edge !
	// But fixing this here might be bad
	if(x == 0)
		x = 1;
	else if(x == mapSizeX)
		x = mapSizeX -1;
	
	if(y == 0)
		y = 1;
	else if(y == mapSizeY)
		y = mapSizeY -1;
	node = (y*mapSizeX+x);
}

int MicroPather::SetupStart(unsigned startNode, unsigned endNode, float& cost )
{
	assert(!hasStartedARun);
	hasStartedARun = true;
	cost = 0.0f;
	if ( startNode == endNode )
	{
		hasStartedARun = false;
		return START_END_SAME;
	}
	FixStartEndNode(startNode, endNode);
	if(!(canMoveIntMaskArray[startNode] & canMoveBitMask))
	{
		// Cant move from the start ???
		// Dont do anything...
		L("Pather: trying to move from a blokked start pos. Mask: " << canMoveBitMask);
	}
	return SOLVED;
}

void MicroPather::SetupEnd(unsigned startNode)
{
	++frame;
	if(frame > 65534)
	{
		L("frame > 65534, pather reset needed");
		Reset();
	}
	// Reset the priority queue
	openQueueBH.clear();
	// Add the start node to the queue
	PathNode* tempStartNode = &pathNodeMem[startNode];
	tempStartNode->Reuse( frame );
	tempStartNode->costFromStart = 0;
	tempStartNode->totalCost = LeastCostEstimateLocal(startNode );
	openQueueBH.Push( tempStartNode );
}


int MicroPather::Solve(unsigned startNode,unsigned endNode, vector<unsigned>& path, float& cost )
{
	if(SetupStart(startNode, endNode, cost) == START_END_SAME)
		return START_END_SAME;

	if(!(canMoveIntMaskArray[endNode] & canMoveBitMask))
	{
		// Cant move into the endNode
		// Just fail fast
		
		L("EndNode blocked, no pathing: " << endNode << ", x: " << xEndNode << ", y: " << yEndNode);
		hasStartedARun = false;
		return NO_SOLUTION;
	}
	SetupEnd(startNode);
	
	// Set the endnode
	PathNode *endPathNode = &(pathNodeMem[endNode]);
	endPathNode->isEndNode = 1;
	// Run a normal solve
	//PathNode* foundNode = FindBestPathStandard();
	PathNode* foundNode = FindBestPathUndirected();
	endPathNode->isEndNode = 0;
	if(endPathNode == foundNode)
	{
		// Make the path now:
		GoalReached( endPathNode, startNode, endNode, path );
		cost = endPathNode->costFromStart;
		hasStartedARun = false;
		return SOLVED;	
	}
	hasStartedARun = false;
	return NO_SOLUTION;
}

int MicroPather::FindBestPathToAnyGivenPoint( unsigned startNode, vector<unsigned>& endNodes, vector<unsigned>& path, float& cost, float cutoff)
{
	cost = 0.0f;
	if(endNodes.size() <= 0)
	{
		// Just fail fast
		return NO_SOLUTION;
	}
	unsigned endNode = endNodes[0];
	if(SetupStart(startNode, endNode, cost) == START_END_SAME)
		return START_END_SAME;
	SetupEnd(startNode);

	// Mark the endNodes
	for(unsigned i = 0; i < endNodes.size(); i++)
	{
		FixNode(endNodes[i]);
		//PathNode* tempEndNode = &pathNodeMem[endNodes[i]];
		//assert(tempEndNode->isEndNode == 0);
		pathNodeMem[endNodes[i]].isEndNode = 1;
	}
	// Run a normal solve
	//PathNode* foundNode = FindBestPathStandard();
	PathNode* foundNode;
	assert(cutoff >= 0);
	if(cutoff == 0)
		foundNode = FindBestPathUndirected();
	else
		foundNode = FindBestPathUndirectedCutoff(cutoff);
	// Unmark the endNodes
	for(unsigned i = 0; i < endNodes.size(); i++)
	{
		//PathNode* tempEndNode = &pathNodeMem[endNodes[i]];
		pathNodeMem[endNodes[i]].isEndNode = 0;
	}
	if(foundNode != NULL)
	{
		// Make the path now:
		GoalReached( foundNode, startNode, (((unsigned long)foundNode)-((unsigned long)pathNodeMem)) / sizeof(PathNode), path );
		cost = foundNode->costFromStart;
		hasStartedARun = false;
		return SOLVED;	
	}
	hasStartedARun = false;
	return NO_SOLUTION;
}

int MicroPather::FindBestPathToPrioritySet( unsigned startNode, vector<unsigned>& endNodes, vector< unsigned >& path, unsigned& priorityIndexFound, float& cost)
{
	cost = 0.0f;
	if(endNodes.size() <= 0)
	{
		// Just fail fast
		return NO_SOLUTION;
	}
	//unsigned endNode = endNodes[0];
	if(SetupStart(startNode, endNodes[0], cost) == START_END_SAME)
		return START_END_SAME;
	SetupEnd(startNode);
	PathNode* tempEndNode = &pathNodeMem[endNodes[0]];
	tempEndNode->isEndNode = 1;
	//PathNode* foundNode = FindBestPathStandard();
	PathNode* foundNode = FindBestPathUndirected();
	tempEndNode->isEndNode = 0;
	// Test each end node:
	for(unsigned i = 0; i < endNodes.size(); i++)
	{
		FixNode(endNodes[i]);
		foundNode = &pathNodeMem[endNodes[i]];
		if(foundNode->frame == frame)
		{
			// That node was updated this frame, so it was pathed too.
			// This means its reachable from the startnode.
			// Make the path now:
			GoalReached( foundNode, startNode, endNodes[i], path );
			cost = foundNode->costFromStart;
			priorityIndexFound = i;
			hasStartedARun = false;
			return SOLVED;	
		}
	}
	hasStartedARun = false;
	return NO_SOLUTION;
}


PathNode* MicroPather::FindBestPathStandard()
{	
	//ai->math->StartTimer(loopTime);
	OpenQueueBH open = openQueueBH;
	while ( !open.Empty() )
	{
		//ai->math->StartTimer(popTime);
		PathNode* node = open.Pop();
		//ai->math->StopTimer(popTime);
		unsigned long indexStart = (((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
		if ( node->isEndNode )
		{
			//ai->math->StopTimer(loopTime);
			return node;
		}
		// We have not reached the goal - add the neighbors.
		//unsigned indexStart = (((unsigned)node)-((unsigned)pathNodeMem)) / sizeof(PathNode);
		#ifdef USE_ASSERTIONS
		int ystart = indexStart / mapSizeX;
		int xstart = indexStart - ystart * mapSizeX;
		
		// No node can be at the edge !
		assert(xstart != 0 && xstart != mapSizeX);
		assert(ystart != 0 && ystart != mapSizeY);
		#endif

		float nodeCostFromStart = node->costFromStart;
		for( int i=0; i < 8; ++i )
		{
			int indexEnd = offsets[i] + indexStart;
			
			//if ( !canMoveArray[indexEnd] ) {
			//if ( !(canMoveIntMaskArray[indexEnd] & canMoveBitMask)) {
			if ( (canMoveIntMaskArray[indexEnd] & canMoveBitMask) != canMoveBitMask) {
				continue;
			}
			PathNode* directNode = &pathNodeMem[indexEnd];
			
			if ( directNode->frame != frame )
				directNode->Reuse( frame );

			#ifdef USE_ASSERTIONS
			int yend = indexEnd / mapSizeX;
			int xend = indexEnd - yend * mapSizeX;
			
			
			// We can move to that spot
			assert(canMoveArray[yend*mapSizeX + xend]);
			

			// No node can be at the edge !
			assert(xend != 0 && xend != mapSizeX);
			assert(yend != 0 && yend != mapSizeY);
			#endif
			
			float newCost = nodeCostFromStart;
			//bool moveingInX = xstart - xend;
			//bool moveingInY = ystart - yend;
			//if(moveingInX && moveingInY)
			//if((xstart - xend) & (ystart - yend))
			if(i > 3)
			{
				newCost += costArray[indexEnd]*1.41f;//+0.4;
			}
			else
				newCost += costArray[indexEnd];//+0.4;
			
			if(directNode->costFromStart <= newCost)
				continue;	// Do nothing. This path is not better than existing.
			
			// Its better, update its data
			directNode->parent = node;
			directNode->costFromStart = newCost;
			directNode->totalCost = newCost + LeastCostEstimateLocal(indexEnd);
			#ifdef USE_ASSERTIONS
			assert(indexEnd == ((((unsigned)directNode)-((unsigned)pathNodeMem)) / sizeof(PathNode)));
			//assert( inClosed->totalCost < FLT_MAX );
			#endif
			
			if(directNode->inOpen)
			{
				//ai->math->StartTimer(updateTime);
				open.Update( directNode );
				//ai->math->StopTimer(updateTime);
			}
			else
			{
				directNode->inClosed = 0;
				//ai->math->StartTimer(pushTime);
				open.Push( directNode );
				//ai->math->StopTimer(pushTime);
			}
		}
		node->inClosed = 1;			
	}
	//ai->math->StopTimer(loopTime);
	return NULL;		
}

PathNode* MicroPather::FindBestPathUndirected()
{	
	//ai->math->StartTimer(loopTime);
	OpenQueueBH open = openQueueBH;
	while ( !open.Empty() )
	{
		//ai->math->StartTimer(popTime);
		PathNode* node = open.Pop();
		//ai->math->StopTimer(popTime);
		unsigned long indexStart = (((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
		if ( node->isEndNode )
		{
			//ai->math->StopTimer(loopTime);
			return node;
		}
		// We have not reached the goal - add the neighbors.
		//unsigned indexStart = (((unsigned)node)-((unsigned)pathNodeMem)) / sizeof(PathNode);
		#ifdef USE_ASSERTIONS
		int ystart = indexStart / mapSizeX;
		int xstart = indexStart - ystart * mapSizeX;
		
		// No node can be at the edge !
		assert(xstart != 0 && xstart != mapSizeX);
		assert(ystart != 0 && ystart != mapSizeY);
		#endif

		float nodeCostFromStart = node->costFromStart;
		for( int i=0; i < 8; ++i )
		{
			int indexEnd = offsets[i] + indexStart;
			
			//if ( !canMoveArray[indexEnd] ) {
			//if ( !(canMoveIntMaskArray[indexEnd] & canMoveBitMask)) {
			if ( (canMoveIntMaskArray[indexEnd] & canMoveBitMask) != canMoveBitMask) {
				continue;
			}
			PathNode* directNode = &pathNodeMem[indexEnd];
			
			if ( directNode->frame != frame )
				directNode->Reuse( frame );

			#ifdef USE_ASSERTIONS
			int yend = indexEnd / mapSizeX;
			int xend = indexEnd - yend * mapSizeX;
			
			
			// We can move to that spot
			assert(canMoveArray[yend*mapSizeX + xend]);
			

			// No node can be at the edge !
			assert(xend != 0 && xend != mapSizeX);
			assert(yend != 0 && yend != mapSizeY);
			#endif
			
			float newCost = nodeCostFromStart;
			//bool moveingInX = xstart - xend;
			//bool moveingInY = ystart - yend;
			//if(moveingInX && moveingInY)
			//if((xstart - xend) & (ystart - yend))
			if(i > 3)
			{
				newCost += costArray[indexEnd]*1.41f;//+0.4;
			}
			else
				newCost += costArray[indexEnd];//+0.4;
			
			if(directNode->costFromStart <= newCost)
				continue;	// Do nothing. This path is not better than existing.
			
			// Its better, update its data
			directNode->parent = node;
			directNode->costFromStart = newCost;
			directNode->totalCost = newCost;// + LeastCostEstimateLocal(indexEnd); // Remove the estimate
			#ifdef USE_ASSERTIONS
			assert(indexEnd == ((((unsigned)directNode)-((unsigned)pathNodeMem)) / sizeof(PathNode)));
			//assert( inClosed->totalCost < FLT_MAX );
			#endif
			
			if(directNode->inOpen)
			{
				//ai->math->StartTimer(updateTime);
				open.Update( directNode );
				//ai->math->StopTimer(updateTime);
			}
			else
			{
				directNode->inClosed = 0;
				//ai->math->StartTimer(pushTime);
				open.Push( directNode );
				//ai->math->StopTimer(pushTime);
			}
		}
		node->inClosed = 1;			
	}
	//ai->math->StopTimer(loopTime);
	return NULL;		
}

PathNode* MicroPather::FindBestPathUndirectedCutoff(float cutoff)
{	
	//ai->math->StartTimer(loopTime);
	OpenQueueBH open = openQueueBH;
	while ( !open.Empty() )
	{
		//ai->math->StartTimer(popTime);
		PathNode* node = open.Pop();
		//ai->math->StopTimer(popTime);
		unsigned long indexStart = (((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
		if ( node->isEndNode )
		{
			//ai->math->StopTimer(loopTime);
			return node;
		}
		// We have not reached the goal - add the neighbors.
		//unsigned indexStart = (((unsigned)node)-((unsigned)pathNodeMem)) / sizeof(PathNode);
		#ifdef USE_ASSERTIONS
		int ystart = indexStart / mapSizeX;
		int xstart = indexStart - ystart * mapSizeX;
		
		// No node can be at the edge !
		assert(xstart != 0 && xstart != mapSizeX);
		assert(ystart != 0 && ystart != mapSizeY);
		#endif

		float nodeCostFromStart = node->costFromStart;
		for( int i=0; i < 8; ++i )
		{
			int indexEnd = offsets[i] + indexStart;
			
			//if ( !canMoveArray[indexEnd] ) {
			//if ( !(canMoveIntMaskArray[indexEnd] & canMoveBitMask) || cutoff < costArray[indexEnd]) {
			if ( ((canMoveIntMaskArray[indexEnd] & canMoveBitMask) != canMoveBitMask) || cutoff < costArray[indexEnd]) {
				continue;
			}
			PathNode* directNode = &pathNodeMem[indexEnd];
			
			if ( directNode->frame != frame )
				directNode->Reuse( frame );

			#ifdef USE_ASSERTIONS
			int yend = indexEnd / mapSizeX;
			int xend = indexEnd - yend * mapSizeX;
			
			
			// We can move to that spot
			assert(canMoveArray[yend*mapSizeX + xend]);
			

			// No node can be at the edge !
			assert(xend != 0 && xend != mapSizeX);
			assert(yend != 0 && yend != mapSizeY);
			#endif
			
			float newCost = nodeCostFromStart;
			//bool moveingInX = xstart - xend;
			//bool moveingInY = ystart - yend;
			//if(moveingInX && moveingInY)
			//if((xstart - xend) & (ystart - yend))
			if(i > 3)
			{
				newCost += costArray[indexEnd]*1.41f;//+0.4;
			}
			else
				newCost += costArray[indexEnd];//+0.4;
			
			if(directNode->costFromStart <= newCost)
				continue;	// Do nothing. This path is not better than existing.
			
			// Its better, update its data
			directNode->parent = node;
			directNode->costFromStart = newCost;
			directNode->totalCost = newCost;// + LeastCostEstimateLocal(indexEnd); // Remove the estimate
			#ifdef USE_ASSERTIONS
			assert(indexEnd == ((((unsigned)directNode)-((unsigned)pathNodeMem)) / sizeof(PathNode)));
			//assert( inClosed->totalCost < FLT_MAX );
			#endif
			
			if(directNode->inOpen)
			{
				//ai->math->StartTimer(updateTime);
				open.Update( directNode );
				//ai->math->StopTimer(updateTime);
			}
			else
			{
				directNode->inClosed = 0;
				//ai->math->StartTimer(pushTime);
				open.Push( directNode );
				//ai->math->StopTimer(pushTime);
			}
		}
		node->inClosed = 1;			
	}
	//ai->math->StopTimer(loopTime);
	return NULL;		
}


/*
// Debug painer: 
float3 pos;
int multiplier = 8 * 8;
pos.z = ystart * multiplier; /////////////////////OPTiMIZE
pos.x = xstart * multiplier;
char c[20];
sprintf(c,"%i",counter);
AIHCAddMapPoint amp; 
amp.pos=pos;
amp.label= c;
ai->cb->HandleCommand(&amp);
counter++;
*/

