/* This file is part of the Spring engine (GPL v2 or 
later), see LICENSE.html */


#ifndef STREAMING_CONTROLLER_H
#define STREAMING_CONTROLLER_H


#include <iostream>
#include <boost/asio.hpp>
#include <openh264.hpp>

using boost::asio::ip::udp;

class StreamingController {
	public:
		StreamingController(boost::asio::ip::address_v4 ipAdress, unsigned int port, GLint FBOtoStream): 
		socket_(io_service, udp::endpoint(udp::v4(), 13)) ;
		~StreamingController() {
			dismantleEncoder();
			};
			
		void initStream(void); 
		
		void handle_receive(const boost::system::error_code& error,  
							std::size_t );

		void handle_send(boost::shared_ptr<std::string> /*message*/,
      					const boost::system::error_code& /*error*/,
     					std::size_t /*bytes_transferred*/);

		void setFBOBuffer(GLint FBOtoStream);
		void initializeEncoder(void); 
		void encodePicture( int width, int height );

		//Encoder settings
		SEncParamBase param;

		void dismantleEncoder(void);

		   
		//Server Settings
		boost::asio::io_service io_service;
		udp::resolver	resolver();
		udp::socket socket_;
  		udp::endpoint remote_endpoint_;
		udp::iostream stream;

		boost::system::error_code ec;

	private:

		GLint targetFBO = 0;

}

#endif //STREAMING_CONTROLLER_H
