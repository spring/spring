#ifndef SENSORHANDLER_H
#define SENSORHANDLER_H

/// This class is responsible for reading the gamedata/sensors.tdf file and parsing relevant data.
class CSensorHandler
{
	NO_COPY(CSensorHandler);
	CR_DECLARE(CSensorHandler);

public:
	CSensorHandler();
	~CSensorHandler();

	/// miplevel for los
	int losMipLevel;
	/// miplevel to use for airlos
	int airMipLevel;

	/// units sightdistance will be multiplied with this, for testing purposes
	float losMul;
	/// units airsightdistance will be multiplied with this, for testing purposes
	float airLosMul;
};

extern CSensorHandler* sensorHandler;

#endif
