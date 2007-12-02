
namespace ntai {
	class CMoveFailReclaimBehaviour : public IBehaviour{
	public:
		CMoveFailReclaimBehaviour(Global* GL, int uid);
		virtual ~CMoveFailReclaimBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
	};
}
