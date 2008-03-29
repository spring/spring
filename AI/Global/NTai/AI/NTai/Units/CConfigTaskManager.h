/*
AF 2004+
LGPL 2
*/

namespace ntai {
	class CConfigTaskManager : public ITaskManager {
	public:
		virtual boost::shared_ptr<IModule> GetNextTask();
	};
}
