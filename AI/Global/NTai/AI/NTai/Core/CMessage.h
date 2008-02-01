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

	int GetFrame();
	void SetFrame(int frame);

	
	float3 GetFloat3();
	float GetRadius();

	void AddParameter(int p);
	void AddParameter(float p);
	void AddParameter(float3 p);

	void SetLifeTime(int lifetime);
	bool IsDead(int currentFrame);
	int GetLifeTime();
	int GetRemainingLifeTime(int currentFrame);

private:
	int frame;
	int lifetime; // -1 indefinate
	string message_type;
	vector<float> parameters;
	void* otherstuff;
};
