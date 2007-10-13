#include "StdAfx.h"
#include "SensorHandler.h"
#include "TdfParser.h"

CR_BIND(CSensorHandler, );

CR_REG_METADATA(CSensorHandler, (
		CR_MEMBER(losMipLevel),
		CR_MEMBER(airMipLevel),
		CR_MEMBER(losMul),
		CR_MEMBER(airLosMul),
		CR_RESERVED(16)
		));


CSensorHandler* sensorHandler;


CSensorHandler::CSensorHandler()
{
	TdfParser tdfparser;

	try {
		tdfparser.LoadFile("gamedata/sensors.tdf");
	} catch (content_error) {
		// No need to do anything here, we just continue
		// getting default values from the empty tdfparser.
	}

	tdfparser.GetDef(losMipLevel, "1", "Sensors\\Los\\LosMipLevel");
	//loshandler->losMipLevel = losMipLevel;
	tdfparser.GetDef(airMipLevel, "2", "Sensors\\Los\\AirLosMipLevel");
	//loshandler->airLosMipLevel = airLosMipLevel;

	// losMipLevel is used as index to readmap->mipHeightmap,
	// so the max value is CReadMap::numHeightMipMaps - 1
	if (losMipLevel < 0 || losMipLevel >= 7)
		throw content_error("Sensors\\Los\\LosMipLevel out of bounds. "
				"The minimum value is 0. The maximum value is 6.");

	// airLosMipLevel doesn't have such restrictions, it's just used in various
	// bitshifts with signed integers
	if (airMipLevel < 0 || airMipLevel > 30)
		throw content_error("Sensors\\Los\\AirLosMipLevel out of bounds. "
				"The minimum value is 0. The maximum value is 30.");

	tdfparser.GetDef(losMul, "1", "Sensors\\Los\\LosMul");
	tdfparser.GetDef(airLosMul, "1", "Sensors\\Los\\AirLosMul");
}


CSensorHandler::~CSensorHandler()
{
}
