#include "../Core/helper.h"

CTaskFactory::CTaskFactory(Global* GL){
	//
	G = GL;
}
CTaskFactory::~CTaskFactory(){
	//
}
void CTaskFactory::Event(CEvent e){
	//
}
CTask* CTaskFactory::GetTask(string TaskType){
	//
	return 0;
}
CTask* CTaskFactory::GetTask(btype TaskType){
	//
	return 0;
}
