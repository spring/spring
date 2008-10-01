
namespace ntai {
	class CDGunBehaviour : public IBehaviour{
	public:
		CDGunBehaviour(Global* GL, int uid);
		virtual ~CDGunBehaviour();
		bool Init();
		void RecieveMessage(CMessage &message);
	private:
		int uid;
		bool active;
	};
}
