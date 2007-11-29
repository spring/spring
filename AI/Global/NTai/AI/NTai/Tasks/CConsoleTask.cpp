#include "../Core/include.h"

CConsoleTask::CConsoleTask(Global* GL){
	//
	valid = false;
}

CConsoleTask::CConsoleTask(Global* GL, string message){
	valid=true;
	G = GL;
	mymessage=message;
}

void CConsoleTask::RecieveMessage(CMessage &message){
}

bool CConsoleTask::Init(){
	G->L.iprint(mymessage);
	End();
	return true;
}
