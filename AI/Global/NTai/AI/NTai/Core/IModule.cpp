#include "helper.h"

IModule::IModule(Global* GL){
	G=GL;
	valid=false;
}

IModule::~IModule(){
}

void IModule::AddListener(boost::shared_ptr<IModule> module){
	listeners.insert(module);
}

void IModule::RemoveListener(boost::shared_ptr<IModule> module){
	listeners.erase(module);
}


void IModule::FireEventListener(CMessage &message){
	if(!listeners.empty()){
		for(set<boost::shared_ptr<IModule> >::iterator i = listeners.begin(); i != listeners.end(); ++i){
			(*i)->RecieveMessage(message);
		}
	}
}

void IModule::DestroyModule(){
	valid=false;
	if(!listeners.empty()){
		listeners.erase(listeners.begin(),listeners.end());
		listeners.clear();
	}
	//G->RemoveHandler(me);
}
