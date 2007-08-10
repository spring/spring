#ifndef CWORKERTHREAD_H_INCLUDED
#define CWORKERTHREAD_H_INCLUDED

class CWorkerThread : IModule{
    CWorkerThread(){}//{workerthreads = 0;}
    ~CWorkerThread(){}

    void InvokeLater(boost::shared_ptr<IModule> thread){}

    void operator()(){}
    //static int workerthreads;
    vector<boost::shared_ptr<IModule> > tasks;
};

#endif // CWORKERTHREAD_H_INCLUDED
