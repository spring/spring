#include "StdAfx.h"
#include "SensorHandler.h"
#include "TdfParser.h"

CSensorHandler* sensorHandler=0;

CSensorHandler::CSensorHandler(void)
{
	TdfParser tdfparser;

	try {
		tdfparser.LoadFile("gamedata/sensors.tdf");
	} catch (content_error) {
		// No need to do anything here, we just continue
		// getting the values from the empty tdfparser.
	}

	tdfparser.GetDef(losMipLevel, "1", "Sensors\\Los\\LosMipLevel");
	//loshandler->losMipLevel = losMipLevel;
	tdfparser.GetDef(airMipLevel, "2", "Sensors\\Los\\AirLosMipLevel");
	//loshandler->airLosMipLevel = airLosMipLevel;

	tdfparser.GetDef(losMul, "1", "Sensors\\Los\\LosMul");
	tdfparser.GetDef(airLosMul, "1", "Sensors\\Los\\AirLosMul");
}

CSensorHandler::~CSensorHandler(void)
{
}
