/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FONT_TEXTURE_H
#define FONT_TEXTURE_H

#include <string>

namespace FontTexture
{
  void Reset();
  bool Execute();

  bool SetInData        (const std::string& data);
  bool SetInFileName    (const std::string& inFile);
  bool SetOutBaseName   (const std::string& baseName);
  bool SetFontHeight    (unsigned int height);
  bool SetTextureWidth  (unsigned int width);
  bool SetMinChar       (unsigned int minChar);
  bool SetMaxChar       (unsigned int maxChar);
  bool SetOutlineMode   (unsigned int mode);
  bool SetOutlineRadius (unsigned int radius);
  bool SetOutlineWeight (unsigned int weight);
  bool SetPadding       (unsigned int padding);
  bool SetStuffing      (unsigned int stuffing);
  bool SetDebugLevel    (unsigned int debugLevel);
};

#endif // FONT_TEXTURE_H
