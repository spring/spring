#ifndef UG_H
#define UG_H
#include <list>
#include "AICallback.h"
#include "Sim/Units/UnitDef.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This is the class that defines a units personality and traits.
// This was implemented for NTAI X project, but hasn't been used as of yet.

/*struct cpersona{
	int courage; // Is a unit fearful of strong units enough that ti will go out alone and not be bothered by an enemy unit passing by, or will it flee for more secure territory?
	int conform; // will the unit do things because its a good idea or because other units are doing it too, affects how strong a cue is
	int temper; // will a unit fly into a rage or be ignorant of emotion and cold and efficient?
	int sensitivity; // will an event affect this unit badly or very little? How easy is it to change this units personality?
	int nurture; // Does this unit prefer building or acting as support or resourcing rather than weapons etc Main attribute for con units build queues
};*/
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// NTAI's version of Command. Still needs further globalising, and
// for NTAI to make use of it for anything other than construction.
// But this holds the potential to have enough code put inside it to
// replace the chaser and scouter agents when combined with a
// more intelligent Tunit structure.
class CUBuild;
class CBuilder;


class Task {
public:
	Task(Global* GL);
	Task(Global* GL,bool meta, string uname, int b_spacing = 9); //Used to start a construction task
	Task(Global* GL,  btype b_type);
	Task(Global* GL, Command _c);
	virtual ~Task(){}
	bool execute(int uid);
	// executes the task on the unit whose id is supplied.
	// Used if this task is to build something only.
	bool valid;
	// Is this task valid or can it never be carried out? If this is set
	// to false then the task will probably get deleted before its used.
	bool  metatag;
	bool antistall;

	string targ;
	
	bool construction;
	btype type;
protected:
	int spacing;
	Command c;
	Global* G;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
/*
class CUBuild;
class Tunit{
public:
	Tunit(){
		uid = 0;
		bad = true;
		can_dgun = cd_unsure;
		iscommander = false;
	}
	Tunit(Global* GLI, int unit);
	bool iscommander;
	//units dont need initialising after construction. When the constructor is called all the necessary variables to
	//initialise will be present and InitAI would be called immediatly afterwards so why complicate the procedure with multiple calls

	virtual ~Tunit();


	void AddTask(string buildname, int spacing = 40); // Add a cosntruction task.
	void AddTask(int btype); // repair nearby unit that is currently being built
	void AddTaskt(Task t){
		tasks.push_back(t);
	}
	int role;

	bool execute(Task t); // Execute this task.
	string GetBuild(vector<string> v, bool efficiency=true);
	CUBuild ubuild;

	cdgun can_dgun;

	unsigned int uid; // The units ID.
	const UnitDef* ud; // It's unitDef
	//Task Tcurrent;
	//bool idle;
	list<Task> tasks; // Saved task list of tasks yet to be done.
	//int frame; // How many frames old is this unit.
	int creation; // What game frame was this unit created in.
	//unit_state state;
	bool repeater; // when it finishes its build queue it gets given a new one
	bool reclaiming;
	bool guarding;
	bool scavenging;
	bool waiting;
	bool dgunning;
	int frame;
	bool factory; // Is this unit a factory?


	bool bad;
protected:
	Global* G;
	//cpersona* p;
};*/

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
	set<int> units;
	bool idle;
protected:
	Global* G;
};


#endif
