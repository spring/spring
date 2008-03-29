/*
AF 2004+
LGPL 2
*/

namespace ntai {
	class ITaskManager : public IModule {
	public:
		virtual boost::shared_ptr<IModule> GetNextTask() = 0;
	};
}
