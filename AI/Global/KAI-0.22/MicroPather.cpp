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
 * must not be misrepresented as being the original software
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
 */



#include "MicroPather.h"

using namespace NSMicroPather;

class OpenQueueBH {
	public:
		OpenQueueBH() {
			this -> size = 0;
			this -> ai = ai;
			this -> heapArray = 0;
		}
		~OpenQueueBH() {
		}

		void setData(AIClasses* ai, PathNode** heapArray) {
			this -> ai = ai;
			this -> heapArray = heapArray;
		}

		void clear() {
			size = 0;
		}

		void Push(PathNode* pNode) {
			pNode -> inOpen = 1;
			// L("Push: " << size);
			if (size) {
				size++;
				heapArray[size] = pNode;
				pNode -> myIndex =  size;
				int i = size;

				while((i > 1) && (heapArray[i >> 1] -> totalCost > heapArray[i] -> totalCost)) {
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
				// L("The tree was empty: ");
				size++;
				heapArray[1] = pNode;
				pNode -> myIndex = size;
			}
		}

		void Update(PathNode* pNode) {
			// L("Update: " << size);
			if (size > 1) {
				// heapify now
				int i = pNode -> myIndex;

				while (i > 1 && (heapArray[i >> 1] -> totalCost > heapArray[i] -> totalCost)) {
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
			// get the first one
			PathNode* min = heapArray[1];
			min -> inOpen = 0;
			heapArray[1] = heapArray[size];
			size--;

			if (size == 0) {
				return min;
			}

			heapArray[1] -> myIndex = 1;
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

				if (left <= size && heapArray[left] -> totalCost < heapArray[index] -> totalCost)
					smalest = left;
				else
					smalest = index;

				if (right <= size && heapArray[right] -> totalCost < heapArray[smalest] -> totalCost)
					smalest = right;

				if (smalest != index) {
					// Swap them:
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
			return size == 0;
		}

		private:
			PathNode** heapArray;
			AIClasses* ai;
			int size;
};

OpenQueueBH openQueueBH;



MicroPather::MicroPather(AIClasses* ai, unsigned allocate): ALLOCATE(allocate) {
	this -> pathNodeMem = 0;
	this -> availMem = 0;
	this -> frame = 0;
	this -> checksum = 0;
	this -> ai = ai;
	this -> hasStartedARun = false;

	AllocatePathNode();
	openQueueBH.setData(ai, heapArrayMem);
}

// make sure that costArray dont contain values below 1.0 (for speed), and below 0.0 (for eternal loop)
void MicroPather::SetMapData(unsigned int* canMoveIntMaskArray, float* costArray, int mapSizeX, int mapSizeY, unsigned int canMoveBitMask) {
	this -> canMoveIntMaskArray = canMoveIntMaskArray;
	this -> canMoveBitMask = canMoveBitMask;
	this -> costArray = costArray;
	this -> mapSizeX = mapSizeX;
	this -> mapSizeY = mapSizeY;
	
	if (mapSizeY * mapSizeX  > (int)ALLOCATE) {
		L("Error: 'mapSizeY * mapSizeX  > ALLOCATE' in pather");
		// stop running
		assert(!(mapSizeY * mapSizeX  > (int)ALLOCATE));
	}

	//L("pather: mapSizeX: " << mapSizeX << ", mapSizeY: " << mapSizeY << ", ALLOCATE: " << ALLOCATE);
	// Tournesol: make a fixed offset array
	// ***
	// *X*
	// ***

	// Smart ordering (do not change)
	offsets[0] = -1;
	offsets[1] = 1;
	offsets[2] = + mapSizeX;
	offsets[3] = - mapSizeX;
	offsets[4] = - mapSizeX - 1;
	offsets[5] = - mapSizeX + 1;
	offsets[6] = + mapSizeX - 1;
	offsets[7] = + mapSizeX + 1;
}

MicroPather::~MicroPather() {
	free(pathNodeMem);
	free(heapArrayMem);
}

void MicroPather::Reset() {
	L("Reseting pather, frame is: " << frame);

	for (unsigned i = 0; i < ALLOCATE; i++) {
		PathNode* directNode = &pathNodeMem[i];
		directNode -> Reuse(0);
	}

	frame = 1;
}

// must only be called once
PathNode* MicroPather::AllocatePathNode() {
	PathNode* result;

	if (availMem == 0) {
		PathNode* newBlock = (PathNode*) malloc(sizeof(PathNode) * ALLOCATE);

		L("pathNodeMem: " << newBlock);

		pathNodeMem = newBlock;
		L(" sizeof(PathNode): " <<  sizeof(PathNode));
		availMem = ALLOCATE;

		L("availMem == " << (sizeof(PathNode) * ALLOCATE));

		// make all the nodes in one go (step one)
		for (unsigned i = 0; i < ALLOCATE; i++) {
			result = &pathNodeMem[i];
			result -> Init(0, FLT_BIG, 0);
			result -> isEndNode = 0;
		}

		result = newBlock;

		heapArrayMem = (PathNode**) malloc(sizeof(PathNode*) * ALLOCATE);
		L("heapArrayMem: " << heapArrayMem);
	}
	else {
		// this is bad
		bool AllocatePathNodeCalledTwice = false;
		assert(AllocatePathNodeCalledTwice);
	}

	return result;
}

void MicroPather::GoalReached(PathNode* node, unsigned start, unsigned end, vector<unsigned> &path) {
	// we have reached the goal: how long is the path?
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
		// handle the short, special case
		path.resize(2);
		path[0] = start;
		path[1] = end;
	}
	else {
		path.resize(count);

		path[0] = start;
		path[count-1] = end;

		count-=2;
		it = node -> parent;

		while (it -> parent) {
			path[count] = (((unsigned long) it) - ((unsigned long) pathNodeMem)) / sizeof(PathNode);
			it = it -> parent;
			--count;
		}
	}

	#ifdef DEBUG_PATH
	printf("Path: ");
	int counter=0;
	#endif

	#ifdef DEBUG_PATH
	printf("Cost=%.1f Checksum %d\n", node -> costFromStart, checksum);
	#endif
}

float MicroPather::LeastCostEstimateLocal(int nodeStartIndex) {
	int yStart = nodeStartIndex / mapSizeX;
	int xStart = nodeStartIndex - yStart * mapSizeX;

	int dx = abs(xStart - xEndNode);
	int dy = abs(yStart - yEndNode);
	int strait = abs(dx - dy);

	return (strait + 1.41f * min(dx, dy));
}

void MicroPather::FixStartEndNode(unsigned& startNode, unsigned& endNode) {
	FixNode(startNode);
	// no node can be at the edge!
	int y = endNode / mapSizeX;
	int x = endNode - y * mapSizeX;

	// no node can be at the edge!
	if (x == 0)
		x = 1;
	else if (x == (int) mapSizeX)
		x = mapSizeX - 1;

	if (y == 0)
		y = 1;
	else if (y == (int) mapSizeY)
		y = mapSizeY - 1;

	xEndNode = x;
	yEndNode = y;
	endNode = (unsigned) (y * mapSizeX + x);
}

void MicroPather::FixNode(unsigned& node) {
	int y = node / mapSizeX;
	int x = node - y * mapSizeX;

	// no node can be at the edge!
	// (fixing this here might be bad)
	if (x == 0)
		x = 1;
	else if (x == (int) mapSizeX)
		x = mapSizeX - 1;

	if (y == 0)
		y = 1;
	else if (y == (int) mapSizeY)
		y = mapSizeY - 1;

	node = (y * mapSizeX + x);
}

int MicroPather::SetupStart(unsigned startNode, unsigned endNode, float& cost) {
	assert(!hasStartedARun);
	hasStartedARun = true;
	cost = 0.0f;

	if (startNode == endNode) {
		hasStartedARun = false;
		return START_END_SAME;
	}

	FixStartEndNode(startNode, endNode);

	if (!(canMoveIntMaskArray[startNode] & canMoveBitMask)) {
		// can't move from the start, don't do anything
		L("Pather: trying to move from a blocked start pos. Mask: " << canMoveBitMask);
	}

	return SOLVED;
}

void MicroPather::SetupEnd(unsigned startNode) {
	++frame;

	if (frame > 65534) {
		L("frame > 65534, pather reset needed");
		Reset();
	}

	// reset the priority queue
	openQueueBH.clear();
	// add the start node to the queue
	PathNode* tempStartNode = &pathNodeMem[startNode];
	tempStartNode -> Reuse(frame);
	tempStartNode -> costFromStart = 0;
	tempStartNode -> totalCost = LeastCostEstimateLocal(startNode);
	openQueueBH.Push(tempStartNode);
}


int MicroPather::Solve(unsigned startNode, unsigned endNode, vector<unsigned>& path, float& cost) {
	if (SetupStart(startNode, endNode, cost) == START_END_SAME)
		return START_END_SAME;

	if (!(canMoveIntMaskArray[endNode] & canMoveBitMask)) {
		// can't move into the endNode, just fail fast

		L("EndNode blocked, no pathing: " << endNode << ", x: " << xEndNode << ", y: " << yEndNode);
		hasStartedARun = false;
		return NO_SOLUTION;
	}

	SetupEnd(startNode);

	// set the endnode
	PathNode *endPathNode = &(pathNodeMem[endNode]);
	endPathNode -> isEndNode = 1;
	// run a normal solve
	PathNode* foundNode = FindBestPathUndirected();
	endPathNode -> isEndNode = 0;

	if (endPathNode == foundNode) {
		// make the path now:
		GoalReached(endPathNode, startNode, endNode, path);
		cost = endPathNode -> costFromStart;
		hasStartedARun = false;
		return SOLVED;
	}

	hasStartedARun = false;
	return NO_SOLUTION;
}

int MicroPather::FindBestPathToAnyGivenPoint(unsigned startNode, vector<unsigned>& endNodes, vector<unsigned>& path, float& cost, float cutoff) {
	cost = 0.0f;

	if (endNodes.size() <= 0) {
		// just fail fast
		return NO_SOLUTION;
	}

	unsigned endNode = endNodes[0];

	if (SetupStart(startNode, endNode, cost) == START_END_SAME)
		return START_END_SAME;

	SetupEnd(startNode);

	// mark the endNodes
	for (unsigned i = 0; i < endNodes.size(); i++) {
		FixNode(endNodes[i]);
		pathNodeMem[endNodes[i]].isEndNode = 1;
	}

	// run a normal solve
	PathNode* foundNode;
	assert(cutoff >= 0);

	if (cutoff == 0)
		foundNode = FindBestPathUndirected();
	else
		foundNode = FindBestPathUndirectedCutoff(cutoff);

	// unmark the endNodes
	for (unsigned i = 0; i < endNodes.size(); i++) {
		pathNodeMem[endNodes[i]].isEndNode = 0;
	}

	if (foundNode != NULL) {
		// make the path now
		GoalReached(foundNode, startNode, (((unsigned long) foundNode) - ((unsigned long) pathNodeMem)) / sizeof(PathNode), path);
		cost = foundNode -> costFromStart;
		hasStartedARun = false;
		return SOLVED;
	}

	hasStartedARun = false;
	return NO_SOLUTION;
}

int MicroPather::FindBestPathToPrioritySet(unsigned startNode, vector<unsigned>& endNodes, vector<unsigned>& path, unsigned& priorityIndexFound, float& cost) {
	cost = 0.0f;

	if (endNodes.size() <= 0) {
		// just fail fast
		return NO_SOLUTION;
	}

	if (SetupStart(startNode, endNodes[0], cost) == START_END_SAME)
		return START_END_SAME;

	SetupEnd(startNode);
	PathNode* tempEndNode = &pathNodeMem[endNodes[0]];
	tempEndNode -> isEndNode = 1;
	PathNode* foundNode = FindBestPathUndirected();
	tempEndNode -> isEndNode = 0;

	// test each end node
	for (unsigned i = 0; i < endNodes.size(); i++) {
		FixNode(endNodes[i]);
		foundNode = &pathNodeMem[endNodes[i]];

		if (foundNode -> frame == frame) {
			// node was updated this frame, so it was pathed too, so it's reachable from the startnode
			GoalReached(foundNode, startNode, endNodes[i], path);
			cost = foundNode -> costFromStart;
			priorityIndexFound = i;
			hasStartedARun = false;

			return SOLVED;
		}
	}

	hasStartedARun = false;
	return NO_SOLUTION;
}


PathNode* MicroPather::FindBestPathStandard() {
	OpenQueueBH open = openQueueBH;

	while (!open.Empty()) {
		PathNode* node = open.Pop();
		unsigned long indexStart = (((unsigned long) node) - ((unsigned long) pathNodeMem)) / sizeof(PathNode);

		if (node -> isEndNode) {
			return node;
		}

		// We have not reached the goal - add the neighbors.
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

			if ((canMoveIntMaskArray[indexEnd] & canMoveBitMask) != canMoveBitMask) {
				continue;
			}

			PathNode* directNode = &pathNodeMem[indexEnd];

			if (directNode -> frame != frame)
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

		node -> inClosed = 1;
	}

	return NULL;
}


PathNode* MicroPather::FindBestPathUndirected() {
	OpenQueueBH open = openQueueBH;

	while (!open.Empty()) {
		PathNode* node = open.Pop();

		unsigned long indexStart = (((unsigned long) node) - ((unsigned long) pathNodeMem)) / sizeof(PathNode);
		if (node -> isEndNode) {
			return node;
		}

		// We have not reached the goal - add the neighbors.
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

			if ((canMoveIntMaskArray[indexEnd] & canMoveBitMask) != canMoveBitMask) {
				continue;
			}

			PathNode* directNode = &pathNodeMem[indexEnd];

			if (directNode -> frame != frame)
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
			directNode -> totalCost = newCost;
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

		node -> inClosed = 1;
	}

	return NULL;
}


PathNode* MicroPather::FindBestPathUndirectedCutoff(float cutoff) {
	OpenQueueBH open = openQueueBH;

	while (!open.Empty()) {
		PathNode* node = open.Pop();
		unsigned long indexStart = (((unsigned long) node) - ((unsigned long) pathNodeMem)) / sizeof(PathNode);

		if (node -> isEndNode) {
			return node;
		}

		// We have not reached the goal - add the neighbors.
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

			if (((canMoveIntMaskArray[indexEnd] & canMoveBitMask) != canMoveBitMask) || cutoff < costArray[indexEnd]) {
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
			directNode -> totalCost = newCost;
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

		node -> inClosed = 1;
	}

	return NULL;
}
