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


//#define DEBUG_PATH
//#define DEBUG_PATH_DEEP
//#define USE_LIST


#define USE_NODEATINDEX100

#include "micropather.h"

//using namespace std;
using namespace micropather;

class OpenQueueBH
{
	public:
	
	OpenQueueBH(AIClasses *ai, PathNode** heapArray) :
	ai(ai), size(0)
	{
		this->heapArray = heapArray;
	}
	
	~OpenQueueBH() {}

	void Push(PathNode* pNode)
	{
		//assert(pNode->inOpen == 0);
		pNode->inOpen = 1;
		////L("Push: " << size);
		if(size)
		{
			// Insert the node at the last index
			/*
			for(int m = 1; m <= size; m++)
			{
				//L("heapArray[m]->myIndex: " << heapArray[m]->myIndex << ", heapArray[m]: " << heapArray[m]);
			}*/
			size++;
			heapArray[size] = pNode; // Not realy needed
			pNode->myIndex =  size; // Self
			//pNode->inOpen = 1;
			/*
			for(int m = 1; m <= size; m++)
			{
				//L("heapArray[m]->myIndex: " << heapArray[m]->myIndex << ", heapArray[m]: " << heapArray[m] );
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
			////L("while " << i);
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
					////L("Swap them: " << i);
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
			////L("The tree was empty: " );
			//assert(false);
			size++;
			heapArray[1] = pNode;
			//pNode->inOpen = 1;
			////L("The tree was empty: 1" );
			pNode->myIndex = size;
			////L("The tree was empty: done" );
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
		////L("Update: " << size);
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
		////L("Pop: " << size);
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
		# define Parent(x) (x >> 1)
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
			////L("left: " << left << ", index: " << index);
			//if(left <= size) //L("heapArray[left]->totalCost: " << heapArray[left]->totalCost << ", heapArray[index]->totalCost: " << heapArray[index]->totalCost);
			if(left <= size && heapArray[left]->totalCost < heapArray[index]->totalCost)
				smalest = left;
			else
				smalest = index;
			////L("right: " << right << ", smalest: " << smalest);
			//if(right <= size) //L("heapArray[right]->totalCost: " << heapArray[right]->totalCost << ", heapArray[smalest]->totalCost: " << heapArray[smalest]->totalCost);
			if(right <= size && heapArray[right]->totalCost < heapArray[smalest]->totalCost)
				smalest = right;
			////L("index: " << index << ", smalest: " << smalest);
			if(smalest != index)
			{
				// Swap them:
				PathNode* temp = heapArray[index];
				heapArray[index] = heapArray[smalest];
				heapArray[smalest] = temp;
				
				//assert( temp->myIndex == index);
				//assert( heapArray[index]->myIndex == smalest);
				
				temp->myIndex = smalest;
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


// Logger hack: 
MicroPather::MicroPather( Graph* _graph, AIClasses *ai, unsigned allocate )
	: ALLOCATE( allocate ),
	  BLOCKSIZE( allocate-1 ),
	  graph( _graph ),
	  pathNodeMem( 0 ),
	  availMem( 0 ),
	  pathNodeCount( 0 ),
	  frame( 0 ),
	  checksum( 0 )
{
	this->ai = ai;
	AllocatePathNode();
	hasStartedARun = false;
}

// Make sure that costArray dont contain values below 1.0 (for speed), and below 0.0 (for eternal loop )
void MicroPather::SetMapData(bool *canMoveArray, float *costArray, int mapSizeX, int mapSizeY)
{
	this->canMoveArray = canMoveArray;
	this->costArray = costArray;
	this->mapSizeX = mapSizeX;
	this->mapSizeY = mapSizeY;
	
	if(mapSizeY * mapSizeX  > (int)ALLOCATE)
	{
		//L("Error: 'mapSizeY * mapSizeX  > ALLOCATE' in pather");
		// Stop running:
		assert(!(mapSizeY * mapSizeX  > (int)ALLOCATE));
	}
	////L("pather: mapSizeX: " << mapSizeX << ", mapSizeY: " << mapSizeY << ", ALLOCATE: " << ALLOCATE);
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
	free(pathNodeMemForFree);
	free(heapArrayMem);
}

    
void MicroPather::Reset()
{
	//L("Reseting pather, frame is: " << frame);
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

		//L("pathNodeMemForFree: " << ((unsigned)newBlock));
		
		pathNodeMemForFree = newBlock;
		pathNodeMem = newBlock;
		//L(" sizeof(PathNode): " <<  sizeof(PathNode));
		availMem = BLOCKSIZE;
		
		//L("availMem == " << (sizeof(PathNode) * ALLOCATE));
		// Make all the nodes in one go (step one)
		for(unsigned i = 0; i < ALLOCATE; i++)
		{
			result = &pathNodeMem[i];// + (BLOCKSIZE	- availMem );
			//--availMem;
			++pathNodeCount;
			result->Init( 0, FLT_BIG, 0 );
			result->isEndNode = 0;
		}
		result = newBlock;
		
		
		// 
		heapArrayMem = (PathNode**) malloc( sizeof(PathNode*) * ALLOCATE);
		//L("heapArrayMem: " << heapArrayMem);
		
		
	}
	else
	{
		// This is bad....
		bool AllocatePathNodeCalledTwice = false;
		assert(AllocatePathNodeCalledTwice);
	}
	//assert( availMem );
	//L("AllocatePathNode()");
	return result;
}

void MicroPather::GoalReached( PathNode* node, void* start, void* end, vector< void* > *path )
{
	path->clear();

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
		path->resize(2);
		(*path)[0] = start;
		(*path)[1] = end;
	}
	else
	{
		path->resize(count);

		(*path)[0] = start;
		(*path)[count-1] = end;

		count-=2;
		it = node->parent;

		while ( it->parent )
		{
			(*path)[count] = (void*) ((((unsigned long)it)-((unsigned long)pathNodeMem)) / sizeof(PathNode));
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
		int xStart = nodeStartIndex - yStart * mapSizeX;
		
		int dx = abs(xStart - xEndNode);
		int dy = abs(yStart - yEndNode);
		int strait = abs(dx - dy);
		//return (strait + 0.4 * strait + 1.41f * min( dx, dy));
		
		//float sum = (strait + 1.41f * min( dx, dy));
		////L("LeastCostEstimate: xStart:, " << xStart << ", dx: " << dx << ", yStart" << yStart << ", dy: " << dy  << ", strait: " << strait << ",  min( dx, dy):" <<  min( dx, dy) <<  ", sum: " << sum );
		return (strait + 1.41f * min( dx, dy));
}

void MicroPather::FixStartEndNode( void** startNode, void** endNode )
{
	long index = (long)*startNode;
	int y = index / mapSizeX;
	int x = index - y * mapSizeX;
	
	// No node can be at the edge !
	if(x == 0)
		x = 1;
	else if(x == mapSizeX)
		x = mapSizeX -1;
	
	if(y == 0)
		y = 1;
	else if(y == mapSizeY)
		y = mapSizeY -1;
	
	*startNode = (void*)(y*mapSizeX+x);
	
	index = (long)*endNode;
	y = index / mapSizeX;
	x = index - y * mapSizeX;
	
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
	*endNode = (void*)(y*mapSizeX+x);
}

void MicroPather::FixNode( void** Node)
{
	long index = (long)*Node;
	int y = index / mapSizeX;
	int x = index - y * mapSizeX;
	
	assert(index >= 0);
	assert(index <= mapSizeX * mapSizeY);
	
	// No node can be at the edge !
	if(x == 0)
		x = 1;
	else if(x == mapSizeX)
		x = mapSizeX -1;
	
	if(y == 0)
		y = 1;
	else if(y == mapSizeY)
		y = mapSizeY -1;
	
	*Node = (void*)(y*mapSizeX+x);
}



int MicroPather::Solve( void* startNode, void* endNode, vector< void* >* path, float* cost )
{
	assert(!hasStartedARun);
	hasStartedARun = true;
	*cost = 0.0f;

	if ( startNode == endNode )
	{
		hasStartedARun = false;
		return START_END_SAME;
	}
	
	{
		FixStartEndNode(&startNode, &endNode);
		if(!canMoveArray[(long)startNode])
		{
			// Cant move from the start ???
			// Dont do anything...
			//L("Pather: trying to move from a blokked start pos");
		}
		if(!canMoveArray[(long)endNode])
		{
			// Cant move into the endNode
			// Just fail fast
			hasStartedARun = false;
			return NO_SOLUTION;
		}
	}
	
	++frame;
	if(frame > 65534)
	{
		//L("frame > 65534, pather reset needed");
		Reset();
	}
	// Make the priority queue
	OpenQueueBH open(ai, heapArrayMem);

	{
		PathNode* tempStartNode = &pathNodeMem[(unsigned long)startNode];
		float estToGoal = LeastCostEstimateLocal( (unsigned long)startNode );
		tempStartNode->Reuse( frame );
		tempStartNode->costFromStart = 0;
		tempStartNode->totalCost = estToGoal;
		open.Push( tempStartNode );
	}
	
	PathNode *endPathNode = &(pathNodeMem[(unsigned long)endNode]);
	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();
		
		if ( node == endPathNode )
		{
			GoalReached( node, startNode, endNode, path );
			*cost = node->costFromStart;
			hasStartedARun = false;
			return SOLVED;
		}
		else
		{
			// We have not reached the goal - add the neighbors.
			// emulate GetNodeNeighbors:
			int indexStart = (((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
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
				
				if ( !canMoveArray[indexEnd] ) {
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
					open.Update( directNode );
				}
				else
				{
					directNode->inClosed = 0;
					open.Push( directNode );
				}

			}
		}
		node->inClosed = 1;			
	}
	hasStartedARun = false;
	return NO_SOLUTION;		
}

int MicroPather::FindBestPathToAnyGivenPoint( void* startNode, vector<void*> endNodes, vector< void* >* path, float* cost)
{
	assert(!hasStartedARun);
	hasStartedARun = true;
	*cost = 0.0f;
	
	for(unsigned i = 0; i < ALLOCATE; i++)
	{
		PathNode* theNode = &pathNodeMem[i];
		if(theNode->isEndNode)
		{	
			//L("i: " << i << ", " << theNode->isEndNode);
			theNode->isEndNode = 0;
			assert(theNode->isEndNode == 0);
		}
	}
	
	
	if(endNodes.size() <= 0)
	{
		// Just fail fast
		hasStartedARun = false;
		return NO_SOLUTION;
	}
	
	{
		
		void* endNode = endNodes[0];
		FixStartEndNode(&startNode, &endNode);
		if(!canMoveArray[(long)startNode])
		{
			// Cant move from the start ???
			// Dont do anything...
			//L("Pather: trying to move from a blokked start pos");
		}
	}
	
	++frame;
	if(frame > 65534)
	{
		//L("frame > 65534, pather reset needed");
		Reset();
	}
	// Make the priority queue
	OpenQueueBH open(ai, heapArrayMem);
	
	{
		PathNode* tempStartNode = &pathNodeMem[(unsigned long)startNode];
		float estToGoal = LeastCostEstimateLocal( (unsigned long)startNode);
		tempStartNode->Reuse( frame );
		tempStartNode->costFromStart = 0;
		tempStartNode->totalCost = estToGoal;
		//assert(tempStartNode->isEndNode == 0);
		open.Push( tempStartNode );
	}
	
	// Mark the endNodes
	for(unsigned i = 0; i < endNodes.size(); i++)
	{
		FixNode(&endNodes[i]);
		PathNode* tempEndNode = &pathNodeMem[(unsigned long)endNodes[i]];
		
		//assert(tempEndNode->isEndNode == 0);
		tempEndNode->isEndNode = 1;
	}
	
	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();
		
		if ( node->isEndNode )
		{
			void* theEndNode = (void *) ((((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode));
			GoalReached( node, startNode, theEndNode, path );
			*cost = node->costFromStart;
			hasStartedARun = false;
			
			// Unmark the endNodes
			for(unsigned i = 0; i < endNodes.size(); i++)
			{
				PathNode* tempEndNode = &pathNodeMem[(unsigned long)endNodes[i]];
				tempEndNode->isEndNode = 0;
			}
			
			return SOLVED;
		}
		else
		{
			// We have not reached the goal - add the neighbors.
			// emulate GetNodeNeighbors:
			int indexStart = (((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
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
				
				if ( !canMoveArray[indexEnd] ) {
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
					open.Update( directNode );
				}
				else
				{
					directNode->inClosed = 0;
					open.Push( directNode );
				}

			}
		}
		node->inClosed = 1;			
	}
	// Unmark the endNodes
	for(unsigned i = 0; i < endNodes.size(); i++)
	{
		PathNode* tempEndNode = &pathNodeMem[(unsigned long)endNodes[i]];
		tempEndNode->isEndNode = 0;
	}
	hasStartedARun = false;
	return NO_SOLUTION;		
}


int MicroPather::FindBestPathToPointOnRadius( void* startNode, void* endNode, vector< void* >* path, float* cost, int radius)
{
	assert(!hasStartedARun);
	hasStartedARun = true;
	*cost = 0.0f;

	if(radius <= 0)
	{
		// Just fail fast
		hasStartedARun = false;
		return NO_SOLUTION;
	}
	
	{
		FixStartEndNode(&startNode, &endNode);
		if(!canMoveArray[(long)startNode])
		{
			// Cant move from the start ???
			// Dont do anything...
			//L("Pather: trying to move from a blokked start pos");
		}
	}
	
	++frame;
	if(frame > 65534)
	{
		//L("frame > 65534, pather reset needed");
		Reset();
	}
	// Make the priority queue
	OpenQueueBH open(ai, heapArrayMem);

	{
		PathNode* tempStartNode = &pathNodeMem[(unsigned long)startNode];
		float estToGoal = LeastCostEstimateLocal( (unsigned long)startNode);
		tempStartNode->Reuse( frame );
		tempStartNode->costFromStart = 0;
		tempStartNode->totalCost = estToGoal;
		open.Push( tempStartNode );
	}
	
	// Make the radius
	
	long indexEnd = (long) endNode;
	int y = indexEnd / mapSizeX;
	int x = indexEnd - y * mapSizeX;
	
	
	
	int* xend = new int[2*radius+1];
	for (int a=0;a<2*radius+1;a++){ 
		float z=a-radius;
		float floatsqrradius = radius*radius;
		xend[a]=int(sqrtf(floatsqrradius-z*z));
		
		int xStart = x - xend[a];
		int xEnd = x + xend[a];
		
		////L("xend[a]: " << xend[a]);
		////L("xStart: " << xStart << ", xEnd: " << xEnd);
	}
	
	/*  Add this test for all border nodes ??
	if(!canMoveArray[(long)endNode])
	{
		// Cant move into the endNode
		// Just fail fast
		hasStartedARun = false;
		return NO_SOLUTION;
	}
	*/
	
	/*
	// Debug painer: 
	for (int a=0;a<2*radius+1;a++){ 
		float3 pos;
		int multiplier = 8 * 8;
		long indexStart = (long)endNode;
		int ystart = indexStart / mapSizeX;
		int xstart = indexStart - ystart * mapSizeX;
		
		pos.z = (ystart + a - radius) * multiplier; /////////////////////OPTiMIZE
		pos.x = (xstart +  xend[a]) * multiplier ;
		AIHCAddMapPoint amp; 
		amp.pos=pos;
		amp.label= "X";
		////ai->cb->HandleCommand(&amp);
		float3 pos2;
		pos2.z = (ystart + a - radius) * multiplier; /////////////////////OPTiMIZE
		pos2.x = (xstart - xend[a]) * multiplier ;
		AIHCAddMapPoint amp2; 
		amp2.pos=pos2;
		amp2.label= "X";
		////ai->cb->HandleCommand(&amp2);
	}
	int counter = 0;
	*/
	
	////L(".");
	////L("yEndNode: " << yEndNode << ", xEndNode: " << xEndNode);
	PathNode *endPathNode = &(pathNodeMem[(unsigned long)endNode]);
	
	while ( !open.Empty() )
	{
		PathNode* node = open.Pop();
		
		int indexStart = (((unsigned long)node)-((unsigned long)pathNodeMem)) / sizeof(PathNode);
		int ystart = indexStart / mapSizeX;
		int xstart = indexStart - ystart * mapSizeX;
		////L("counter: " << counter << ", ystart: " << ystart << ", xstart: " << xstart);
		
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
		////ai->cb->HandleCommand(&amp);
		counter++;
		*/
		
		// Do a box test  (slow/test)
		// Note that a <= x <= b  is the same as x-a <= b - a  ()
		if(y - radius <= ystart && ystart <= y + radius  &&  x - radius <= xstart && xstart <= x + radius)
		{
			// We are in range (x and y direction)
			// Find the relative pos from endNode
			int relativeY = ystart - (yEndNode - radius);
			int relativeX = abs(xstart - xEndNode);
			////L("relativeY: " << relativeY << ", relativeX: " << relativeX);
			if(relativeX <= xend[relativeY])
			{
				// Its a hit
				////L("Its a hit: " << counter);
				
				GoalReached( node, startNode, (void *)(indexStart) , path );
				*cost = node->costFromStart;
				hasStartedARun = false;
				return SOLVED;
			}
		}
		
		/*
		if ( node == endPathNode )
		{
			GoalReached( node, startNode, endNode, path );
			*cost = node->costFromStart;
			hasStartedARun = false;
			return SOLVED;
		}*/
		{
			// We have not reached the goal - add the neighbors.
			#ifdef USE_ASSERTIONS
			// No node can be at the edge !
			assert(xstart != 0 && xstart != mapSizeX);
			assert(ystart != 0 && ystart != mapSizeY);
			#endif

			float nodeCostFromStart = node->costFromStart;
			for( int i=0; i < 8; ++i )
			{
				int indexEnd = offsets[i] + indexStart;
				
				if ( !canMoveArray[indexEnd] ) {
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
					open.Update( directNode );
				}
				else
				{
					directNode->inClosed = 0;
					open.Push( directNode );
				}
			}
		}
		node->inClosed = 1;			
	}
	hasStartedARun = false;
	return NO_SOLUTION;		
}	




