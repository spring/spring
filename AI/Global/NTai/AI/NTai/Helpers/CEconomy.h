// CEconomy

namespace ntai {
	class CEconomy{
	public:
		CEconomy(Global* GL);
		virtual ~CEconomy(){}
		btype Get(bool extreme,bool factory=true);
		bool BuildFactory(bool extreme=true);
		bool BuildPower(bool extreme=true);
		bool BuildMex(bool extreme=true);
		bool BuildEnergyStorage(bool extreme=true);
		bool BuildMetalStorage(bool extreme=true);
		bool BuildMaker(bool extreme=true);
	private:
		Global* G;
	};
}
