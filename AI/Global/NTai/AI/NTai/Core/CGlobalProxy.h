namespace ntai {

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

	class CGlobalProxy : public IModule{
	public:
		CGlobalProxy(Global* GL);
		virtual ~CGlobalProxy();

		bool Init();

		void RecieveMessage(CMessage &message);

	private:
		Global* G;
	};

}
