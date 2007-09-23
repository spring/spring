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

bool CConsoleTask::Init(boost::shared_ptr<IModule> me){
	this->me = &me;
	G->L.iprint(mymessage);
	CMessage message(string("taskfinished"));
	FireEventListener(message);
	return true;
}
