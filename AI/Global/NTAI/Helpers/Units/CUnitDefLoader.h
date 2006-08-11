// haha

class CUnitDefLoader{
public:
	CUnitDefLoader(Global* GL);
	virtual ~CUnitDefLoader(){}
	void Init();
	const UnitDef* GetUnitDef(string name);
private:
	Global* G;
	map<string,const UnitDef*> defs;
};
