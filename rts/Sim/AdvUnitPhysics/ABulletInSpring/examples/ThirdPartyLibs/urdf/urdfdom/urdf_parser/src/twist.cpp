/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: John Hsu */


#include <urdf/urdfdom_headers/urdf_model/include/urdf_model/twist.h>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <tinyxml.h>
#include <console_bridge/console.h>

namespace urdf{

bool parseTwist(Twist &twist, TiXmlElement* xml)
{
  twist.clear();
  if (xml)
  {
    const char* linear_char = xml->Attribute("linear");
    if (linear_char != NULL)
    {
      try {
        twist.linear.init(linear_char);
      }
      catch (ParseError &e) {
        twist.linear.clear();
        logError("Malformed linear string [%s]: %s", linear_char, e.what());
        return false;
      }
    }

    const char* angular_char = xml->Attribute("angular");
    if (angular_char != NULL)
    {
      try {
        twist.angular.init(angular_char);
      }
      catch (ParseError &e) {
        twist.angular.clear();
        logError("Malformed angular [%s]: %s", angular_char, e.what());
        return false;
      }
    }
  }
  return true;
}

}



