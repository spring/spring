#ifndef CWORKERTHREAD_H_INCLUDED
#define CWORKERTHREAD_H_INCLUDED

/*class CWorkerThread{// : public IModule{
public:
    CWorkerThread();//{workerthreads = 0;}
    ~CWorkerThread();

    void NextThread();
    bool HasNext();

    void operator()();

    //void RecieveMessage(CMessage &message){}
	//bool Init(boost::shared_ptr<IModule> me){ return true;}

    //static int workerthreads;

};*/
#include <list>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
class ThreadPool
{
public:

	typedef boost::function0<void> Functor_T;

	ThreadPool()
	{
		init();
		m_nMaxThreads = m_nQueueSize = 0;
	}

	explicit
	ThreadPool(size_t nThreads, size_t nQueueSize = 0)
	: m_nMaxThreads(nThreads),
	  m_nQueueSize(nQueueSize)
	{
		init();

		//dgassert3(nQueueSize == 0 || nQueueSize >= nThreads, ThreadException, "Queue size must be zero or greater than number of threads")
		//dgassert3(nThreads > 0, ThreadException, "Thread pool created with zero threads")

		if (m_nQueueSize == 0)
			m_nQueueSize = m_nMaxThreads*2;
	}

	void setQueueSize(size_t x)
	{
        boost::mutex::scoped_lock lock1(m_mutex);
		m_nQueueSize = x;
	}

	size_t getQueueSize() const
	{
		return m_nQueueSize;
	}

	void setMaxThreads(size_t x)
	{
        boost::mutex::scoped_lock lock1(m_mutex);
		m_nMaxThreads = x;
	}

	size_t getMaxThreads() const
	{
		return m_nMaxThreads;
	}

	// Register a function to be called when a new thread is created
	void setThreadCreatedListener(const Functor_T &f)
	{
		m_threadCreated = f;
	}

	// Call a function in a separate thread managed by the pool
    void invoke(const Functor_T& threadfunc)
    {
        boost::mutex::scoped_lock lock1(m_mutex);

		//dgassert3(m_nMaxThreads, ThreadException, "Please set the maxThreads property")
		//dgassert3(m_nQueueSize >= m_nThreads, ThreadException, "The queue size must be greater or equal to the number of threads")

    	for(;;)
    	{
	    	//dgassert3(!m_bStop, dg::thread::ThreadException, "The thread pool has been stopped and can not longer service requests")
	    	//dgassert3(!m_nGeneralErrors, dg::thread::ThreadException, "An error has occurred in the thread pool and can no longer service requests")

	    	try
	    	{
		    	if ( m_waitingFunctors.size() < m_nThreads)
		    	{
			    	// Don't create a thread unless it's needed.  There
			    	// is a thread available to service this request.
			    	addFunctor(threadfunc);
			    	lock1.unlock();
		    	    break;
	    		}

	    	    bool bAdded = false;

                if (m_waitingFunctors.size() < m_nQueueSize)
		    	{
			    	// Don't create a thread unless you have to
			    	addFunctor(threadfunc);
		    	    bAdded = true;
	            }

		    	if (m_nThreads < m_nMaxThreads)
	    		{
		    		++m_nThreads;

		    		lock1.unlock();
		    		m_threads.create_thread(beginThreadFunc(*this));

		    		if (bAdded)
		    			break;

		    		// Terris -- if the mutex is unlocked before creating the thread
		    		// this allows m_threadAvailable to be triggered
		    		// before the wait below runs.  So run the loop again.
		    		lock1.lock();
   	    			continue;
   	    		}

    			if (bAdded)
	    		{
		    	    lock1.unlock();
		    		break;
	    		}

			    m_threadAvailable.wait(lock1);
	    	}
	    	catch(...)
	    	{
		    	// no idea what the state of lock1 is.. don't need thread safety here
		    	++m_nGeneralErrors;
		    	throw;
    		}
		}

		m_needThread.notify_all();
    }

