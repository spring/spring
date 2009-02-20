/*
 * Copyright (c) 2000-2005 Lee Thomason (www.grinninglizard.com)
 *
 * Grinning Lizard Utilities.
 *
 * This software is provided 'as-is', without any express or implied 
 * warranty. In no event will the authors be held liable for any 
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any 
 * purpose, including commercial applications, and to alter it and 
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must 
 * not claim that you wrote the original software. If you use this 
 * software in a product, an acknowledgment in the product documentation 
 * would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and 
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source 
 * distribution.
 */

/*
 * Updated and changed by Tournesol (on the TA Spring client).
 * May (currenty) only be used to compile KAI (AI for TA Spring).
 * Take care when you use it!
 *
 * This notice may not be removed or altered from any source
 *
 * As of 12 march, 2007, the following applies instead of the above notice:
 * (Tournesol gave permission to change it per e-mail)
 *
 * "All parts of the code in this file that are made by 'Tournesol' are
 * released under GPL licence."
 *
 * --Tobi Vollebregt
 */


#include "MicroPather.h"

#define USE_NODEATINDEX100
// #define USE_ASSERTIONS
// #define DEBUG_PATH
// #define DEBUG_PATH_DEEP
// #define USE_LIST



using namespace NSMicroPather;

class OpenQueueBH {
	public:
	
	OpenQueueBH(AIClasses* ai, PathNode** heapArray): ai(ai), size(0) {
		this -> heapArray = heapArray;
	}

	~OpenQueueBH() {}

	void Push(PathNode* pNode) {
		pNode->inOpen = 1;
		// L("Push: " << size);
		if (size) {
			size++;
			heapArray[size] = pNode;
			pNode->myIndex =  size;
			int i = size;

			while((i > 1) && (heapArray[i >> 1]->totalCost > heapArray[i]->totalCost)) {
				// L("Swap them: " << i);
				PathNode* temp = heapArray[i >> 1];
				heapArray[i >> 1] = heapArray[i];
				heapArray[i] = temp;

				temp -> myIndex = i;
				heapArray[i >> 1] -> myIndex = i >> 1;

				i >>= 1;
			}
		}
		else {
			// L("The tree was empty: " );
			size++;
			heapArray[1] = pNode;
			pNode->myIndex = size;
		}
	}


	void Update(PathNode* pNode) {
		// L("Update: " << size);

		if (size > 1) {
			// heapify now
			int i = pNode->myIndex;

			while(i > 1 && ((heapArray[i >> 1] -> totalCost) > (heapArray[i] -> totalCost))) {
				// swap them
				PathNode* temp = heapArray[i >> 1];
				heapArray[i >> 1] = heapArray[i];
				heapArray[i] = temp;

				temp -> myIndex = i;
				heapArray[i >> 1] -> myIndex =  i >> 1;
				i >>= 1;
			}
		}
	}


	PathNode* Pop() {
		// L("Pop: " << size);

		// get the first one
		PathNode* min = heapArray[1];
		min -> inOpen = 0;
		heapArray[1] = heapArray[size];
		size--;

		if (size == 0) {
			return min;
		}

		heapArray[1] -> myIndex = 1;
		# define Parent(x) (x >> 1)
		# define Left(x)  (x << 1)
		# define Right(x) ((x << 1) + 1)

		// fix the heap
		int index = 1;
		int smalest = 1;

		{
			LOOPING_HEAPIFY:
			index = smalest;
			int left = Left(index);
			int right = Right(index);
			// L("left: " << left << ", index: " << index);

			if (left <= size && ((heapArray[left] -> totalCost) < (heapArray[index] -> totalCost)))
				smalest = left;
			else
				smalest = index;

			if (right <= size && ((heapArray[right] -> totalCost) < (heapArray[smalest] -> totalCost)))
				smalest = right;

			// L("index: " << index << ", smalest: " << smalest);
			if (smalest != index) {
				// swap them
				PathNode* temp = heapArray[index];
				heapArray[index] = heapArray[smalest];
				heapArray[smalest] = temp;

				temp -> myIndex = smalest;
				heapArray[index] -> myIndex = index;

				goto LOOPING_HEAPIFY;
			}
		}

		return min;
	}

