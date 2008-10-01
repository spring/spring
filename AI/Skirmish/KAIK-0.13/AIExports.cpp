#include "AIExport.h"

#include <map>

#include "ExternalAI/Interface/LegacyCppWrapper/AI.h"
#include "ExternalAI/Interface/LegacyCppWrapper/AIGlobalAI.h"
#include "ExternalAI/Interface/SSAILibrary.h"

#include "GlobalAI.h"

/*
std::set<IGlobalAI*> ais;


DLL_EXPORT int GetGlobalAiVersion() {
	return GLOBAL_AI_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name) {
	strcpy(name, AI_VERSION);
}

DLL_EXPORT IGlobalAI* GetNewAI() {
	if (ais.empty())
		creg::System::InitializeClasses();

	CGlobalAI* ai = new CGlobalAI();
	ais.insert(ai);
	return ai;
}

DLL_EXPORT void ReleaseAI(IGlobalAI* i) {
	ais.erase(i);
	delete (CGlobalAI*) i;

	if (ais.empty())
		creg::System::FreeClasses();
}

DLL_EXPORT int IsCInterface(void) {
	return 0;
}

DLL_EXPORT int IsLoadSupported() {
	return 1;
}
*/


std::map<int, CAI*> ais;
std::vector<InfoItem> myInfos;

int getInfos(InfoItem infos[], int max) {
	
	int i = 0;
	
	// initialize the myInfos
	if (myInfos.empty()) {
		InfoItem ii_0 = {SKIRMISH_AI_PROPERTY_SHORT_NAME, "KAIK", NULL}; myInfos.push_back(ii_0);
		InfoItem ii_1 = {SKIRMISH_AI_PROPERTY_VERSION, "0.13", NULL}; myInfos.push_back(ii_1);
		InfoItem ii_2 = {SKIRMISH_AI_PROPERTY_NAME, "Kloots Skirmish AI", NULL}; myInfos.push_back(ii_2);
		InfoItem ii_3 = {SKIRMISH_AI_PROPERTY_DESCRIPTION, "This Skirmish AI supports most TA based mods and plays decently.", NULL}; myInfos.push_back(ii_3);
		InfoItem ii_4 = {SKIRMISH_AI_PROPERTY_URL, "http://spring.clan-sy.com/wiki/AI:KAIK", NULL}; myInfos.push_back(ii_4);
		InfoItem ii_5 = {SKIRMISH_AI_PROPERTY_LOAD_SUPPORTED, "no", NULL}; myInfos.push_back(ii_5);
	}
	
	// copy myInfos to the argument container infos
	std::vector<InfoItem>::const_iterator inf;
	for (inf=myInfos.begin(); inf!=myInfos.end() && i < max; inf++) {
		infos[i] = *inf;
		i++;
	}

	// return the number of key-value pairs copied into properties
	return i;
}

enum LevelOfSupport getLevelOfSupportFor(
		const char* engineVersionString, int engineVersionNumber,
		const char* aiInterfaceShortName, const char* aiInterfaceVersion) {
	
	if (strcmp(engineVersionString, ENGINE_VERSION_STRING) == 0 &&
			engineVersionNumber <= ENGINE_VERSION_NUMBER) {
		return LOS_Working;
	}
	
	return LOS_None;
}

int getOptions(struct Option options[], int max) {
	return 0;
}

// Since this is a C interface, we can only be told by the engine
// to set up an AI with the number team that indicates a receiver
// of any handleEvent() call.
Export(int) init(int teamId) {
    // the map already has an AI for this team.
    // raise an error, since it's probably a mistake if we're trying
    // reinitialise a team that's already had init() called on it.
    if (ais.count(teamId) > 0) {
        return -1;
    }
    //TODO:
    // Change the line below so that CAI is 
    // your AI, which should be a subclass of CAI that
    // overrides the handleEvent() method.
    ais[teamId] = new CAIGlobalAI(teamId, new CGlobalAI());
	
	return 0;
}

Export(int) release(int teamId) {
    // the map has no AI for this team.
    // raise an error, since it's probably a mistake if we're trying to
    // release a team that's not initialized.
    if (ais.count(teamId) == 0) {
        return -1;
    }
	
    delete ais[teamId];
	ais.erase(teamId);
	
	return 0;
}

Export(int) handleEvent(int teamId, int topic, const void* data) {
    // events sent to team -1 will always be to the AI object itself,
    // not to a particular team.
    if (ais.count(teamId) > 0){
        // allow the AI instance to handle the event.
        return ais[teamId]->handleEvent(topic, data);
    }
    // no ai with value, so return error.
    else return -1;
}
