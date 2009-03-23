
namespace  ntai {
	class CEfficiency : public IModule {
	public:
		CEfficiency(Global* GL);
		virtual ~CEfficiency();
		virtual void RecieveMessage(CMessage &message);
		virtual bool Init();

		virtual void DestroyModule();
		
		float GetEfficiency(string s, float def_value);
		void SetEfficiency(std::string s, float e);
		float GetEfficiency(std::string s, std::set<string>& doneconstructors, int techlevel);
		
		bool LoadUnitData();
		bool SaveUnitData();



		std::map<std::string, float> efficiency;
		std::map<std::string, float> builderefficiency;
		std::map<std::string, int> lastbuilderefficiencyupdate;
	};
}