	bool Empty() {
		return (size == 0);
	}
	
	private:
		PathNode** heapArray;
		AIClasses* ai;
		int size;
};


// logger hack
MicroPather::MicroPather(Graph* _graph, AIClasses* ai, unsigned allocate):
	ALLOCATE(allocate), BLOCKSIZE(allocate - 1), graph(_graph), pathNodeMem(0), availMem(0), pathNodeCount(0), frame(0), checksum(0) {

	this -> ai = ai;
	AllocatePathNode();
	hasStartedARun = false;
}

MicroPather::~MicroPather() {
	free(pathNodeMemForFree);
	free(heapArrayMem);
}


// make sure that costArray doesn't contain values below 1.0 (for speed), and below 0.0 (for eternal loop)
void MicroPather::SetMapData(bool *canMoveArray, float *costArray, int mapSizeX, int mapSizeY) {
	this -> canMoveArray = canMoveArray;
	this -> costArray = costArray;
	this -> mapSizeX = mapSizeX;
	this -> mapSizeY = mapSizeY;

	assert(mapSizeX >= 0);
	assert(mapSizeY >= 0);

	if (mapSizeY * mapSizeX  > (int) ALLOCATE) {
		// L("Error: 'mapSizeY * mapSizeX  > ALLOCATE' in pather");
		// Stop running:
		assert(!(mapSizeY * mapSizeX  > (int)ALLOCATE));
	}

	// L("pather: mapSizeX: " << mapSizeX << ", mapSizeY: " << mapSizeY << ", ALLOCATE: " << ALLOCATE);

	// Tournesol: make a fixed offset array
	// ***
	// *X*
	// ***

	// smart ordering (do not change)
	offsets[0] = -1;
	offsets[1] = 1;
	offsets[2] = + mapSizeX;
	offsets[3] = - mapSizeX;
	offsets[4] = - mapSizeX - 1;
	offsets[5] = - mapSizeX + 1;
	offsets[6] = + mapSizeX - 1;
	offsets[7] = + mapSizeX + 1;
}


void MicroPather::Reset() {
	// L("Reseting pather, frame is: " << frame);
	for (unsigned i = 0; i < ALLOCATE; i++) {
		PathNode* directNode = &pathNodeMem[i];
		directNode -> Reuse(0);
	}

	frame = 1;
}


// must only be called once...
PathNode* MicroPather::AllocatePathNode() {
	PathNode* result = 0x0;

	if (availMem == 0) {
		PathNode* newBlock = (PathNode*) malloc(sizeof(PathNode) * ALLOCATE);
		// L("pathNodeMemForFree: " << ((unsigned) newBlock));
		pathNodeMemForFree = newBlock;
		pathNodeMem = newBlock;
		// L(" sizeof(PathNode): " << sizeof(PathNode));
		availMem = BLOCKSIZE;

		// make all the nodes in one go (step one)
		for (unsigned i = 0; i < ALLOCATE; i++) {
			result = &pathNodeMem[i];
			++pathNodeCount;
			result -> Init(0, FLT_BIG, 0);
			result -> isEndNode = 0;
		}

		result = newBlock;
		heapArrayMem = (PathNode**) malloc(sizeof(PathNode*) * ALLOCATE);
	}
	else {
		// this is bad....
		// AllocatePathNodeCalledTwice
		assert(false);
	}

	return result;
}

