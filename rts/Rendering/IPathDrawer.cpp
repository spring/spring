#include "IPathDrawer.h"
#include "DefaultPathDrawer.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/Default/PathManager.h"

IPathDrawer* pathDrawer = NULL;

IPathDrawer* IPathDrawer::GetInstance() {
	static IPathDrawer* pd = NULL;

	if (pd == NULL) {
		if (dynamic_cast<CPathManager*>(pathManager) != NULL) {
			pd = new DefaultPathDrawer();
		} else {
			pd = new IPathDrawer();
		}
	}

	return pd;
}

void IPathDrawer::FreeInstance(IPathDrawer* pd) {
	delete pd;
}
