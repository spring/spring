#ifndef DEMO_READER
#define DEMO_READER

#include "Demo.h"
#include "FileSystem/FileHandler.h"

/**
@brief Utility class for reading demofiles
*/
class CDemoReader : public CDemo
{
public:
	/**
	@brief Open a demofile for reading
	@throw std::runtime_error Demofile not found / header corrupt / outdated
	*/
	CDemoReader(const std::string& filename, float curTime);
	
	/**
	@brief read from demo file
	@return Amount of data read (bytes)
	*/
	unsigned GetData(unsigned char *buf, const unsigned length, float curTime);

	/**
	@brief Wether the demo has reached the end
	@return true when end reached, false when there is still stuff to read
	*/
	bool ReachedEnd() const;

private:
	CFileHandler* playbackDemo;
	float demoTimeOffset;
	float nextDemoRead;
	int bytesRemaining;
	DemoStreamChunkHeader chunkHeader;
};

#endif

