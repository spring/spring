/*
AF 2007
*/

// boost::shared_ptr<IModule> 
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
	bool Init(boost::shared_ptr<IModule> me);
	const UnitDef* GetUnitDef();
	bool operator==(int unit);
	int GetID();
	void SetRepeat(bool n){
		repeat = n;
	}
	bool GetRepeat(){
		return repeat;
	}
	int GetAge();
	bool AddTask(boost::shared_ptr<IModule> &t);
	bool LoadTaskList();

	bool doingplan;
	uint curplan;
protected:
	bool under_construction;
	bool executenext;
	vector< boost::shared_ptr<IModule> > tasks;
	const UnitDef* ud;
	int uid;
	bool constring;
	string con;
	bool repeat;
	int birth;
	bool nolist;
	bool killnext;
};

