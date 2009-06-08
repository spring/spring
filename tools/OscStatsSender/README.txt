This little tool is useful for testing the OSC Stats sender of the Spring RTS
game engine. It is able to send the same OSC messages the engine sends to a
specific IPv4 address and port, with random data where applicable. This is
useful when testing an OSC message reciever (eg a packet pre-analyzer or a
music generator).
For questions, please consider asking in the forum at http://springrts.com.

compile:
	make

help:
	./OscStatsSender.bin --help

