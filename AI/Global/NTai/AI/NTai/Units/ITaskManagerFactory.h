/*
AF 2004+
LGPL 2
*/

namespace ntai {
	class ITaskManagerFactory : public IModule {
	public:
		virtual boost::shared_ptr<ITaskManager> GetTaskManager(boost::shared_ptr<IModule> unit) = 0;
	};
}
