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
	
	bool GetSingleBuild();// is this unit type subject to the single build behaviour?
	void SetSingleBuild(bool value);

	bool GetSingleBuildActive();// is this behaviour currently active? (is one of these being built atm?)
	void SetSingleBuildActive(bool value);

	bool CanBuild();
private:
	Global* G;
	const UnitDef* ud;
	string unit_name; // the name of the unit trimmed in lowercase

	bool attacker;

	bool singleBuild;
	bool singleBuildActive;
};
