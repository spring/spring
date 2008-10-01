namespace ntai {
	class CRetreatBehaviour : public IBehaviour{
	public:
		CRetreatBehaviour(Global* GL, int uid);
		virtual ~CRetreatBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
		int uid;
		bool active;
		float damage;
	};
}
