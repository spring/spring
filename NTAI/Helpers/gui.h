#include "IAICallback.h"

class Global;
class GUIController;
class GObject;
const float3 ZVector(0,0,0);

struct ARGB{
	int blue;
	int green;
	int red;
	int alpha;
};

struct Line{
	void Draw(Global* G, float3 rpos, int scale);
	float3 start;
	float3 end;
	int width;
	bool arrow;
	int FPS;

	bool Pulsate;
	int maxwidth;
	int minwidth;

	ARGB colour;
	bool rainbow;
	bool flash;
	ARGB flashcolour1;
	ARGB flashcolour2;
	int flashtime;
	int flashprogress;
	bool flashing;

	bool fade;
	int fadetime;
	int fadeprogress;
	bool fadecycle;
	int maxalpha;
	int minalpha;
};

struct Group{
	float3 position;
	void move(float3 shifted){}
	void Draw(Global* G){}
		//for(map<int,Line>::iterator il = lines.begin()
	map<int,Line> lines;
	float scale;
	bool visible;
};

class GObject{
public:
	GObject(GUIController* parent){
		gui = parent;
	}
	virtual ~GObject(){}
	bool Interface;

	int type;
	string name;

	string GetProperty(int p){
		string s = "";
		return s;
	}

	float3 corner;
	float3 centre;
	float width;
	float height;

	void Draw(bool hover){}// drawn
	void Click(float3 cpos){}// user has clicked
	void Update(){}// Update, called every frame
	void Exit(){}// hard exit, cannot be overridden

	GUIController* gui;

	bool rotate;
	int rspeed;
	float angle;
	vector<Group> Groups;

	float Scale;
};

class GUIController{
public:
	GUIController(Global* GL){}
	virtual ~GUIController(){}
	void Click(float3 cpos){
		for(vector<GObject>::iterator ig = GO.begin();ig != GO.end();++ig){
			ig->Click(cpos);
		}
	}
	void Draw(){
		for(vector<GObject>::iterator ig = GO.begin();ig != GO.end();++ig){
			ig->Draw(false);
		}
	}
	void DrawGroup(Group* g){
		}
	void Create(GObject* gob, float3 pos){}
	void Exit(GObject* gob){
		gob->Exit();
	}
	void Update(){
		for(vector<GObject>::iterator ig = GO.begin();ig != GO.end();++ig){
			ig->Update();
		}
	}
	void Open(float3 start){}
	void Close(){}
	float3 rotate(float3 centre, float3 pos, float angle, int axis){return ZVector;}
	vector<GObject> GO;
	IGlobalAICallback* cb;
	float3 cpos;
	Global* G;
};
