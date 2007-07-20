/*
AF 2007
*/

class CMessage {
public:
	CMessage(string my_type);
	CMessage(string my_type, vector<float> &myparameters);
	vector<float> GetParameters();
	float GetParameter(int i);
	void* GetOtherParameters();
	string GetType();
	void SetType(string type);
	float3 GetFloat3();
	float GetRadius();
	void AddParameter(int p);
	void AddParameter(float p);
	void AddParameter(float3 p);
private:
	string message_type;
	vector<float> parameters;
	void* otherstuff;
};
