#ifndef UG_H
#define UG_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This is the class that defines a untis personality and traits.
// This was implemented for NTAI X project, but hasnt been used as of yet.

/*struct cpersona{
	int courage; // Is a unit fearful of strong units enough that ti will go out alone and not be bothered by an enemy unit passing by, or will it flee for more secure territory?
	int conform; // will the unit do things because its a good idea or because other units are doing it too, affects how strong a cue is
	int temper; // will a unit fly into a rage or be ignorant of emotion and cold and efficient?
	int sensitivity; // will an event affect this unit badly or very little? How easy is it to change this units personality?
	int nurture; // Does this unit prefer building or acting as support or resourcing rather than weapons etc Main attribute for con units build queues
};*/
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// NTAI's version of Command. Still needs further globalising, and
// for NTAI to make use of it for anythign other than construction.
// But this holds the potential to ahve enough code put inside it to
// replace the chaser and scouter agents when combined with a
// more itnelligent Tunit structure.
class Tunit;

class Task {
public:
	Task(){}
	Task(Global* GL);
	Task(Global* GL,string uname, int b_spacing = 9); //Used to start a construction task
	Task(Global* GL,  int b_type, Tunit* tu);
	Task(Global* GL, Command _c);
	virtual ~Task(){}

	bool execute(int uid);
	// executes the task on the unit whose id is supplied.


	Command c;

	// Used if this task is to build something only.
	string targ;
	bool construction;
	int spacing;
	bool help;
	int team;
	int type;

	int executions;
	bool valid;
	Tunit* t;
	// Is this task valid or can it never be carried out? If this si set
	// to false then the task will probably get deleted before its used.

protected:
	Global* G;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
enum cdgun {cd_no, cd_yes, cd_unsure};
class Tunit{
public:
	Tunit(){
		uid = 0;
		bad = true;
		can_dgun = cd_unsure;
		iscommander = false;
	}
	Tunit(Global* GLI, int id);
	bool iscommander;
	/*units dont need initialising after construction. When the constructor is called all the necessary variables to
	initialise will be present and InitAI would be called immediatly afterwards so why complicate the procedure with multiple calls*/

	virtual ~Tunit();


	void AddTask(string buildname, int spacing = 40); // Add a cosntruction task.
	void AddTask(int btype); // repair nearby unit that is currently being built
	void AddTaskt(Task t){
		tasks.push_back(t);
	}
	bool builder;
	bool attacker;
	bool scouter;
	bool factory; // Is this unit a factory?

	bool execute(Task t); // Execute this task.
	string GetBuild(int btype,bool water);
	string GetBuild(vector<string> v, bool efficiency=true);
	cdgun can_dgun;

	int uid; // The units ID.
	const UnitDef* ud; // It's unitDef
	Task Tcurrent;
	bool idle;
	list<Task> tasks; // Saved task list of tasks yet to eb done.
	int frame; // How many frames old is this unit.
	int creation; // What game frame was this unti created in.
	
	bool repeater; // when it finishes its build queue it gets given a new one
	bool guarding; //  when ti finishes its buidl queue it guards another unit
	bool scavenge; // When it finishes its build queue it ressurects thgins and reclaims obstacles and trees
	bool waiting; // Has this unti bene given a task that would be a ludicrous thign to do and is waiting for it to become feasible?
	bool reclaiming;


	bool bad;
protected:
	Global* G;
	//cpersona* p;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This can be used to control a group of units without iterating
// through each unit in turn.
// Could do with more complete implementation of extra functions
// and actually be used.

class ugroup{
public:
	ugroup(){
		G=0;
	}
	ugroup(Global* GLI){
		G = GLI;
	}
	virtual ~ugroup(){}
	//map<int,Tunit*> Getforce(){return umap;}
	int gid;
	int acknum;
	set<int> units;
	bool idle;
protected:
	Global* G;
};


#endif