void MicroPather::GoalReached(PathNode* node, void* start, void* end, vector<void*>* path) {
	path -> clear();

	// we have reached the goal, how long is the path?
	// (used to allocate the vector which is returned)
	int count = 1;
	PathNode* it = node;

	while (it -> parent) {
		++count;
		it = it -> parent;
	}

	// now that the path has a known length, allocate
	// and fill the vector that will be returned
	if (count < 3) {
		// Handle the short, special case.
		path -> resize(2);
		(*path)[0] = start;
		(*path)[1] = end;
	}
	else {
		path -> resize(count);

		(*path)[0] = start;
		(*path)[count - 1] = end;

		count -= 2;
		it = node -> parent;

		while (it -> parent) {
			(*path)[count] = (void*) ((((size_t) it) - ((size_t) pathNodeMem)) / sizeof(PathNode));
			it = it -> parent;
			--count;
		}
	}

	#ifdef DEBUG_PATH
	printf("Path: ");
	printf("Cost = %.1f Checksum %d\n", node -> costFromStart, checksum);
	#endif
}

float MicroPather::LeastCostEstimateLocal(int nodeStartIndex) {
	int yStart = nodeStartIndex / mapSizeX;
	int xStart = nodeStartIndex - yStart * mapSizeX;

	int dx = abs(xStart - xEndNode);
	int dy = abs(yStart - yEndNode);
	int strait = abs(dx - dy);

	return (strait + 1.41f * std::min(dx, dy));
}

void MicroPather::FixStartEndNode(void** startNode, void** endNode) {
	size_t index = (size_t) *startNode;
	int y = index / mapSizeX;
	int x = index - y * mapSizeX;
	
	// no node can be at the edge!
	if (x == 0)
		x = 1;
	else if (x == mapSizeX)
		x = mapSizeX - 1;

	if (y == 0)
		y = 1;
	else if (y == mapSizeY)
		y = mapSizeY - 1;

	*startNode = (void*) (y * mapSizeX + x);
	index = (size_t) *endNode;
	y = index / mapSizeX;
	x = index - y * mapSizeX;

	// no node can be at the edge!
	if (x == 0)
		x = 1;
	else if (x == mapSizeX)
		x = mapSizeX - 1;

	if (y == 0)
		y = 1;
	else if (y == mapSizeY)
		y = mapSizeY - 1;

	xEndNode = x;
	yEndNode = y;
	*endNode = (void*) (y * mapSizeX + x);
}

void MicroPather::FixNode( void** Node) {
	size_t index = (size_t) *Node;
	int y = index / mapSizeX;
	int x = index - y * mapSizeX;

	assert(index >= 0);
	assert(index <= (unsigned int) ((unsigned int)mapSizeX * mapSizeY));

	// no node can be at the edge!
	if (x == 0)
		x = 1;
	else if (x == mapSizeX)
		x = mapSizeX - 1;

	if (y == 0)
		y = 1;
	else if (y == mapSizeY)
		y = mapSizeY - 1;

	*Node = (void*) (y * mapSizeX + x);
}



