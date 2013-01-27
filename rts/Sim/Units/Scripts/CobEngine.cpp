/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CobEngine.h"
#include "CobThread.h"
#include "CobInstance.h"
#include "CobFile.h"
#include "UnitScriptLog.h"
#include "System/FileSystem/FileHandler.h"

#ifndef _CONSOLE
#include "System/TimeProfiler.h"
#endif
#ifdef _CONSOLE
#define START_TIME_PROFILE(a) {}
#define END_TIME_PROFILE(a) {}
#define SCOPED_TIMER(a) {}
#endif


CCobEngine GCobEngine;
CCobFileHandler GCobFileHandler;
int GCurrentTime;


/******************************************************************************/
/******************************************************************************/


CCobEngine::CCobEngine()
	: curThread(NULL)
{
	GCurrentTime = 0;
}


CCobEngine::~CCobEngine()
{
	//Should delete all things that the scheduler knows
	while (!running.empty()) {
		CCobThread *tmp = running.front();
		tmp->SetCallback(NULL, NULL, NULL);
		running.pop_front();
		delete tmp;
	}
	while(!wantToRun.empty()) {
		CCobThread *tmp = wantToRun.front();
		tmp->SetCallback(NULL, NULL, NULL);
		wantToRun.pop_front();
		delete tmp;
	}
	while (!sleeping.empty()) {
		CCobThread *tmp = sleeping.top();
		tmp->SetCallback(NULL, NULL, NULL);
		sleeping.pop();
		delete tmp;
	}

	assert(running.empty() && wantToRun.empty() && sleeping.empty());
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
			wantToRun.push_front(thread);
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

	GCurrentTime += deltaTime;

	LOG_L(L_DEBUG, "----");

	// Advance all running threads
	for (std::list<CCobThread*>::iterator i = running.begin(); i != running.end(); ++i) {
		//LOG_L(L_DEBUG, "Now 1running %d: %s", GCurrentTime, (*i)->GetName().c_str());
#ifdef _CONSOLE
		printf("----\n");
#endif
		TickThread(*i);
	}

	// A thread can never go from running->running, so clear the list
	// note: if preemption was to be added, this would no longer hold
	// however, ta scripts can not run preemptively anyway since there
	// isn't any synchronization methods available
	running.clear();

	// The threads that just ran may have added new threads that should run next tick
	for (std::list<CCobThread *>::iterator i = wantToRun.begin(); i != wantToRun.end(); ++i) {
		running.push_front(*i);
	}

	wantToRun.clear();

	//Check on the sleeping threads
	if (!sleeping.empty()) {
		CCobThread* cur = sleeping.top();

		while ((cur != NULL) && (cur->GetWakeTime() < GCurrentTime)) {
			// Start with removing the executing thread from the queue
			sleeping.pop();

			//Run forward again. This can quite possibly readd the thread to the sleeping array again
			//But it will not interfere since it is guaranteed to sleep > 0 ms
			//LOG_L(L_DEBUG, "Now 2running %d: %s", GCurrentTime, cur->GetName().c_str());
#ifdef _CONSOLE
			printf("+++\n");
#endif
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
