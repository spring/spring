// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used, distributed and modified 
// freely for any purpose, as long as 
// this notice remains unchanged

#ifndef GMLSRV_H
#define GMLSRV_H

#ifdef USE_GML

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/bind.hpp>
#include "System/Platform/errorhandler.h"
#include "lib/streflop/streflop_cond.h"
#if !defined(_MSC_VER) && defined(_WIN32)
#	include <windows.h>
#endif

EXTERN inline void gmlUpdateServers() {
	gmlItemsConsumed=0;
	gmlProgramServer.GenerateItems();
	gmlProgramObjectARBServer.GenerateItems();
	gmlShaderServer_VERTEX.GenerateItems();
	gmlShaderServer_FRAGMENT.GenerateItems();
	gmlShaderServer_GEOMETRY_EXT.GenerateItems();
	gmlShaderObjectARBServer_VERTEX.GenerateItems();
	gmlShaderObjectARBServer_FRAGMENT.GenerateItems();
	gmlShaderObjectARBServer_GEOMETRY_EXT.GenerateItems();
	gmlQuadricServer.GenerateItems();

	gmlTextureServer.GenerateItems();
	gmlBufferARBServer.GenerateItems();
	gmlFencesNVServer.GenerateItems();
	gmlProgramsARBServer.GenerateItems();
	gmlRenderbuffersEXTServer.GenerateItems();
	gmlFramebuffersEXTServer.GenerateItems();
	gmlQueryServer.GenerateItems();
	gmlBufferServer.GenerateItems();

	gmlListServer.GenerateItems();
}

#define GML_MAX_EXEC_DEPTH 4

template<class R, class A, typename U>
struct gmlExecState {
	R (*worker)(void *);
	R (*workerarg)(void *,A);
	R (*workeriter)(void *,U);
	void* workerclass;
	int maxthreads;
	BOOL_ syncmode;
	int num_units;
	const GML_TYPENAME std::set<U> *iter;
	int limit1;
	int limit2;
	BOOL_ serverwork;
	void (*serverfun)(void *);

	gmlCount UnitCounter;
	gmlExecState(R (*wrk)(void *)=NULL,R (*wrka)(void *,A)=NULL,R (*wrki)(void *,U)=NULL,
		void* cls=NULL,int mt=0,BOOL_ sm=FALSE,int nu=0,const GML_TYPENAME std::set<U> *it=NULL,int l1=1,int l2=1,BOOL_ sw=FALSE,void (*swf)(void *)=NULL):
	worker(wrk),workerarg(wrka),workeriter(wrki),workerclass(cls),maxthreads(mt),
		syncmode(sm),num_units(nu),iter(it),limit1(l1),limit2(l2),serverwork(sw),serverfun(swf),UnitCounter(-1) {
	}

	void ExecServerFun() {
		if(serverfun)
			(*serverfun)(workerclass);
	}

	void ExecAll(int &pos, typename std::set<U>::const_iterator &it) {
		int i=UnitCounter;
		if(i>=num_units)
			return;
		if(workeriter) {
			while(++i<num_units) {
				(*workeriter)(workerclass,*it);
				++it;
				++pos;
			}
		}
		else if(worker) {
			while(++i<num_units)
				(*worker)(workerclass);
		}
		else if(workerarg) {
			while(++i<num_units)
				(*workerarg)(workerclass,i);
		}
		UnitCounter%=num_units;
	}

	BOOL_ Exec(int &pos, typename std::set<U>::const_iterator &it) {
		int i=++UnitCounter;
		if(i>=num_units)
			return FALSE;
		if(workeriter) {
			while(pos<i) {
				++it;
				++pos;
			}
			(*workeriter)(workerclass,*it);
		}
		else if(worker) {
			(*worker)(workerclass);
		}
		else if(workerarg) {
			(*workerarg)(workerclass,i);
		}
		return TRUE;
	}

};


template<class R,class A, typename U>
class gmlClientServer {
public:
	int ExecDepth;
	GML_TYPENAME gmlExecState<R,A,U> ExecState[GML_MAX_EXEC_DEPTH];
	boost::barrier Barrier; 
	boost::thread *threads[GML_MAX_NUM_THREADS];
	volatile BOOL_ dorun;
	BOOL_ inited;
	gmlCount threadcnt;
	gmlCount ClientsReady;
	BOOL_ newwork;

	BOOL_ auxinited;
	R (*auxworker)(void *);
	void* auxworkerclass;
	boost::barrier AuxBarrier; 
	gmlCount AuxClientsReady;


	gmlClientServer()
		: ExecDepth(0)
		, Barrier(GML_CPU_COUNT)
		, dorun(TRUE)
		, inited(FALSE)
		, threadcnt(0)
		, ClientsReady(0)
		, newwork(FALSE)
		, auxinited(FALSE)
		, auxworker(NULL)
		, AuxBarrier(2)
		, AuxClientsReady(0) {
	}

