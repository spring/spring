
namespace ntai {
	class IBehaviour : public IModule{
	public:
		IBehaviour(){}
		IBehaviour(Global* GL, int uid){}
		virtual ~IBehaviour(){}
		virtual bool Init(){ return false;}
		virtual void RecieveMessage(CMessage &message){}
	protected:
		int uid;
		bool active;
	};
}
