/* This file is part of the Spring engine (GPL v2 or 
later), see LICENSE.html */


#ifndef STREAMING_CONTROLLER_H
#define STREAMING_CONTROLLER_H


class StreamingController {
	public:
		boost::asio::io_service io_service;
		udp::iostream stream;

		boost::system::error_code ec;
	protected:


	private:



}
#endif 