int MicroPather::Solve(void* startNode, void* endNode, vector<void*>* path, float* cost) {
	assert(!hasStartedARun);
	hasStartedARun = true;
	*cost = 0.0f;

	if (startNode == endNode) {
		hasStartedARun = false;
		return START_END_SAME;
	}

	{
		FixStartEndNode(&startNode, &endNode);

		if (!canMoveArray[(size_t) startNode]) {
			// L("Pather: trying to move from a blocked start pos");
		}
		if (!canMoveArray[(size_t) endNode]) {
			// can't move into the endNode: just fail fast
			hasStartedARun = false;
			return NO_SOLUTION;
		}
	}

	++frame;

	if (frame > 65534) {
		// L("frame > 65534, pather reset needed");
		Reset();
	}

	// Make the priority queue
	OpenQueueBH open(ai, heapArrayMem);

	{
		PathNode* tempStartNode = &pathNodeMem[(size_t) startNode];
		float estToGoal = LeastCostEstimateLocal( (size_t) startNode);
		tempStartNode -> Reuse(frame);
		tempStartNode -> costFromStart = 0;
		tempStartNode -> totalCost = estToGoal;
		open.Push(tempStartNode);
	}

	PathNode* endPathNode = &(pathNodeMem[(size_t) endNode]);

	while (!open.Empty()) {
		PathNode* node = open.Pop();

		if (node == endPathNode) {
			GoalReached(node, startNode, endNode, path);
			*cost = node -> costFromStart;
			hasStartedARun = false;
			return SOLVED;
		}
		else {
			// we have not reached the goal, add the neighbors (emulate GetNodeNeighbors)
			int indexStart = (((size_t) node) - ((size_t) pathNodeMem)) / sizeof(PathNode);

			#ifdef USE_ASSERTIONS
			int ystart = indexStart / mapSizeX;
			int xstart = indexStart - ystart * mapSizeX;
			
			// no node can be at the edge!
			assert(xstart != 0 && xstart != mapSizeX);
			assert(ystart != 0 && ystart != mapSizeY);
			#endif

			float nodeCostFromStart = node -> costFromStart;

			for (int i = 0; i < 8; ++i) {
				int indexEnd = offsets[i] + indexStart;
				
				if (!canMoveArray[indexEnd]) {
					continue;
				}

				PathNode* directNode = &pathNodeMem[indexEnd];

				if (directNode -> frame != frame)
					directNode -> Reuse(frame);

				#ifdef USE_ASSERTIONS
				int yend = indexEnd / mapSizeX;
				int xend = indexEnd - yend * mapSizeX;

				// we can move to that spot
				assert(canMoveArray[yend * mapSizeX + xend]);

				// no node can be at the edge!
				assert(xend != 0 && xend != mapSizeX);
				assert(yend != 0 && yend != mapSizeY);
				#endif

				float newCost = nodeCostFromStart;

				if (i > 3) {
					newCost += costArray[indexEnd] * 1.41f;
				}
				else
					newCost += costArray[indexEnd];

				if (directNode -> costFromStart <= newCost) {
					// do nothing, this path is not better than existing one
					continue;
				}

				// it's better, update its data
				directNode -> parent = node;
				directNode -> costFromStart = newCost;
				directNode -> totalCost = newCost + LeastCostEstimateLocal(indexEnd);
				#ifdef USE_ASSERTIONS
				assert(indexEnd == ((((unsigned) directNode) - ((unsigned) pathNodeMem)) / sizeof(PathNode)));
				#endif

				if (directNode -> inOpen) {
					open.Update(directNode);
				}
				else {
					directNode -> inClosed = 0;
					open.Push(directNode);
				}
			}
		}

		node -> inClosed = 1;
	}

	hasStartedARun = false;
	return NO_SOLUTION;
}


