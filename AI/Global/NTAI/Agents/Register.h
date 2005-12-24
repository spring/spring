#ifndef Reg_H
#define Reg_H
#include <list>
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"

class Register : public base{
public:
	Register(){
	lines = true;}
	virtual ~Register(){}
	void InitAI(Global* GLI){
		G = GLI;
	}
	void UnitCreated(int unit);
	void Update();
	void UnitMoveFailed(int unit){}
	void UnitIdle(int unit){}
	void UnitDestroyed(int unit);
	bool lines;
private:
	Global* G;
	//map<int,Tunit*> cp;
	set<int> units;
};

#endif
