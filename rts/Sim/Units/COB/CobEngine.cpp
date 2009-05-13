#include "StdAfx.h"
#include "mmgr.h"

#include "CobEngine.h"
#include "CobThread.h"
#include "CobInstance.h"
#include "CobFile.h"
#include "LogOutput.h"
#include "FileSystem/FileHandler.h"
#include "Platform/errorhandler.h"

#ifndef _CONSOLE
#include "TimeProfiler.h"
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
	for (std::list<CCobThread *>::iterator i = running.begin(); i != running.end(); ++i) {
		delete *i;
	}
	for (std::list<CCobThread *>::iterator i = wantToRun.begin(); i != wantToRun.end(); ++i) {
		delete *i;
	}
	while (sleeping.size() > 0) {
		CCobThread *tmp;
		tmp = sleeping.top();
		sleeping.pop();
		delete tmp;
	}
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
			logOutput.Print("CobError: thread added to scheduler with unknown state (%d)", thread->state);
			break;
	}
}


void CCobEngine::TickThread(int deltaTime, CCobThread* thread)
{
	curThread = thread; // for error messages originating in CUnitScript

	int res = thread->Tick(deltaTime);
	thread->CommitAnims(deltaTime);

	if (res == -1)
		delete thread;

	curThread = NULL;
}


void CCobEngine::Tick(int deltaTime)
{
	SCOPED_TIMER("Scripts");

	GCurrentTime += deltaTime;

#if COB_DEBUG > 0
	logOutput.Print("----");
#endif

	// Advance all running threads
	for (std::list<CCobThread *>::iterator i = running.begin(); i != running.end(); ++i) {
		//logOutput.Print("Now 1running %d: %s", GCurrentTime, (*i)->GetName().c_str());
#ifdef _CONSOLE
		printf("----\n");
#endif
		TickThread(deltaTime, *i);
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
	if (sleeping.size() > 0) {
		CCobThread *cur = sleeping.top();
		while ((cur != NULL) && (cur->GetWakeTime() < GCurrentTime)) {

			// Start with removing the executing thread from the queue
			sleeping.pop();

			//Run forward again. This can quite possibly readd the thread to the sleeping array again
			//But it will not interfere since it is guaranteed to sleep > 0 ms
			//logOutput.Print("Now 2running %d: %s", GCurrentTime, cur->GetName().c_str());
#ifdef _CONSOLE
			printf("+++\n");
#endif
			if (cur->state == CCobThread::Sleep) {
				cur->state = CCobThread::Run;
				TickThread(deltaTime, cur);
			} else if (cur->state == CCobThread::Dead) {
				delete cur;
			} else {
				logOutput.Print("CobError: Sleeping thread strange state %d", cur->state);
			}
			if (sleeping.size() > 0)
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
		logOutput.Print("ScriptError: %s outside script execution", msg.c_str());
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