	~gmlClientServer() {
		if(inited) {
			GML_TYPENAME gmlExecState<R,A,U> *ex=ExecState+ExecDepth;
			BOOL_ dowait=TRUE;
			for(int i=1; i<gmlThreadCount; ++i) {
				if(!threads[i]->joinable() || threads[i]->timed_join(boost::posix_time::milliseconds(10)))
					dowait=FALSE;
			}
			ex->maxthreads=0;
			dorun=FALSE;
			if(dowait)
				Barrier.wait();
			for(int i=1; i<gmlThreadCount; ++i) {
				if(threads[i]->joinable()) {
					if(dowait)
						threads[i]->join();
					else if(!threads[i]->timed_join(boost::posix_time::milliseconds(100)))
						threads[i]->interrupt();
				}
				delete threads[i];
			}
		}
		if(auxinited) {
			BOOL_ dowait=threads[gmlThreadCount]->joinable() && !threads[gmlThreadCount]->timed_join(boost::posix_time::milliseconds(10));
			auxworker=NULL;
			dorun=FALSE;
			if(dowait)
				AuxBarrier.wait();
			if(threads[gmlThreadCount]->joinable()) {
				if(dowait)
					threads[gmlThreadCount]->join();
				else if(!threads[gmlThreadCount]->timed_join(boost::posix_time::milliseconds(100)))
					threads[gmlThreadCount]->interrupt();
			}
			delete threads[gmlThreadCount];
		}
	}

	void gmlServer() {

		do {
			ClientsReady%=0;
			if(newwork>0)
				++ExecDepth;
			GML_TYPENAME gmlExecState<R,A,U> *ex=ExecState+ExecDepth;
			if(newwork==0)
				ex->UnitCounter%=-1;
			BOOL_ execswf=newwork>=0;
			newwork=0;

			Barrier.wait();
			
			if(execswf)
				ex->ExecServerFun();

			typename std::set<U>::const_iterator it;
			if(ex->workeriter)
				it=ex->iter->begin();
			int pos=0;
//			int nproc=0;
			int updsrv=0;
			if(ex->maxthreads>1) {
				while(ClientsReady<=gmlThreadCount+1) {
					if((updsrv++%GML_UPDSRV_INTERVAL)==0 || *(volatile int *)&gmlItemsConsumed>=GML_UPDSRV_INTERVAL)
						gmlUpdateServers();
					BOOL_ processed=FALSE;

					for(int i=1; i<gmlThreadCount; ++i) {
						gmlQueue *qd=&gmlQueues[i];
						if(qd->Reloc)
							qd->Realloc();
						if(qd->GetRead()) {
							qd->Execute();
							qd->ReleaseRead();
							processed=TRUE;
						}
						if(qd->Sync) {
							qd->ExecuteSynced();
							processed=TRUE;
						}
					}
					if(GML_SERVER_GLCALL && ex->serverwork && !processed) {
						if(ex->Exec(pos,it)) {
							//						++nproc;
						}
					}

					if(ClientsReady>=gmlThreadCount-1)
						++ClientsReady;
				}
			}
			else {
				ex->ExecAll(pos,it);
			}

//			GML_DEBUG("server ",nproc, 3)
			if(ExecDepth>0 && !*(volatile int *)&newwork) {
				--ExecDepth;
				newwork=-1;
			}

		} while(*(volatile int *)&newwork);
	}

	void WorkInit() {
//		set_threadnum(0);
		gmlInit();

		for(int i=1; i<gmlThreadCount; ++i)
			threads[i]=new boost::thread(boost::bind<void, gmlClientServer, gmlClientServer*>(&gmlClientServer::gmlClient, this));
#if GML_ENABLE_TLS_CHECK
		for(int i=0; i<GML_MAX_NUM_THREADS; ++i)
			boost::thread::yield();
		if(gmlThreadNumber!=0) {
			handleerror(NULL, "Thread Local Storage test failed", "GML error:", MBF_OK | MBF_EXCL);
		}
#endif
		inited=TRUE;
	}

	void Work(R (*wrk)(void *),R (*wrka)(void *,A), R (*wrkit)(void *,U),void *cls,int mt,BOOL_ sm, const GML_TYPENAME std::set<U> *it,int nu,int l1,int l2,BOOL_ sw,void (*swf)(void *)=NULL) {
		if(!inited)
			WorkInit();
		if(auxworker)
			--mt;
		if(gmlThreadNumber!=0) {
			NewWork(wrk,wrka,wrkit,cls,mt,sm,it,nu,l1,l2,sw,swf);
			return;
		}
		GML_TYPENAME gmlExecState<R,A,U> *ex=ExecState;
		new (ex) GML_TYPENAME gmlExecState<R,A,U>(wrk,wrka,wrkit,cls,mt,sm,nu,it,l1,l2,sw,swf);
		gmlServer();
	}

