/*
NTai
Tom J Nowell
tarendai@darkstars.co.uk
www.darkstars.co.uk
LGPL 2 licence 2004+
*/

namespace ntai {

	class CUnit : public IModule{
	public:
		CUnit();
		CUnit(Global* GL, int uid);
		virtual ~CUnit();
		
		bool operator==(int unit);

		bool Init();
		void RecieveMessage(CMessage &message);
		
		CUnitTypeData* GetUnitDataType();
		

		int GetID();
		int GetAge();

		bool LoadBehaviours();

		

		void SetTaskManager(ITaskManager* taskManager);
		ITaskManager* GetTaskManager();
	protected:
		
		bool doingplan;
		uint curplan;
		ITaskManager* taskManager;
		bool under_construction;
		boost::shared_ptr<IModule> currentTask;
		list< boost::shared_ptr<IBehaviour> > behaviours;
		CUnitTypeData* utd;
		int uid;
		int birth;
	};
}
