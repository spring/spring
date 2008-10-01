/*
AF 2004+
LGPL 2
*/

namespace ntai {
	class CConfigTaskManager : public ITaskManager {
	public:
		CConfigTaskManager(Global* G, boost::shared_ptr<IModule> unit);
		
		virtual bool Init();
		virtual void RecieveMessage(CMessage &message);
		
		virtual boost::shared_ptr<IModule> GetNextTask();
		bool HasTasks();
		void TaskFinished();

		virtual bool LoadTaskList();

		void RemoveAllTasks();
	protected:
		std::list< boost::shared_ptr<IModule> > tasks;
		bool nolist;
		bool repeat;
		boost::shared_ptr<IModule> unit;
	};
}