    ~ThreadPool() throw()
    {
	    //try
	    //{
			stop();
    	//}
		//DG_CATCH_BASIC_LOG_STDERR
    }

	// This method is not thread-safe
    void stop()
    {
        // m_bStop must be set to true in a critical section.  Otherwise
        // it is possible for a thread to miss notify_all and never
        // terminate.
    	boost::mutex::scoped_lock lock1(m_mutex);
		m_bStop = true;
		lock1.unlock();

		m_needThread.notify_all();
    	m_threads.join_all();
   }

	// This method is not thread-safe
    void wait()
    {
        boost::mutex::scoped_lock lock1(m_mutex);

	    while (!m_waitingFunctors.empty())
	    {
            m_threadAvailable.wait(lock1);
   		}
    }

private:

    ThreadPool(const ThreadPool&);
    ThreadPool& operator = (const ThreadPool&);

	size_t m_nThreads,
		m_nMaxThreads,
		m_nQueueSize;

	typedef std::list<Functor_T> Container_T;
	Container_T m_waitingFunctors;
	Container_T::iterator m_nextFunctor;
	Functor_T	m_threadCreated;

    boost::mutex m_mutex;
    boost::condition 	m_threadAvailable, // triggered when a thread is available
    					m_needThread;      // triggered when a thread is needed
    boost::thread_group m_threads;
    bool 				m_bStop;
    long 	m_nGeneralErrors,
    		m_nFunctorErrors;

	void init()
	{
		m_nThreads = 0;
		m_nGeneralErrors = 0;
		m_nFunctorErrors = 0;
		m_bStop = false;
		m_threadCreated = NoOp();
		m_nextFunctor = m_waitingFunctors.end();
	}

    friend struct beginThreadFunc;

    void addFunctor(const Functor_T& func)
    {
	    bool bAtEnd = false;

	    if (m_nextFunctor == m_waitingFunctors.end())
	    	bAtEnd = true;

	    m_waitingFunctors.push_back(func);
	    if (bAtEnd)
	    {
	        --m_nextFunctor;
	    }
    }

	struct NoOp
	{
		void operator () () const
		{
		}
	};

    struct beginThreadFunc
    {
	    beginThreadFunc(ThreadPool& impl)
	    : m_impl(impl)
	    {
	    }

        void operator() ()
        {
            m_impl.beginThread();
        }

        ThreadPool &m_impl;
    };

    // Thread entry point.  This method runs once per thread.
    void beginThread() throw()
    {
        try
        {
			m_threadCreated();

	    	boost::mutex::scoped_lock lock1(m_mutex);

            for(;;)
            {
		    	if (m_bStop)
		    		break;

				if (m_nextFunctor == m_waitingFunctors.end())
				{
					// Wait until someone needs a thread
					m_needThread.wait(lock1);
				}
				else
				{
					// ++m_nThreadsInProcess;
					Container_T::iterator iter = m_nextFunctor;
					Functor_T &func = (*iter); // m_waitingFunctors.front();
					++m_nextFunctor;

					lock1.unlock();

		            try
		            {
			            (func)();
		            }
		            catch(...)
		            {
		                lock1.lock();
			            ++m_nFunctorErrors;
		                //--m_nThreadsInProcess;
		               	m_waitingFunctors.erase(iter);
			            throw;
		            }

		            lock1.lock();
		            m_waitingFunctors.erase(iter);
	                // --m_nThreadsInProcess;
	                lock1.unlock();

		            m_threadAvailable.notify_all();

		            lock1.lock();
				}
			}
		}
		catch(...)
		{
			// This is a real problem.  Since this thread is about to die, m_nThreads
			// will be out of sync with the actual number of threads.  But this should
			// not occur except when something really bad happens.
			// not thread safe.. who cares
			++m_nGeneralErrors;

			// Log the exception and exit this thread
			//try
			//{
			//	throw;
			//}
			//DG_CATCH_BASIC_LOG_STDERR
		}
    }
};


#endif // CWORKERTHREAD_H_INCLUDED
