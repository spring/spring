
namespace ntai {
	class CMetalMakerBehaviour : public IBehaviour{
	public:
		CMetalMakerBehaviour(Global* GL, int uid);
		virtual ~CMetalMakerBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
		bool turnedOn;
		float energyUse;
	};
}
