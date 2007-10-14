#include "../Core/helper.h"
// Tasks
#include "../Tasks/CConsoleTask.h"

CConsoleTask::CConsoleTask(Global* GL){}

CConsoleTask::CConsoleTask(Global* GL, string message){
	valid=true;
	G = GL;
	mymessage=message;
}

void CConsoleTask::RecieveMessage(CMessage &message){
	if(message.GetType() == string("killme")){
		//G->RemoveHandler(me);
	}
}

bool CConsoleTask::Init(){
	G->L.iprint(mymessage);
	End();
	return true;
}
