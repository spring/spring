#ifndef IGROUPAI_H
#define IGROUPAI_H
// IGroupAI.h: interface for the IGroupAI class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#include "aibase.h"
#include "Sim/Units/CommandAI/Command.h"
//#include "Interface/SGAI.h"

class IGroupAICallback;
class IAICallback;

#define AI_INTERFACE_VERSION (14 + AI_INTERFACE_GENERATED_VERSION)

class SPRING_API IGroupAI
{
public:
	virtual void InitAi(IGroupAICallback* callback)=0;
	virtual bool AddUnit(int unit)=0;									//group should return false if it doenst want the unit for some reason
	virtual void RemoveUnit(int unit)=0;								//no way to refuse giving up a unit

	virtual void GiveCommand(Command* c)=0;								//the group is given a command by the player
	virtual const std::vector<CommandDescription>& GetPossibleCommands()=0;	//the ai tells the interface what commands it can take (note that it returns a reference so it must keep the vector in memory itself)
	virtual int GetDefaultCmd(int unitid)=0;							//the default command for the ai given that the mouse pointer hovers above unit unitid (or no unit if unitid=0)

	virtual void CommandFinished(int unit,int type)=0;					//a specific unit has finished a specific command, might be a good idea to give new orders to it

	virtual void Update()=0;											//called once a frame (30 times a second)
	virtual void DrawCommands()=0;										//the place to use the LineDrawer interface functions
	virtual void Load(IGroupAICallback* callback,std::istream *s){};	//load ai from file
	virtual void Save(std::ostream *s){};							//save ai to file

	DECLARE_PURE_VIRTUAL(~IGroupAI())
};

#endif /* IGROUPAI_H */
