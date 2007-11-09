class CUnitTypeData{
public:
	//
	CUnitTypeData();
	~CUnitTypeData();
	void Init(Global* G, const UnitDef* ud);

	const UnitDef* GetUnitDef();
	string GetName();

	bool IsMex();
	bool IsMetalMaker();
	bool IsEnergy();
	bool IsFactory();
	bool IsHub();

	bool IsAirCraft();
	bool IsGunship();
	bool IsFighter();
	bool IsBomber();

	bool IsUWCapable(){
		return IsUWStructure()||IsShip()||IsSub();
	}
	bool IsUWStructure(){ return false;}
	bool IsHovercraft(){ return false;}
	bool IsAmphib(){ return false;}
	bool IsShip(){ return false;}
	bool IsSub(){ return false;}

	bool IsMobile();

	bool IsAIRSUPPORT();

	bool IsAttacker();
	
private:
	Global* G;
	const UnitDef* ud;
	string unit_name; // the name of the unit trimmed in lowercase
};
