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

void CTask::Init(Global* GL) {
}
void CTask::Update() {
}
void CTask::UnitIdle() {
}
void CTask::Event(CEvent e) {
}
btype CTask::GetPrimaryType() {
    return B_NA;
}
void CTask::Execute(set<int> units) {
}

CTask::~CTask() {
}
