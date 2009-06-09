/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
This little tool is useful for testing the OSC Stats sender
of the Spring RTS game engine. See the README file for more info.
*/

#include "rts/lib/oscpack/OscOutboundPacketStream.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#ifdef WIN32
#include <windows.h> // for Sleep()
#else
#include <unistd.h> // for net stuff and usleep()
#endif

#include <stdlib.h>  // for rand()
#include <time.h>    // for time(), for initializing rand
#include <stdio.h>   // for printf()
#include <string.h>  // for strcmp()

static const unsigned int DEFAULT_DESTINATION_PORT = 6447;
static const char* const DEFAULT_DESTINATION_ADDRESS = "127.0.0.1";
static const char* const DEFAULT_TOPIC = "rd";

static const unsigned int OUTPUT_BUFFER_SIZE = 10240;


static inline float randFloat(float min = 0.0, float max = 10000.0)
{
	float rf = rand();

	rf = (float) (min + ((double)rf / RAND_MAX * (max-min)));

	return rf;
}
static inline int randInt(int min = 1, int max = 100)
{
	int ri = rand();

	ri = (int) (min + ((double)ri / RAND_MAX * (max-min)));

	return ri;
}

static void packMidiCSound(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: 4i midi evt\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/midi" )
					<< (int)144
					<< (int)1
					<< (int)75
					<< (int)50
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}

static void packRangeAndDuration(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: range and duration\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/range" )
					<< (float)40.0
					<< (float)500.1415
				<< osc::EndMessage
				<< osc::BeginMessage( "/duration" )
					<< 200
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}

static void packInitDataTitles(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: initial data [titles]\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/spring/info/initial/titles" )
					<< "Engine name"
					<< "Engine version"
					<< "Number of players"
					<< "Some float value"
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}
static void packInitDataValues(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: initial data [values]\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/spring/info/initial/values" )
					<< "spring"
					<< "0.1.abc.xzy"
					<< (int) 8
					<< (float) 0.123456
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}


static void packTeamStatsTitles(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: team stats [titles]\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/spring/stats/team/titles" )
					<< "Team number"

					<< "Metal used"
					<< "Energy used"
					<< "Metal produced"
					<< "Energy produced"

					<< "Metal excess"
					<< "Energy excess"

					<< "Metal received"
					<< "Energy received"

					<< "Metal sent"
					<< "Energy sent"

					<< "Metal stored"
					<< "Energy stored"

					<< "Active Units"
					<< "Units killed"

					<< "Units produced"
					<< "Units died"

					<< "Units received"
					<< "Units sent"
					<< "Units captured"
					<< "Units stolen"

					<< "Damage Dealt"
					<< "Damage Received"
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}
static void packTeamStatsValues(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: team stats [values]\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/spring/stats/team/values" )
					<< (int) randInt(0, 2)

					<< (float) randFloat()
					<< (float) randFloat()
					<< (float) randFloat()
					<< (float) randFloat()

					<< (float) randFloat()
					<< (float) randFloat()

					<< (float) randFloat()
					<< (float) randFloat()

					<< (float) randFloat()
					<< (float) randFloat()

					<< (float) randFloat()
					<< (float) randFloat()

					<< (int) randInt()
					<< (int) randInt()

					<< (int) randInt()
					<< (int) randInt()

					<< (int) randInt()
					<< (int) randInt()
					<< (int) randInt()
					<< (int) randInt()

					<< (float) randFloat()
					<< (float) randFloat()
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}


static void packPlayerStatsTitles(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: player stats [titles]\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/spring/stats/player/titles" )
					<< "Player Name"
					<< "Mouse clicks per minute"
					<< "Mouse movement in pixels per minute"
					<< "Keyboard presses per minute"
					<< "Unit commands per minute"
					<< "Average command size (units affected per command)"
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}
static void packPlayerStatsValues(osc::OutboundPacketStream* oscPacker)
{
	//printf("packing: player stats [values]\n");

	(*oscPacker)
			<< osc::BeginBundleImmediate
				<< osc::BeginMessage( "/spring/stats/player/values" )
					<< (const char*) "hoijui"
					<< (float) randFloat()
					<< (float) randFloat()
					<< (float) randFloat()
					<< (float) randFloat()
					<< (float) randFloat()
				<< osc::EndMessage
			<< osc::EndBundle;

	return;
}

static void printUsage(int argc, const char* const* argv)
{
	printf("usage:\n");
	printf("\t%s [destination-ip-address] [destination-port] [loop-intervall] [topic]\n", argv[0]);
	printf("\n");
	printf("\tdestination-ip-address   default: %s\n", DEFAULT_DESTINATION_ADDRESS);
	printf("\tdestination-port         default: %u\n", DEFAULT_DESTINATION_PORT);
	printf("\tloop-intervall           time in ms between subsequent sends. default: %i, -1 means: do not loop\n", -1);
	printf("\ttopic                    default: %s, possible values: rd, it, iv, tt, tv, pt, pv, mi\n", DEFAULT_TOPIC);
}


int main(int argc, const char* const* argv)
{
	srand((unsigned int) time(NULL));

	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

	const char* dstAddress = DEFAULT_DESTINATION_ADDRESS;
	if (argc > 1) {
		dstAddress = argv[1];
	}

	if (strcmp(dstAddress, "--help") == 0) {
		printUsage(argc, argv);
		return 0;
	}

	unsigned int dstPort = DEFAULT_DESTINATION_PORT;
	if (argc > 2) {
		dstPort = atoi(argv[2]);
	}

	int loopMilliSeconds = -1;
	if (argc > 3) {
		loopMilliSeconds = atoi(argv[3]);
	}

	static const unsigned int topicParamIndex = 4;
	const char* topic = DEFAULT_TOPIC;
	if (argc > 4) {
		topic = argv[4];
	}

	printf("sending to %s:%u, sending every %ims, topic: %s ...\n",
			dstAddress, dstPort, loopMilliSeconds, topic);

	struct sockaddr_in destination;
	destination.sin_family = AF_INET;
	destination.sin_addr.s_addr = inet_addr(dstAddress);
	destination.sin_port = htons(dstPort);

	int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// allow broadcasting
	int broadCastPerm = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, (void*) &broadCastPerm, sizeof(broadCastPerm));
	int flags = 0;

	while (true) {
		if (strcmp(topic, "rd") == 0) {
			packRangeAndDuration(&p);
		} else if (strcmp(topic, "it") == 0) {
			packInitDataTitles(&p);
		} else if (strcmp(topic, "iv") == 0) {
			packInitDataValues(&p);
		} else if (strcmp(topic, "tt") == 0) {
			packTeamStatsTitles(&p);
		} else if (strcmp(topic, "tv") == 0) {
			packTeamStatsValues(&p);
		} else if (strcmp(topic, "pt") == 0) {
			packPlayerStatsTitles(&p);
		} else if (strcmp(topic, "pv") == 0) {
			packPlayerStatsValues(&p);
		} else if (strcmp(topic, "mi") == 0) {
			packMidiCSound(&p);
		} else {
			printf("unknown topic: %s\n", topic);
		}

		if (p.IsReady()) {
			sendto(sock_fd, p.Data(), p.Size(), flags,
					(struct sockaddr*) &destination, sizeof(destination));
			p.Clear();
		}

		if (loopMilliSeconds < 0) {
			break;
		}

#ifdef WIN32
		Sleep(loopMilliSeconds);
#else
		usleep(loopMilliSeconds * 1000);
#endif
	}

	close(sock_fd);

	return 0;
}
