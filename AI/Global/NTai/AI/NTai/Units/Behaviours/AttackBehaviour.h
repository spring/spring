
namespace ntai {
	class CAttackBehaviour : public IBehaviour{
	public:
		CAttackBehaviour(Global* GL, int uid);
		virtual ~CAttackBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
		bool engaged;
	};
}
