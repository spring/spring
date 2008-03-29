/*
AF 2004+
LGPL 2
*/

namespace ntai {

	class CUnit : public IModule{
	public:
		CUnit(){
			G = 0;
			valid=false;
		}
		CUnit(Global* GL);
		CUnit(Global* GL, int uid);
		virtual ~CUnit();

		void RecieveMessage(CMessage &message);
		bool Init();
		CUnitTypeData* GetUnitDataType();
		bool operator==(int unit);
		int GetID();
		/*void SetRepeat(bool n){
			repeat = n;
		}
		bool GetRepeat(){
			return repeat;
		}*/
		int GetAge();
		//bool AddTask(boost::shared_ptr<IModule> &t);
		//bool LoadTaskList();
		bool LoadBehaviours();

		bool doingplan;
		uint curplan;

		void SetTaskManager(boost::shared_ptr<ITaskManager> taskManager);
		boost::shared_ptr<ITaskManager> GetTaskManager();
	protected:
		boost::shared_ptr<ITaskManager> taskManager;
		bool under_construction;
		boost::shared_ptr<IModule> currentTask;
		//list< boost::shared_ptr<IModule> > tasks;
		list< boost::shared_ptr<IBehaviour> > behaviours;
		CUnitTypeData* utd;
		int uid;
		//bool repeat;
		int birth;
		//bool nolist;
	};
}
