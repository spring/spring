// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used freely for any purpose, as
// long as this notice remains unchanged

#ifndef GMLSRV_H
#define GMLSRV_H

#ifdef USE_GML

#include <boost/thread/barrier.hpp>
#include <boost/bind.hpp>

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
	GML_TYPENAME std::list<U> *iter;
	int limit1;
	int limit2;
	BOOL_ serverwork;
	void (*serverfun)(void *);

	gmlCount UnitCounter;
	gmlExecState(R (*wrk)(void *)=NULL,R (*wrka)(void *,A)=NULL,R (*wrki)(void *,U)=NULL,
		void* cls=NULL,int mt=0,BOOL_ sm=FALSE,int nu=0,GML_TYPENAME std::list<U> *it=NULL,int l1=1,int l2=1,BOOL_ sw=FALSE,void (*swf)(void *)=NULL):
	worker(wrk),workerarg(wrka),workeriter(wrki),workerclass(cls),maxthreads(mt),
		syncmode(sm),num_units(nu),iter(it),limit1(l1),limit2(l2),serverwork(sw),serverfun(swf),UnitCounter(-1) {
	}

	void ExecServerFun() {
		if(serverfun)
			(*serverfun)(workerclass);
	}

	void ExecAll(int &pos, typename std::list<U>::iterator &it) {
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

	BOOL_ Exec(int &pos, typename std::list<U>::iterator &it) {
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
	BOOL_ inited;
	gmlCount threadcnt;
	gmlCount ClientsReady;
	BOOL_ newwork;

	gmlClientServer():threadcnt(0),ClientsReady(0),Barrier(GML_CPU_COUNT),ExecDepth(0),newwork(FALSE) {
		inited=FALSE;
	}

	~gmlClientServer() {
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

			typename std::list<U>::iterator it;
			if(ex->workeriter)
				it=ex->iter->begin();
			int pos=0;
//			int nproc=0;
			int updsrv=0;
			if(gmlThreadCount>1) {
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
//			GML_DEBUG("server ",nproc)
			if(ExecDepth>0 && !*(volatile int *)&newwork) {
				--ExecDepth;
				newwork=-1;
			}

		} while(*(volatile int *)&newwork);
	}

	void WorkInit() {
		if(!inited) {
			gmlInit();
			for(int i=1; i<gmlThreadCount; ++i)
				threads[i]=new boost::thread(boost::bind<void, gmlClientServer, gmlClientServer*>(&gmlClientServer::gmlClient, this));
			inited=TRUE;
		}
	}

	void Work(R (*wrk)(void *),R (*wrka)(void *,A), R (*wrkit)(void *,U),void *cls,int mt,BOOL_ sm, GML_TYPENAME std::list<U> *it,int nu,int l1,int l2,BOOL_ sw,void (*swf)(void *)=NULL) {
		if(gmlThreadNumber!=0) {
			NewWork(wrk,wrka,wrkit,cls,mt,sm,it,nu,l1,l2,sw,swf);
			return;
		}
		GML_TYPENAME gmlExecState<R,A,U> *ex=ExecState;
		new (ex) GML_TYPENAME gmlExecState<R,A,U>(wrk,wrka,wrkit,cls,mt,sm,nu,it,l1,l2,sw,swf);
		if(!inited)
			WorkInit();
		gmlServer();
	}

	void NewWork(R (*wrk)(void *),R (*wrka)(void *,A), R (*wrkit)(void *,U),void *cls,int mt,BOOL_ sm, GML_TYPENAME std::list<U> *it,int nu,int l1,int l2,BOOL_ sw,void (*swf)(void *)=NULL) {
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

		typename std::list<U>::iterator it;
		if(ex->workeriter)
			it=((GML_TYPENAME std::list<U> *)*(GML_TYPENAME std::list<U> * volatile *)&ex->iter)->begin();
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
//				GML_DEBUG("client ",exproc)
//			}
		}
		qd->ReleaseWrite();
		++ClientsReady;	
	}

	void gmlClient() {
		gmlThreadNumber=++threadcnt;
		streflop_init<streflop::Simple>();
		while(1) {
			gmlClientSub();
		}
	}
};

#endif // USE_GML

#endif
