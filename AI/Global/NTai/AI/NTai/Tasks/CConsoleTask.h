
namespace ntai {

	class CConsoleTask : public IModule {
	public:
		CConsoleTask(Global* GL);
		CConsoleTask(Global* GL, string message);
		void RecieveMessage(CMessage &message);
		bool Init();
		bool Succeeded();
	protected:
		bool succeed;
		string mymessage;
	};
}
