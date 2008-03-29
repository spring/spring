#include "include.h"

namespace ntai {

	IModule::IModule(Global* GL){
		G=GL;
		valid=false;
	}

	IModule::~IModule(){
	}

	void IModule::DestroyModule(){
		valid=false;
		//G->RemoveHandler(me);
	}
}
