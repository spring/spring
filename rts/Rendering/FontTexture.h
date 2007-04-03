/*******************************************************************************/
/*******************************************************************************/
//
//  file:     FontTexture.h
//  author:   Dave Rodgers  (aka: trepan)
//  date:     Apr 01, 2007
//  license:  GPLv2
// 
/*******************************************************************************/
/*******************************************************************************/

#ifndef FONT_TEXTURE_H
#define FONT_TEXTURE_H

#include <string>


namespace FontTexture
{
  void Reset();
  bool Execute();

  bool SetInFileName    (const std::string& inFile);
  bool SetOutBaseName   (const std::string& baseName);
  bool SetFontHeight    (unsigned int height);
  bool SetTextureWidth  (unsigned int width);
  bool SetMinChar       (unsigned int minChar);
  bool SetMaxChar       (unsigned int maxChar);
  bool SetOutlineMode   (unsigned int mode);
  bool SetOutlineRadius (unsigned int radius);
  bool SetPadding       (unsigned int padding);
  bool SetStuffing      (unsigned int stuffing);
  bool SetDebugLevel    (unsigned int debugLevel);
};


#endif // FONT_TEXTURE_H
