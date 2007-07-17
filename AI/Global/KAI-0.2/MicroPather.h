/*
 * Copyright (c) 2000-2003 Lee Thomason (www.grinninglizard.com)
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
 */


#include "GlobalAI.h"

#ifndef GRINNINGLIZARD_MICROPATHER_INCLUDED
#define GRINNINGLIZARD_MICROPATHER_INCLUDED

/* @mainpage MicroPather
 *
 * MicroPather is a path finder and A* solver (astar or a-star) written in platform independent
 * C++ that can be easily integrated into existing code. MicroPather focuses on being a path
 * finding engine for video games but is a generic A* solver. MicroPather is open source, with
 * a license suitable for open source or commercial use.
 *
 * An overview of using MicroPather is in the <A HREF="../readme.htm">readme</A> or
 * on the Grinning Lizard website: http://grinninglizard.com/micropather/
 */


namespace NSMicroPather {
	const float FLT_BIG = FLT_MAX / 2.0f;

	class PathNode {
		// trashy trick to get rid of compiler warning because this class has a private constructor and destructor
		// (it can never be "new" or created on the stack, only by special allocators)
		friend class none;

		public:
			void Init(unsigned _frame, float _costFromStart, PathNode* _parent) {
				costFromStart = _costFromStart;
				totalCost = _costFromStart;
				parent = _parent;
				frame = _frame;
				inOpen = 0;
				inClosed = 0;
				isEndNode = 0;
			}

			inline void Reuse(unsigned _frame) {
				costFromStart = FLT_BIG;
				parent = 0;
				frame = _frame;
				inOpen = 0;
				inClosed = 0;
			}

			int myIndex;			// the index in the heap array
			float costFromStart;	// exact
			float totalCost;		// could be a function, but save some math.
			PathNode* parent;		// the parent is used to reconstruct the path

			unsigned inOpen:1;
			unsigned inClosed:1;
			unsigned isEndNode:1;	// this must be cleared by the call that sets it
			unsigned frame:16;		// this might be set to more that 16

		private:
			PathNode();
			~PathNode();
	};

	// create a MicroPather object to solve for a best path
	class MicroPather {
		friend class NSMicroPather::PathNode;

		public:
			enum {
				SOLVED,
				NO_SOLUTION,
				START_END_SAME,
			};

		/*
		 * construct the pather (@param allocate is the block size that the
		 * node cache is allocated from, must be the same as the total states)
		 */
		MicroPather(AIClasses* ai, unsigned allocate);
		AIClasses* ai;
		~MicroPather();

		/*
		 * Solve for the path from start to end.
		 *
		 * @param startState	Input, the starting state for the path.
		 * @param endState		Input, the ending state for the path.
		 * @param path			Output, a vector of states that define the path. Empty if not found.
		 * @param totalCost		Output, the cost of the path, if found.
		 * @return				Success or failure, expressed as SOLVED, NO_SOLUTION, or START_END_SAME.
		 */
		int Solve(void* startState, void* endState, std::vector<void*>* path, float* totalCost);

		// return the "checksum" of the last path returned by Solve(). Useful for debugging, and a quick way to see if 2 paths are the same.
		unsigned Checksum() {
			return checksum;
		}

		// Tounesol's stuff
		void SetMapData(unsigned int* canMoveIntMaskArray, float* costArray, int mapSizeX, int mapSizeY, unsigned int canMoveBitMask);
		int Solve(unsigned startNode, unsigned endNode, vector<unsigned>& path, float& cost);

		// cutoff is the maximum cost of a point that its posible to path too, cutoff == 0 disables it
		int FindBestPathToAnyGivenPoint( unsigned startNode, vector<unsigned>& endNodes, vector<unsigned>& path, float& cost, float cutoff);

		// each node in endNodes is separate, do not use this for radius search
		int FindBestPathToPrioritySet(unsigned startNode, vector<unsigned>& endNodes, vector<unsigned>& path, unsigned& priorityIndexFound, float& cost);

		private:
			// Tounesol's stuff

			// should not be called unless there is danger for frame overflow (16bit atm)
			void Reset();
			// used with the bit mask for testing if its posible to move to a spot.
			unsigned int* canMoveIntMaskArray;
			unsigned int canMoveBitMask;
			float* costArray;
			unsigned mapSizeX;
			unsigned mapSizeY;
			int offsets[8];
			int xEndNode, yEndNode;
			bool hasStartedARun;

			PathNode* FindBestPathStandard();
			PathNode* FindBestPathUndirected();
			PathNode* FindBestPathUndirectedCutoff(float cutoff);

			void GoalReached(PathNode* node, unsigned start, unsigned end, std::vector<unsigned> &path);
			void FixStartEndNode( unsigned& startNode, unsigned& endNode );
			float LeastCostEstimateLocal(int nodeStartIndex);
			void FixNode(unsigned& node);
			int SetupStart(unsigned startNode, unsigned endNode, float& cost);
			void SetupEnd(unsigned startNode);

			// allocates the node array, don't call more than once
			PathNode* AllocatePathNode();

			const unsigned ALLOCATE;	// how big a block of pathnodes to allocate at once
			PathNode* pathNodeMem;		// pointer to root of PathNode blocks
			PathNode** heapArrayMem;	// pointer to root of a PathNode pointer array

			unsigned availMem;			// #pathNodes available in the current block
			unsigned frame;				// incremented with every solve, used to determine if cached data needs to be refreshed
			unsigned checksum;			// the checksum of the last successful "Solve".
	};
};


#endif
