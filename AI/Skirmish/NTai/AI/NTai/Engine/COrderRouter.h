namespace ntai {
	class COrderRouter{
	public:
		COrderRouter(Global* GL);
		virtual ~COrderRouter(){}
		bool GiveOrder(TCommand c, bool newer=true);
		bool SubjectiveCommand(int cmd);
		bool SubjectiveCommand(TCommand& cmd);
		bool SubjectOf(TCommand c, int unit);
		void CleanUpOrders();
		void IssueOrders();
		void UnitDestroyed(int uid);
		void Update();
	private:
		Global* G;
		vector<TCommand> CommandCache;// Command cache
	};
}
