#ifndef UG_H
#define UG_H
#include <list>
#include "AICallback.h"
#include "UnitDef.h"

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
// Defines for abstract tags

#define C_METAL "m"
#define C_ENERGY "e"
#define C_RADAR "r"
#define C_WISHLIST "w"
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// NTAI's version of Command. Still needs further globalising, and
// for NTAI to make use of it for anythign other than construction.
// But this holds the potential to ahve enough code put inside it to
// replace the chaser and scouter agents when combined with a
// more itnelligent Tunit structure.

class Task {
public:
	Task(){}
	Task(Global* GL);
	Task(Global* GL,string uname); //Used to start a construction task
	virtual ~Task(){}

	bool execute(int uid);
	// executes the task on the unit whose id is supplied.

	void refresh();
	// err, forgotten what this does, I'll check back when I have looked

	Command* c;

	// Used if this task is to build somehting only.
	string targ;
	bool construction;

	int executions;
	bool valid;
	// Is this task valid or can it never be carried out? If this si set
	// to false then the task will probably get deleted before its used.

protected:
	Global* G;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

class Tunit : public base{
public:
	Tunit(){}
	Tunit(Global* GLI, int id);
	/*units dont need initialising after construction. When the constructor is called all the necessary variables to
	initialise will be present and InitAI would be called immediatly afterwards so why complicate the procedure with multiple calls*/

	virtual ~Tunit();


	void AddTask(string buildname); // Add a cosntruction task.

	bool execute(Task* t); // Execute this task.

	int uid; // The units ID.
	const UnitDef* ud; // It's unitDef
	Task Tcurrent;
	bool idle;
	list<Task*> tasks; // Saved task list of tasks yet to eb done.
	int frame; // How many frames old is this unit.
	int creation; // What game frame was this unti created in.
	int BList; // What build tree does this unti have?
	bool factory; // Is this unit a factory?
	bool repeater; // when it finishes its build queue it gets given a new one
	bool guarding; //  when ti finishes its buidl queue it guards another unit
	bool scavenge; // When it finishes its build queue it ressurects thgins and reclaims obstacles and trees
	bool waiting; // Has this unti bene given a task that would be a ludicrous thign to do and is waiting for it to become feasible?

	Command current;
	bool bad;
protected:
	Global* G;
	//cpersona* p;
};

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
// This can be used to cotnrol a group fo untis without iterating
// through each unti in turn.
// Could do with more complete implementation fo extra functions
// and actually be used.

class ugroup{
public:
	ugroup(){}
	ugroup(Global* GLI){
		G = GLI;
	}
	virtual ~ugroup(){}
	//map<int,Tunit*> Getforce(){return umap;}
	void Add(int uid){}
	//map<int,Tunit*> umap;
	int gid;
	int acknum;
	set<int> units;
	bool idle;
protected:
	Global* G;
};


#endif
