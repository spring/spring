#include "helper.h"
// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

using namespace std;

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Global::Global(IGlobalAICallback* callback){
	gcb = callback;
	cb = gcb->GetAICallback();
	ccb = 0;
	ccb = gcb->GetCheatInterface();
    L= new Log(this);
	L->Open();
	if(ccb == 0) L->eprint("cheat interface is not on");
	R = new Register;
	//L->iprint("registered");
	Ct = new ctaunt;
	//L->iprint("taunted");
	try{
		M = new CMetalHandler(cb);
		//L->iprint("cained");
		M->loadState();
	}catch(exception e){
		L->iprint("error cains class crashed");
	}
	//L->iprint("cained and loaded");
	Pl = new Planning;
	//L->iprint("planned");
	//GM = new GridManager(callback);
	//B = new BuildPlacer(this);
	As = new Assigner;
	//L->iprint("assigned");
	Sc = new Scouter;
	//L->iprint("scouted");
	Fa = new Factor;
	//L->iprint("built");
	Ch = new Chaser;
	//L->iprint("chased");
	MGUI = new GUIController(this);
	//L->iprint("plotted");
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

Global::~Global(){
	L->Close();
	string s;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/** unitdef->type
	MetalExtractor
	Transport
	Builder
	Factory
	Bomber
	Fighter
	GroundUnit
	Building
**/

