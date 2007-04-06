/*******************************************************************************/
/*******************************************************************************/
//
//  file:     main.cpp
//  author:   Dave Rodgers  (aka: trepan)
//  date:     Apr 01, 2007
//  license:  GPLv2
//  desc:     application front-end for FontTexture.cpp
// 
/*******************************************************************************/
/*******************************************************************************/

#include <stdio.h>
#include <string>

#include <IL/il.h>
#include <IL/ilu.h>

#include "FontTexture.h"


static bool ParseArgs(char** argv);


/*******************************************************************************/
/*******************************************************************************/

int main(int /* argc */, char** argv)
{
  FontTexture::Reset();

  if (!ParseArgs(argv)) {
    printf("\n");
    printf("Usage: %s [options] <fontfile>\n", argv[0]);
    printf("       -h  <height>   : font height in pixels (scaled fonts)\n");
    printf("       -os            : use separate outlines\n");
    printf("       -or <radius>   : outline radius in pixels\n");
    printf("       -ow <weight>   : outline weight (100 nominal)\n");
    printf("       -f  <outName>  : set the output base name\n");
    printf("       -w  <width>    : set the texture width\n");
    printf("       -sc <minChar>  : set first character\n");
    printf("       -ec <maxChar>  : set last  character\n");
    printf("       -p  <padding>  : set the glyph padding\n");
    printf("       -s  <stuffing> : set the glyph stuffing\n");
    printf("       -d  <debug>    : set the debug level\n");
    printf("\n");
    return 1;
  }

  ilInit();
  iluInit();
  
  FontTexture::Execute();

  ilShutDown();

  return 0;
}


/*******************************************************************************/

static bool ParseArgs(char** argv)
{
  argv++;

  while (argv[0] != NULL) {
    if (argv[0][0] != '-') {
      break;
    }

    const char* opt = argv[0] + 1;

    if (strcmp (opt, "?") == 0) {
      return false;
    }
    else if (strcmp (opt, "h") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetFontHeight(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "os") == 0) {
      FontTexture::SetOutlineMode(1);
    }
    else if (strcmp (opt, "or") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetOutlineRadius(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "ow") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetOutlineWeight(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "f") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetOutBaseName(argv[1]);
      argv++;
    }
    else if (strcmp (opt, "w") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetTextureWidth(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "sc") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetMinChar(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "ec") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetMaxChar(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "p") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetPadding(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "s") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetStuffing(atoi(argv[1]));
      argv++;
    }
    else if (strcmp (opt, "d") == 0) {
      if (argv[1] == NULL) { return false; } // missing parameter
      FontTexture::SetDebugLevel(atoi(argv[1]));
      argv++;
    }
    else {
      return false;
    }

    argv++;
  }

  if (argv[0] == NULL) {
    return false; // fontfile is required
  }

  FontTexture::SetInFileName(argv[0]);
  return true;
}


/*******************************************************************************/
/*******************************************************************************/
