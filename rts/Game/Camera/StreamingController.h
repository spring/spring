/* This file is part of the Spring engine (GPL v2 or 
later), see LICENSE.html */


#ifndef STREAMING_CONTROLLER_H
#define STREAMING_CONTROLLER_H


#include <iostream>
#include <boost/asio.hpp>

class StreamingController {
	public:
		StreamingController(inet ipAdress, unsigned int port) : socket_(io_service, udp::endpoint(udp::v4(), 13));
		~StreamingController() {
			Dismantle();
			};
			
			
		void initStream(void); 
		void encodePicture(unsigned char* bufferPtr, int width, int height );
		void dismantle(void);
			
		//Dataset
		SEncParamBase param;
		boost::asio::io_service io_service;
		udp::resolver			resolver();
		   
		udp::iostream stream;

		boost::system::error_code ec;
	protected:


	private:



}

#endif //STREAMING_CONTROLLER_H
