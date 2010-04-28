/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __COMMAND_QUEUE_H__
#define __COMMAND_QUEUE_H__

#ifndef BUILDING_AI
#include "Rendering/GL/myGL.h"
#else
#define GML_STDMUTEX_LOCK(x)
#endif
#include <deque>
#include "Command.h"

// A wrapper class for  std::deque<Command>  to keep track of commands


class CCommandQueue {

	friend class CCommandAI;
	friend class CFactoryCAI;
	friend class CAIAICallback; // the C++ AI interface wrapper

	// see CommandAI.cpp for further creg stuff for this class
	CR_DECLARE(CCommandQueue);

	public:
		enum QueueType {
			CommandQueueType,
			NewUnitQueueType,
			BuildQueueType
		};

		inline QueueType GetType() const { return queueType; }

	public:
		/// limit to a float's integer range
		static const int maxTagValue = (1 << 24); // 16777216

		typedef std::deque<Command> basis;

		typedef basis::size_type              size_type;
		typedef basis::iterator               iterator;
		typedef basis::const_iterator         const_iterator;
		typedef basis::reverse_iterator       reverse_iterator;
		typedef basis::const_reverse_iterator const_reverse_iterator;

		inline bool empty() const { return queue.empty(); }

		inline size_type size() const { return queue.size(); }

		inline void push_back(const Command& cmd);
		inline void push_front(const Command& cmd);

		inline iterator insert(iterator pos, const Command& cmd);

		inline void pop_back()
		{ 
			GML_STDMUTEX_LOCK(cai); // pop_back

			queue.pop_back(); 
		}
		inline void pop_front()
		{ 
			GML_STDMUTEX_LOCK(cai); // pop_front

			queue.pop_front(); 
		}

		inline iterator erase(iterator pos)
		{
			GML_STDMUTEX_LOCK(cai); // erase

			return queue.erase(pos);
		}
		inline iterator erase(iterator first, iterator last)
		{
			GML_STDMUTEX_LOCK(cai); // erase

			return queue.erase(first, last);
		}
		inline void clear()
		{ 
			GML_STDMUTEX_LOCK(cai); // clear 

			queue.clear(); 
		}

		inline iterator       end()         { return queue.end(); }
		inline const_iterator end()   const { return queue.end(); }
		inline iterator       begin()       { return queue.begin(); }
		inline const_iterator begin() const { return queue.begin(); }

		inline reverse_iterator       rend()         { return queue.rend(); }
		inline const_reverse_iterator rend()   const { return queue.rend(); }
		inline reverse_iterator       rbegin()       { return queue.rbegin(); }
		inline const_reverse_iterator rbegin() const { return queue.rbegin(); }

		inline       Command& back()        { return queue.back(); }
		inline const Command& back()  const { return queue.back(); }
		inline       Command& front()       { return queue.front(); }
		inline const Command& front() const { return queue.front(); }

		inline       Command& at(size_type i)       { return queue.at(i); }
		inline const Command& at(size_type i) const { return queue.at(i); }

		inline       Command& operator[](size_type i)       { return queue[i]; }
		inline const Command& operator[](size_type i) const { return queue[i]; }

	private:
		CCommandQueue() : queueType(CommandQueueType), tagCounter(0) {};
		CCommandQueue(const CCommandQueue&);
		CCommandQueue& operator=(const CCommandQueue&);

	private:
		inline int GetNextTag();
		inline void SetQueueType(QueueType type) { queueType = type; }

	private:
		std::deque<Command> queue;
		QueueType queueType;
		int tagCounter;
};


inline int CCommandQueue::GetNextTag()
{
	tagCounter++;
	if (tagCounter >= maxTagValue) {
		tagCounter = 1;
	}
	return tagCounter;
}


inline void CCommandQueue::push_back(const Command& cmd)
{
	GML_STDMUTEX_LOCK(cai); // push_back

	queue.push_back(cmd);
	queue.back().tag = GetNextTag();
}


inline void CCommandQueue::push_front(const Command& cmd)
{
	GML_STDMUTEX_LOCK(cai); // push_front

	queue.push_front(cmd);
	queue.front().tag = GetNextTag();
}


inline CCommandQueue::iterator CCommandQueue::insert(iterator pos,
                                                     const Command& cmd)
{
	Command tmpCmd = cmd;
	tmpCmd.tag = GetNextTag();
	GML_STDMUTEX_LOCK(cai); // insert
	return queue.insert(pos, tmpCmd);
}


#endif // __COMMAND_QUEUE_H__
