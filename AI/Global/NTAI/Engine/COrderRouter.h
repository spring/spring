

class COrderRouter{
public:
	COrderRouter(Global* GL);
	virtual ~COrderRouter(){}
	bool GiveOrder(TCommand c, bool newer=true);
	void CleanUpOrders();
	void IssueOrders();
	void UnitDestroyed(int uid);
	void Update();
private:
	Global* G;
	vector<TCommand> CommandCache;// Command cache
	//vector<TCommand> OtherCached;
};
