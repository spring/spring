//New construction system


class CBuilder{
public:
	CBuilder(Global* GL, int uid);
	virtual ~CBuilder();
	const UnitDef* GetUnitDef();
	bool operator==(int unit);
	bool AddTask(btype type);
	bool AddTask(string buildname,bool meta, int spacing=9);
	bool IsValid();
	int GetID();
	unit_role GetRole();
	void SetRole(unit_role new_role);
	void SetRepeat(bool n){
		repeat = n;
	}
	bool GetRepeat(){
		return repeat;
	}
	int GetAge();
	bool antistall;
	vector<Task> tasks;
	CUBuild Build;
private:
	Global* G;
	const UnitDef* ud;
	unit_role role;
	int uid;
	bool constring;
	string con;
	bool valid;
	bool repeat;
	int birth;
};

