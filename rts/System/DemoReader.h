#ifndef DEMO_READER
#define DEMO_READER

#include "Demo.h"

class CFileHandler;
namespace netcode { class RawPacket; }

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
	@return The data read (or 0 if no data), don't forget to delete it
	*/
	netcode::RawPacket* GetData(float curTime);

	/**
	@brief Wether the demo has reached the end
	@return true when end reached, false when there is still stuff to read
	*/
	bool ReachedEnd() const;

	float GetNextReadTime() const;
	const std::string& GetSetupScript() const
	{
		return setupScript;
	};

private:
	CFileHandler* playbackDemo;
	float demoTimeOffset;
	float nextDemoRead;
	int bytesRemaining;
	DemoStreamChunkHeader chunkHeader;
	std::string setupScript;	// the original, unaltered version from script
};

#endif

