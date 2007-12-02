
namespace ntai {
	class CKamikazeBehaviour : public IBehaviour{
	public:
		CKamikazeBehaviour(Global* GL, int uid);
		virtual ~CKamikazeBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
		float maxrange;
		bool engaged;
		int uid;
	};
}
