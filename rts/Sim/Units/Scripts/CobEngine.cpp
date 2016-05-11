/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"
#include "CobThread.h"
#include "CobInstance.h"
#include "CobFile.h"
#include "System/FileSystem/FileHandler.h"
#include "System/TimeProfiler.h"


CCobEngine* cobEngine = nullptr;
CCobFileHandler* cobFileHandler = nullptr;


/******************************************************************************/
/******************************************************************************/

CR_BIND(CCobEngine, )

CR_REG_METADATA(CCobEngine, (
	CR_MEMBER(currentTime),
	CR_MEMBER(running),
	CR_MEMBER(sleeping),

	//always null/empty when saving
	CR_IGNORED(wantToRun),
	CR_IGNORED(curThread)
))


CCobEngine::CCobEngine()
	: curThread(nullptr)
	, currentTime(0)

{ }


CCobEngine::~CCobEngine()
{
	//Should delete all things that the scheduler knows
	do {
		while (!running.empty()) {
			CCobThread* tmp = running.back();
			running.pop_back();
			delete tmp;
		}
		while (!wantToRun.empty()) {
			CCobThread* tmp = wantToRun.back();
			wantToRun.pop_back();
			delete tmp;
		}
		while (!sleeping.empty()) {
			CCobThread* tmp = sleeping.top();
			sleeping.pop();
			delete tmp;
		}
		// callbacks may add new threads
	} while (!running.empty() || !wantToRun.empty() || !sleeping.empty());
}


CCobFileHandler::~CCobFileHandler()
{
	//Free all cobfiles
	for (std::map<std::string, CCobFile *>::iterator i = cobFiles.begin(); i != cobFiles.end(); ++i) {
		delete i->second;
	}
}


//A thread wants to continue running at a later time, and adds itself to the scheduler
void CCobEngine::AddThread(CCobThread *thread)
{
	switch (thread->state) {
		case CCobThread::Run:
			wantToRun.push_back(thread);
			break;
		case CCobThread::Sleep:
			sleeping.push(thread);
			break;
		default:
			LOG_L(L_ERROR, "thread added to scheduler with unknown state (%d)", thread->state);
			break;
	}
}


void CCobEngine::TickThread(CCobThread* thread)
{
	curThread = thread; // for error messages originating in CUnitScript

	if (!thread->Tick())
		delete thread;

	curThread = NULL;
}


void CCobEngine::Tick(int deltaTime)
{
	SCOPED_TIMER("CobEngine::Tick");

	currentTime += deltaTime;

	// Advance all running threads
	for (CCobThread* t: running) {
		//LOG_L(L_DEBUG, "Now 1running %d: %s", currentTime, (*i)->GetName().c_str());
		TickThread(t);
	}

	// A thread can never go from running->running, so clear the list
	// note: if preemption was to be added, this would no longer hold
	// however, ta scripts can not run preemptively anyway since there
	// isn't any synchronization methods available
	running.clear();

	// The threads that just ran may have added new threads that should run next tick
	std::swap(running, wantToRun);

	//Check on the sleeping threads
	if (!sleeping.empty()) {
		CCobThread* cur = sleeping.top();

		while ((cur != NULL) && (cur->GetWakeTime() < currentTime)) {
			// Start with removing the executing thread from the queue
			sleeping.pop();

			//Run forward again. This can quite possibly readd the thread to the sleeping array again
			//But it will not interfere since it is guaranteed to sleep > 0 ms
			//LOG_L(L_DEBUG, "Now 2running %d: %s", currentTime, cur->GetName().c_str());
			if (cur->state == CCobThread::Sleep) {
				cur->state = CCobThread::Run;
				TickThread(cur);
			} else if (cur->state == CCobThread::Dead) {
				delete cur;
			} else {
				LOG_L(L_ERROR, "Sleeping thread strange state %d", cur->state);
			}

			if (!sleeping.empty())
				cur = sleeping.top();
			else
				cur = NULL;
		}
	}
}


void CCobEngine::ShowScriptError(const string& msg)
{
	if (curThread)
		curThread->ShowError(msg);
	else
		LOG_L(L_ERROR, "%s outside script execution", msg.c_str());
}


/******************************************************************************/
/******************************************************************************/

CCobFile* CCobFileHandler::GetCobFile(const string& name)
{
	//Already known?
	map<string, CCobFile *>::iterator i;
	if ((i = cobFiles.find(name)) != cobFiles.end()) {
		return i->second;
	}

	CFileHandler f(name);
	if (!f.FileExists()) {
		return NULL;
	}
	CCobFile *cf = new CCobFile(f, name);

	cobFiles[name] = cf;
	return cf;
}


CCobFile* CCobFileHandler::ReloadCobFile(const string& name)
{
	map<string, CCobFile *>::iterator it = cobFiles.find(name);
	if (it == cobFiles.end()) {
		return GetCobFile(name);
	}

	delete it->second;
	cobFiles.erase(it);

	return GetCobFile(name);
}


const CCobFile* CCobFileHandler::GetScriptAddr(const string& name) const
{
	map<string, CCobFile *>::const_iterator it = cobFiles.find(name);
	if (it != cobFiles.end()) {
		return it->second;
	}
	return NULL;
}


/******************************************************************************/
/******************************************************************************/