int MicroPather::FindBestPathToAnyGivenPoint(void* startNode, vector<void*> endNodes, vector<void*>* path, float* cost) {
	assert(!hasStartedARun);
	hasStartedARun = true;
	*cost = 0.0f;
	
	for (unsigned i = 0; i < ALLOCATE; i++) {
		PathNode* theNode = &pathNodeMem[i];

		if (theNode -> isEndNode) {
			// L("i: " << i << ", " << theNode->isEndNode);
			theNode -> isEndNode = 0;
			assert(theNode -> isEndNode == 0);
		}
	}

	if (endNodes.size() <= 0) {
		// just fail fast
		hasStartedARun = false;
		return NO_SOLUTION;
	}

	{
		void* endNode = endNodes[0];
		FixStartEndNode(&startNode, &endNode);

		if (!canMoveArray[(size_t)startNode]) {
			// L("Pather: trying to move from a blocked start pos");
		}
	}

	++frame;

	if (frame > 65534) {
		// L("frame > 65534, pather reset needed");
		Reset();
	}

	// Make the priority queue
	OpenQueueBH open(ai, heapArrayMem);

	{
		PathNode* tempStartNode = &pathNodeMem[(size_t) startNode];
		float estToGoal = LeastCostEstimateLocal( (size_t) startNode);
		tempStartNode -> Reuse(frame);
		tempStartNode -> costFromStart = 0;
		tempStartNode -> totalCost = estToGoal;
		open.Push( tempStartNode );
	}

	// mark the endNodes
	for (unsigned i = 0; i < endNodes.size(); i++) {
		FixNode(&endNodes[i]);
		PathNode* tempEndNode = &pathNodeMem[(size_t) endNodes[i]];
		tempEndNode -> isEndNode = 1;
	}

	while (!open.Empty()) {
		PathNode* node = open.Pop();

		if (node -> isEndNode) {
			void* theEndNode = (void*) ((((size_t) node) - ((size_t) pathNodeMem)) / sizeof(PathNode));
			GoalReached(node, startNode, theEndNode, path);
			*cost = node -> costFromStart;
			hasStartedARun = false;

			// unmark the endNodes
			for (unsigned i = 0; i < endNodes.size(); i++) {
				PathNode* tempEndNode = &pathNodeMem[(size_t) endNodes[i]];
				tempEndNode -> isEndNode = 0;
			}

			return SOLVED;
		}
		else {
			// we have not reached the goal, add the neighbors (emulate GetNodeNeighbors)
			int indexStart = (((size_t) node) - ((size_t) pathNodeMem)) / sizeof(PathNode);

			#ifdef USE_ASSERTIONS
			int ystart = indexStart / mapSizeX;
			int xstart = indexStart - ystart * mapSizeX;

			// no node can be at the edge!
			assert(xstart != 0 && xstart != mapSizeX);
			assert(ystart != 0 && ystart != mapSizeY);
			#endif

			float nodeCostFromStart = node -> costFromStart;

			for (int i = 0; i < 8; ++i) {
				int indexEnd = offsets[i] + indexStart;

				if (!canMoveArray[indexEnd]) {
					continue;
				}

				PathNode* directNode = &pathNodeMem[indexEnd];

				if (directNode -> frame != frame)
					directNode -> Reuse(frame);

				#ifdef USE_ASSERTIONS
				int yend = indexEnd / mapSizeX;
				int xend = indexEnd - yend * mapSizeX;

				// we can move to that spot
				assert(canMoveArray[yend * mapSizeX + xend]);

				// no node can be at the edge!
				assert(xend != 0 && xend != mapSizeX);
				assert(yend != 0 && yend != mapSizeY);
				#endif

				float newCost = nodeCostFromStart;

				if (i > 3) {
					newCost += costArray[indexEnd] * 1.41f;
				}
				else
					newCost += costArray[indexEnd];

				if (directNode -> costFromStart <= newCost) {
					// do nothing, this path is not better than existing one
					continue;
				}

				// it's better, update its data
				directNode -> parent = node;
				directNode -> costFromStart = newCost;
				directNode -> totalCost = newCost + LeastCostEstimateLocal(indexEnd);

				#ifdef USE_ASSERTIONS
				assert(indexEnd == ((((unsigned) directNode) - ((unsigned) pathNodeMem)) / sizeof(PathNode)));
				#endif

				if (directNode -> inOpen) {
					open.Update(directNode);
				}
				else {
					directNode -> inClosed = 0;
					open.Push(directNode);
				}
			}
		}

		node -> inClosed = 1;
	}

	// unmark the endNodes
	for (unsigned i = 0; i < endNodes.size(); i++) {
		PathNode* tempEndNode = &pathNodeMem[(size_t)endNodes[i]];
		tempEndNode -> isEndNode = 0;
	}

	hasStartedARun = false;
	return NO_SOLUTION;
}