	void NewWork(R (*wrk)(void *),R (*wrka)(void *,A), R (*wrkit)(void *,U),void *cls,int mt,BOOL_ sm, const GML_TYPENAME std::set<U> *it,int nu,int l1,int l2,BOOL_ sw,void (*swf)(void *)=NULL) {
		gmlQueue *qd=&gmlQueues[gmlThreadNumber];
		qd->ReleaseWrite();

		GML_TYPENAME gmlExecState<R,A,U> *ex=ExecState+ExecDepth;
		new (ex+1) GML_TYPENAME gmlExecState<R,A,U>(wrk,wrka,wrkit,cls,mt,sm,nu,it,l1,l2,sw,swf);
		newwork=TRUE;

		++ClientsReady;	
		gmlClientSub();

		Barrier.wait();

		qd->GetWrite(TRUE);
		if(ex->syncmode)
			qd->SyncRequest();

	}

	void gmlClientSub() {
		Barrier.wait();

		GML_TYPENAME gmlExecState<R,A,U> *ex=ExecState+ExecDepth;
		
		int thread=gmlThreadNumber;
		if(thread>=ex->maxthreads) {
			++ClientsReady;	
			return;
		}

		typename std::set<U>::iterator it;
		if(ex->workeriter)
			it=((GML_TYPENAME std::set<U> *)*(GML_TYPENAME std::set<U> * volatile *)&ex->iter)->begin();
		int pos=0;

		int processed=0;
		gmlQueue *qd=&gmlQueues[thread];

		qd->GetWrite(TRUE); 

		if(ex->syncmode)
			qd->SyncRequest();

		while(ex->Exec(pos,it)) {
			++processed;

//			int exproc=processed;
#if GML_ALTERNATE_SYNCMODE
			if(qd->WasSynced && qd->GetWrite(ex->syncmode?TRUE:2))
#else
			if(qd->WasSynced && qd->GetWrite(TRUE))
#endif
				processed=0;
			if(processed>=ex->limit1 && qd->GetWrite(processed>=ex->limit2))
				processed=0;
//			if(exproc!=processed) {
//				GML_DEBUG("client ",exproc, 3)
//			}
		}
		qd->ReleaseWrite();
		++ClientsReady;	
	}

	void gmlClient() {
		set_threadnum(++threadcnt);
		streflop_init<streflop::Simple>();
		while(dorun) {
			gmlClientSub();
		}
	}

	void GetQueue() {
		gmlQueue *qd=&gmlQueues[gmlThreadCount];

		if(!qd->WasSynced && qd->Write==qd->WritePos)
			return;

		BOOL_ isq1=qd->Write==qd->Queue1;

		qd->GetWrite(qd->WasSynced?2:TRUE);

		if(isq1) {
			while(!qd->Locked1 && *(BYTE * volatile *)&qd->Pos1!=qd->Queue1)
				boost::thread::yield();
		}
		else {
			while(!qd->Locked2 && *(BYTE * volatile *)&qd->Pos2!=qd->Queue2)
				boost::thread::yield();
		}
	}

	BOOL_ PumpAux() {
		static int updsrvaux=0;
		if((updsrvaux++%GML_UPDSRV_INTERVAL)==0 || *(volatile int *)&gmlItemsConsumed>=GML_UPDSRV_INTERVAL)
			gmlUpdateServers();

		while(AuxClientsReady<=3) {
			gmlQueue *qd=&gmlQueues[gmlThreadCount];
			if(qd->Reloc)
				qd->Realloc();
			if(qd->GetRead()) {
				qd->ExecuteDebug();
				qd->ReleaseRead();
			}
			if(qd->Sync) {
				qd->ExecuteSynced(&gmlQueue::ExecuteDebug);
			}
			if(AuxClientsReady==0)
				return FALSE;
			else
				++AuxClientsReady;
		}
		return TRUE;
	}

	void AuxWork(R (*wrk)(void *),void *cls) {
		auxworker=wrk;
		auxworkerclass=cls;
		AuxClientsReady%=0;
		if(!auxinited) {
			if(!inited)
				WorkInit();
			threads[gmlThreadCount]=new boost::thread(boost::bind<void, gmlClientServer, gmlClientServer*>(&gmlClientServer::gmlClientAux, this));
			auxinited=TRUE;
		}
		AuxBarrier.wait();
	}


	void gmlClientAuxSub() {
		AuxBarrier.wait();

		if(!auxworker)
			return;
		
		gmlQueue *qd=&gmlQueues[gmlThreadCount];

		qd->GetWrite(TRUE); 

		(*auxworker)(auxworkerclass);

		qd->ReleaseWrite();

		++AuxClientsReady;	
		auxworker=NULL;
	}

	void gmlClientAux() {
#ifdef _WIN32
		extern HANDLE simthread; // from System/Platform/Win/CrashHandler.cpp
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
						&simthread, 0, TRUE, DUPLICATE_SAME_ACCESS);
#endif
		set_threadnum(gmlThreadCount);
		streflop_init<streflop::Simple>();
		while(dorun) {
			gmlClientAuxSub();
		}
	}

	void ExpandAuxQueue() {
		gmlQueue *qd=&gmlQueues[gmlThreadNumber];
		while(qd->WriteSize<qd->Write+GML_AUX_PREALLOC)
			qd->WaitRealloc();
	}

};

#endif // USE_GML

#endif
