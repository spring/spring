/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TUIOHANDLER_H
#define TUIOHANDLER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "SDLTuioClient.h"
#include "lib/tuio/TuioObject.h"
#include "System/myMath.h"

class CInputReceiver;

/* Windowing helpers */
shortint2 toWindowSpace(TUIO::TuioPoint *point);
void clampToWindowSpace(shortint2 &pnt);
bool isInWindowSpace(const shortint2 &pnt);

struct CInputReceiver_comp
{
    size_t operator()(const CInputReceiver *a, const CInputReceiver *b) const
    {
        return a < b ;
    }
};

class CTuioHandler : public TUIO::TuioListener
{

public:
	CTuioHandler();
	~CTuioHandler();

    void addTuioObject(TUIO::TuioObject *tobj);
    void updateTuioObject(TUIO::TuioObject *tobj);
    void removeTuioObject(TUIO::TuioObject *tobj);
    void addTuioCursor(TUIO::TuioCursor *tcur);
    void updateTuioCursor(TUIO::TuioCursor *tcur);
    void removeTuioCursor(TUIO::TuioCursor *tcur);
    void refresh(TUIO::TuioTime ftime);

    void lock();
    void unlock();

private:
	SDLTuioClient* client;
	std::map<int, CInputReceiver*> activeReceivers;
	std::set<CInputReceiver*, CInputReceiver_comp> refreshedReceivers;
};

extern CTuioHandler* tuio;

#endif /* TUIOHANDLER_H */
