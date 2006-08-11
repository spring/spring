#ifndef SENSORHANDLER_H
#define SENSORHANDLER_H

//This class is responsible for reading the sensor.tdf file and parsing relevant data.
class CSensorHandler
{
public:
	CSensorHandler(void);
	~CSensorHandler(void);

	int losMipLevel;	//miplevel for los
	int airMipLevel;	//miplevel to use for airlos

	float losMul;		//units sightdistance will be multiplied with this, for testing purposes
	float airLosMul;	//units airsightdistance will be multiplied with this, for testing purposes
};

extern CSensorHandler* sensorHandler;

#endif
