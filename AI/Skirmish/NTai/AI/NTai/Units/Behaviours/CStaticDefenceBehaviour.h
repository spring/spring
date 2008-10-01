namespace ntai {

	class CStaticDefenceBehaviour : public IBehaviour{
	public:
		CStaticDefenceBehaviour(Global* GL, int uid);
		virtual ~CStaticDefenceBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
		bool engaged;
		int uid;
	};
}