int MicroPather::FindBestPathToPointOnRadius(void* startNode, void* endNode, vector<void*>* path, float* cost, int radius) {
	assert(!hasStartedARun);
	hasStartedARun = true;
	*cost = 0.0f;

	if (radius <= 0) {
		// just fail fast
		hasStartedARun = false;
		return NO_SOLUTION;
	}

	{
		FixStartEndNode(&startNode, &endNode);

		if (!canMoveArray[(size_t)startNode]) {
			// L("Pather: trying to move from a blocked start pos");
		}
	}

	++frame;
	if (frame > 65534) {
		// L("frame > 65534, pather reset needed");
		Reset();
	}

	// make the priority queue
	OpenQueueBH open(ai, heapArrayMem);

	{
		PathNode* tempStartNode = &pathNodeMem[(size_t) startNode];
		float estToGoal = LeastCostEstimateLocal( (size_t) startNode);
		tempStartNode -> Reuse(frame);
		tempStartNode -> costFromStart = 0;
		tempStartNode -> totalCost = estToGoal;
		open.Push(tempStartNode);
	}
	
	// make the radius
	size_t indexEnd = (size_t) endNode;
	int y = indexEnd / mapSizeX;
	int x = indexEnd - y * mapSizeX;
	int* xend = new int[2 * radius + 1];

	for (int a = 0; a < (2 * radius + 1); a++) {
		float z = a - radius;
		float floatsqrradius = radius * radius;
		xend[a] = int(sqrtf(floatsqrradius - z * z));

		// L("xend[a]: " << xend[a]);
		// L("xStart: " << xStart << ", xEnd: " << xEnd);
	}

	// L("yEndNode: " << yEndNode << ", xEndNode: " << xEndNode);

	while (!open.Empty()) {
		PathNode* node = open.Pop();

		int indexStart = (((size_t) node) - ((size_t) pathNodeMem)) / sizeof(PathNode);
		int ystart = indexStart / mapSizeX;
		int xstart = indexStart - ystart * mapSizeX;
		// L("counter: " << counter << ", ystart: " << ystart << ", xstart: " << xstart);

		// do a box test (slow/test, note that a <= x <= b is the same as x - a <= b - a)
		if ((y - radius <= ystart && ystart <= y + radius) && (x - radius <= xstart && xstart <= x + radius)) {
			// we are in range (x and y direction), find the relative pos from endNode
			int relativeY = ystart - (yEndNode - radius);
			int relativeX = abs(xstart - xEndNode);
			// L("relativeY: " << relativeY << ", relativeX: " << relativeX);

			if (relativeX <= xend[relativeY]) {
				// L("Its a hit: " << counter);

				GoalReached(node, startNode, (void*) (indexStart), path);
				*cost = node -> costFromStart;
				hasStartedARun = false;
				return SOLVED;
			}
		}

		{
			// we have not reached the goal, add the neighbors.
			#ifdef USE_ASSERTIONS
			// no node can be at the edge!
			assert(xstart != 0 && xstart != mapSizeX);
			assert(ystart != 0 && ystart != mapSizeY);
			#endif

			float nodeCostFromStart = node -> costFromStart;
			for (int i = 0; i < 8; ++i) {
				int indexEnd = offsets[i] + indexStart;

				if (!canMoveArray[indexEnd]) {
					continue;
				}

				PathNode* directNode = &pathNodeMem[indexEnd];

				if (directNode->frame != frame)
					directNode -> Reuse(frame);

				#ifdef USE_ASSERTIONS
				int yend = indexEnd / mapSizeX;
				int xend = indexEnd - yend * mapSizeX;

				// we can move to that spot
				assert(canMoveArray[yend*mapSizeX + xend]);

				// no node can be at the edge!
				assert(xend != 0 && xend != mapSizeX);
				assert(yend != 0 && yend != mapSizeY);
				#endif

				float newCost = nodeCostFromStart;

				if (i > 3) {
					newCost += costArray[indexEnd] * 1.41f;
				}
				else
					newCost += costArray[indexEnd];

				if (directNode -> costFromStart <= newCost) {
					// do nothing, this path is not better than existing one
					continue;
				}

				// it's better, update its data
				directNode -> parent = node;
				directNode -> costFromStart = newCost;
				directNode -> totalCost = newCost + LeastCostEstimateLocal(indexEnd);

				#ifdef USE_ASSERTIONS
				assert(indexEnd == ((((size_t) directNode) - ((size_t) pathNodeMem)) / sizeof(PathNode)));
				#endif

				if (directNode -> inOpen) {
					open.Update(directNode);
				}
				else {
					directNode -> inClosed = 0;
					open.Push(directNode);
				}
			}
		}

		node -> inClosed = 1;
	}

	hasStartedARun = false;
	return NO_SOLUTION;
}
