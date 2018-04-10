/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/EventClient.h"
#include "System/EventHandler.h"

/******************************************************************************/
/******************************************************************************/

CEventClient::CEventClient(const std::string& _name, int _order, bool _synced)
	: name(_name)
	, order(_order)
	, synced_(_synced)
	, autoLinkEvents(false)
{
	// note: virtual functions aren't available in the ctor, can't autobind here
	// eventHandler.AddClient() calls CEventClient::WantsEvent() which is virtual
	//RegisterLinkedEvents(this);
}

CEventClient::~CEventClient()
{
	eventHandler.RemoveClient(this);
}


bool CEventClient::WantsEvent(const std::string& eventName)
{
	if (!autoLinkEvents)
		return false;

	assert(!autoLinkedEvents.empty());

	const auto comp = [](const LinkPair& a, const LinkPair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(autoLinkedEvents.begin(), autoLinkedEvents.end(), LinkPair{eventName, false}, comp);

	return (iter != autoLinkedEvents.end() && iter->second && iter->first == eventName);
}


/******************************************************************************/
/******************************************************************************/
//
//  Unsynced
//

void CEventClient::Save(zipFile archive) {}

void CEventClient::Update() {}
void CEventClient::UnsyncedHeightMapUpdate(const SRectangle& rect) {}

void CEventClient::SunChanged() {}

void CEventClient::ViewResize() {}

bool CEventClient::DefaultCommand(const CUnit* unit, const CFeature* feature, int& cmd) { return false; }

void CEventClient::GameProgress(int gameFrame) {}

void CEventClient::DrawLoadScreen() {}
void CEventClient::LoadProgress(const std::string& msg, const bool replace_lastline) {}

// from LuaUI
bool CEventClient::KeyPress(int key, bool isRepeat) { return false; }
bool CEventClient::KeyRelease(int key) { return false; }
bool CEventClient::TextInput(const std::string& utf8) { return false; }
bool CEventClient::TextEditing(const std::string& utf8, unsigned int start, unsigned int length) { return false; }
bool CEventClient::MouseMove(int x, int y, int dx, int dy, int button) { return false; }
bool CEventClient::MousePress(int x, int y, int button) { return false; }
void CEventClient::MouseRelease(int x, int y, int button) { }
bool CEventClient::MouseWheel(bool up, float value) { return false; }

void CEventClient::DownloadQueued(int ID, const string& archiveName, const string& archiveType) {}
void CEventClient::DownloadStarted(int ID) {}
void CEventClient::DownloadFinished(int ID) {}
void CEventClient::DownloadFailed(int ID, int errorID) {}
void CEventClient::DownloadProgress(int ID, long downloaded, long total) {}

bool CEventClient::IsAbove(int x, int y) { return false; }
std::string CEventClient::GetTooltip(int x, int y) { return ""; }

bool CEventClient::CommandNotify(const Command& cmd) { return false; }

bool CEventClient::AddConsoleLine(const std::string& msg, const std::string& section, int level) { return false; }

void CEventClient::LastMessagePosition(const float3& pos) {}

bool CEventClient::GroupChanged(int groupID) { return false; }

bool CEventClient::GameSetup(const std::string& state, bool& ready,
                             const std::vector< std::pair<int, std::string> >& playerStates) { return false; }

std::string CEventClient::WorldTooltip(const CUnit* unit,
                                 const CFeature* feature,
                                 const float3* groundPos) { return ""; }

bool CEventClient::MapDrawCmd(int playerID, int type,
                        const float3* pos0,
                        const float3* pos1,
                        const std::string* label) { return false; }

/******************************************************************************/
/******************************************************************************/
