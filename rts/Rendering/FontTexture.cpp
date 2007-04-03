#include "StdAfx.h"
/*******************************************************************************/
/*******************************************************************************/
//
//  file:     FontTexture.cpp
//  author:   Dave Rodgers  (aka: trepan)
//  date:     Apr 01, 2007
//  license:  GPLv2
//  desc:     creates a font texture atlas and spec file (optionally outlined)
// 
/*******************************************************************************/
/*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <string>
#include <vector>
using namespace std;

#include <ft2build.h>
#include FT_FREETYPE_H

#include <IL/il.h>
#include <IL/ilu.h>

#include "FontTexture.h"


// custom types
typedef   signed char  s8;
typedef unsigned char  u8;
typedef   signed short s16;
typedef unsigned short u16;
typedef   signed int   s32;
typedef unsigned int   u32;


/*******************************************************************************/
/*******************************************************************************/

static string inputFile = "";
static string outputFileName = "";

static u32 height = 16;
static u32 outline = 1;
static u32 outlineMode = 0;
static u32 debugLevel = 0;

static u32 xTexSize = 512;
static u32 yTexSize = 1;

static u32 minChar = 32;
static u32 maxChar = 255;

static const u32 padding = 1;

static vector<class Glyph*> glyphs;


/*******************************************************************************/
/*******************************************************************************/
//
//  Prototypes
//

static bool ProcessFace(FT_Face& face, const string& filename, u32 size);
static void PrintFaceInfo(FT_Face& face);
static void PrintGlyphInfo(FT_GlyphSlot& glyph, int g);


/*******************************************************************************/

class Glyph {
  public:
    Glyph(FT_Face& face, int num);
    ~Glyph();
    bool SaveSpecs(FILE* f);
    bool Outline(u32 radius);
  public:
    u32 num;
    ILuint img;
    u32 xsize;
    u32 ysize;
    u8* pixels;
    u32 oxn, oyn, oxp, oyp; // offsets
    u32 txn, tyn, txp, typ; // texcoords
    u32 advance;
    bool valid;
};


/*******************************************************************************/
/*******************************************************************************/

bool FontTexture::SetInFileName(const std::string& inFile)
{
  inputFile = inFile;
  return true;
}


bool FontTexture::SetOutBaseName(const std::string& baseName)
{
  outputFileName = baseName;
  return true;
}


bool FontTexture::SetTextureWidth(unsigned int width)
{
  xTexSize = width;
  return true;
}


bool FontTexture::SetFontHeight(unsigned int _height)
{
  height = _height;
  return true;
}


bool FontTexture::SetMinChar(unsigned int _minChar)
{
  minChar = _minChar;
  return true;
}


bool FontTexture::SetMaxChar(unsigned int _maxChar)
{
  maxChar = _maxChar;
  return true;
}


bool FontTexture::SetOutlineMode(unsigned int mode)
{
  outlineMode = mode;
  return true;
}


bool FontTexture::SetOutlineRadius(unsigned int radius)
{
  outline = radius;
  return true;
}


bool FontTexture::SetDebugLevel(unsigned int _debugLevel)
{
  debugLevel = _debugLevel;
  return true;
}


void FontTexture::Reset()
{
  inputFile = "";
  outputFileName = "";
  xTexSize = 512;
  yTexSize = 1;
  height = 16;
  minChar = 32;
  maxChar = 255;
  outline = 1;
  outlineMode = 0;
  debugLevel = 0;
}


bool FontTexture::Execute()
{
  if (inputFile.empty()) {
    return false;
  }

  if (debugLevel >= 1) {
    printf("fontfile    = %s\n", inputFile.c_str());
    printf("height      = %i\n", height);
    printf("outline     = %i\n", outline);
    printf("outlineMode = %i\n", outlineMode);
  }

  int error;
  FT_Library library; // handle to library
  FT_Face    face;    // handle to face object

  error = FT_Init_FreeType(&library);
  if (error) {
    printf("freetype library init failed: %i\n", error);
    return 1;
  }

  error = FT_New_Face(library, inputFile.c_str(), 0, &face);
  if (error == FT_Err_Unknown_File_Format) {
    printf("bad font file type\n");
    FT_Done_FreeType(library);
    return 1;
  }
  else if (error) {
    printf("unknown font file error: %i\n", error);
    FT_Done_FreeType(library);
    return 1;
  }

  if (face->num_fixed_sizes <= 0) {
    error = FT_Set_Pixel_Sizes(face, 0, height);
    if (error) {
      printf("FT_Set_Pixel_Sizes() error: %i\n", error);
      FT_Done_Face(face);
      FT_Done_FreeType(library);
      return 1;
    }
  } else {
    height = face->available_sizes[0].height;
  }

  ProcessFace(face, inputFile, height);

  for (int g = 0; g < (int)glyphs.size(); g++) {
    delete glyphs[g];
  }
  glyphs.clear();

  FT_Done_Face(face);
  FT_Done_FreeType(library);
  
  return true;
}


/*******************************************************************************/
/*******************************************************************************/

static bool ProcessFace(FT_Face& face, const string& filename, u32 fontHeight)
{
  string imageName;
  string specsName;
  if (!outputFileName.empty()) {
    imageName = outputFileName + ".png";
    specsName = outputFileName + ".fmt";
  }
  else {
    string basename = filename;
    const string::size_type lastOf = filename.find_last_of('.');
    if (lastOf != string::npos) {
      basename = filename.substr(0, lastOf);
      if (debugLevel >= 1) {
        printf("basename = %s\n", basename.c_str());
      }
    }
    char heightText[64];
    sprintf(heightText, "_%i", fontHeight);
    char outlineText[64];
    sprintf(outlineText, "_%i", outline);
    imageName = basename + heightText + ".png";
    specsName = basename + heightText + ".fmt";
  }

  printf("Processing %s @ %i\n", filename.c_str(), fontHeight);
  if (debugLevel >= 1) {
    PrintFaceInfo(face);
  }

  u32 maxPixelXsize = 0;
  u32 maxPixelYsize = 0;

  for (u32 g = minChar; g <= maxChar; g++) {
    Glyph* glyph = new Glyph(face, g);
    if (!glyph->valid) {
      continue;
    }
    glyphs.push_back(glyph);

    if (outline > 0) {
      glyph->Outline(outline);
    }
    if (debugLevel >= 2) {
      PrintGlyphInfo(face->glyph, g);
    }
    if (maxPixelXsize < glyph->xsize) {
      maxPixelXsize = glyph->xsize;
    }
    if (maxPixelYsize < glyph->ysize) {
      maxPixelYsize = glyph->ysize;
    }
  }

  const u32 xskip = maxPixelXsize + (2 * padding);
  const u32 yskip = maxPixelYsize + (2 * padding);
  const u32 xdivs = (xTexSize / xskip);
  const u32 ydivs = ((maxChar - minChar + 1) / xdivs) + 1;

  yTexSize = ydivs * yskip;
  u32 binSize = 1;
  while (binSize < yTexSize) {
    binSize = binSize << 1;
  }
  yTexSize = binSize;
  
  if (debugLevel >= 1) {
    printf("xTexSize = %i\n", xTexSize);
    printf("yTexSize = %i\n", yTexSize);
    printf("xdivs = %i\n", xdivs);
    printf("ydivs = %i\n", ydivs);
    printf("maxPixelXsize = %i\n", maxPixelXsize);
    printf("maxPixelYsize = %i\n", maxPixelYsize);
  }

  FILE* specFile = fopen(specsName.c_str(), "wt");

  u32 yStep;
  if (FT_IS_SCALABLE(face)) {
    yStep = (fontHeight * face->height) / face->units_per_EM;
  } else {
    yStep = (face->height / 64);
    if (yStep == 0) {
      // some fonts do not provide a face->height, so make one up
      yStep = (5 * fontHeight) / 4;
    }
  }

  fprintf(specFile, "#\n");
  fprintf(specFile, "#  Source file: %s\n", filename.c_str());
  fprintf(specFile, "#  Source size: %i\n", fontHeight);
  fprintf(specFile, "#  Font family: %s\n", face->family_name);
  fprintf(specFile, "#  Font  style: %s\n", face->style_name);
  fprintf(specFile, "#\n");
  fprintf(specFile, "\n");
  fprintf(specFile, "yStep:    %i\n", yStep);
  fprintf(specFile, "height:   %i\n", fontHeight);
  fprintf(specFile, "xTexSize: %i\n", xTexSize);
  fprintf(specFile, "yTexSize: %i\n", yTexSize);
  fprintf(specFile, "\n");
  fprintf(specFile, "#\n");
  fprintf(specFile, "#  Coordinates format: -x -y +x +y\n");
  fprintf(specFile, "#\n");

  ILuint img;
  ilGenImages(1, &img);
  if (img == 0) {
    printf("ERROR: ilGenImages() == 0\n");
  }
  ilBindImage(img);

  ilTexImage(xTexSize, yTexSize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
  ilClearColor(1.0f, 1.0f, 1.0f, 0.0f);
  ilClearImage();

  u32 xcell = 0;
  u32 ycell = 0;
  for (int g = 0; g < (int)glyphs.size(); g++) {
    Glyph* glyph = glyphs[g];
    const u32 txOffset = (xcell * xskip) + padding;
    const u32 tyOffset = (ycell * yskip) + padding;
    ilSetPixels(
      txOffset, tyOffset, 0,
      glyph->xsize, glyph->ysize, 1,
      IL_RGBA, IL_UNSIGNED_BYTE, glyph->pixels
    );
    glyph->txn += txOffset;
    glyph->tyn += tyOffset;
    glyph->txp += txOffset;
    glyph->typ += tyOffset;
    glyph->SaveSpecs(specFile);

    if (debugLevel >= 2) {
      PrintGlyphInfo(face->glyph, g);
    }

    xcell++;
    if (xcell >= xdivs) {
      xcell = 0;
      ycell++;
    }
  }

  fclose(specFile);
  printf("Saved: %s\n", specsName.c_str());

  ilEnable(IL_FILE_OVERWRITE);
  ilSaveImage((char*)imageName.c_str());
  ilDisable(IL_FILE_OVERWRITE);
  printf("Saved: %s\n", imageName.c_str());

  ilDeleteImages(1, &img);

  return 0;
}


/*******************************************************************************/
/*******************************************************************************/

Glyph::Glyph(FT_Face& face, int _num) : num(_num), valid(false)
{
  int error = FT_Load_Char(face, num, FT_LOAD_RENDER);
  if (error) {
    printf("FT_Load_Char(%i '%c') error: %i\n", num, num, error);
    return;
  }
  FT_GlyphSlot glyph = face->glyph;

  xsize = glyph->bitmap.width;
  ysize = glyph->bitmap.rows;

  bool realData = true;
  if ((xsize == 0) || (ysize == 0)) {
    xsize = 1;
    ysize = 1;
    realData = false;
  }

  advance = glyph->advance.x / 64;

  // offsets
  oxn = face->glyph->bitmap_left;
  oyp = face->glyph->bitmap_top;
  oxp = oxn + xsize;
  oyn = oyp - ysize;

  // texture coordinates
  txn = 0;
  tyn = 0;
  txp = xsize;
  typ = ysize;

  ilGenImages(1, &img);
  if (img == 0) {
    printf("ERROR: ilGenImages() == 0\n");
  }
  ilBindImage(img);

  u8* rgbaPixels = new u8[xsize * ysize * 4];
  if (realData) {
    for (u32 x = 0; x < xsize; x++) {
      for (u32 y = 0; y < ysize; y++) {
        u8* px = rgbaPixels + (4 * (x + (y * xsize)));
        px[0] = 0xFF;
        px[1] = 0xFF;
        px[2] = 0xFF;
        if (glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
          px[3] = face->glyph->bitmap.buffer[((ysize - y - 1) * xsize) + x] ? 0xFF : 0x00;
        } else {
          px[3] = face->glyph->bitmap.buffer[((ysize - y - 1) * xsize) + x];
        }
      }
    }
  } else {
    rgbaPixels[0] = 0xFF;
    rgbaPixels[1] = 0xFF;
    rgbaPixels[2] = 0xFF;
    rgbaPixels[3] = 0x00;
  }
  ilTexImage(xsize, ysize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
  ilSetData(rgbaPixels); // ilTexImage() eats the memory?
  delete[] rgbaPixels;

  pixels = ilGetData();

  valid = true;
}


Glyph::~Glyph()
{
  ilDeleteImages(1, &img);
}


bool Glyph::SaveSpecs(FILE* f)
{
  fprintf(f, "\n");
  fprintf(f, "#%c#\n", num);
  fprintf(f, "glyph:     %4i\n", num);
  fprintf(f, "advance:   %4i\n", advance);
  fprintf(f, "offsets:   %4i %4i %4i %4i\n", oxn, oyn, oxp, oyp);
  fprintf(f, "texcoords: %4i %4i %4i %4i\n", txn, tyn, txp, typ);
  return true;
}

bool Glyph::Outline(u32 radius)
{
  if (!valid) {
    return false;
  }

  const u32 radPad = 2;
  const u32 tmpRad = (radius + radPad);
  const u32 xSizeTmp = xsize + (2 * tmpRad);
  const u32 ySizeTmp = ysize + (2 * tmpRad);

  ILuint newImg;
  ilGenImages(1, &newImg);
  if (img == 0) {
    printf("ERROR: ilGenImages() == 0\n");
  }
  ilBindImage(newImg);

  // copy the original image, centered
  ilTexImage(xSizeTmp, ySizeTmp, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
  ilClearColor(1.0f, 1.0f, 1.0f, 0.0f);
  ilClearImage();
  ilSetPixels(
    tmpRad, tmpRad, 0,
    xsize, ysize, 1,
    IL_RGBA, IL_UNSIGNED_BYTE, pixels
  );

  // make it black
  iluScaleColours(0.0f, 0.0f, 0.0f);

  // blur the black
  iluBlurGaussian(radius);

  // tweak the outline alpha values
  u8* tmpPixels = ilGetData();
  for (u32 i = 0; i < (xSizeTmp * ySizeTmp); i++) {
    const u32 index = (i * 4) + 3;
    const u32 alpha = tmpPixels[index];
    tmpPixels[index] = (u8)min((u32)0xFF, 3 * alpha / 2);
  }

  // overlay the original white text
  // (could use ilOverlayImage(), but it requires flipping, less flexible)
  for (u32 x = 0; x < xsize; x++) {
    for (u32 y = 0; y < ysize; y++) {
      u32 x2 = x + tmpRad;
      u32 y2 = y + tmpRad;
      u8* oldPx = pixels    + (4 * (x  + (y  * xsize)));
      u8* tmpPx = tmpPixels + (4 * (x2 + (y2 * xSizeTmp)));
      tmpPx[0] = oldPx[3];
      tmpPx[1] = oldPx[3];
      tmpPx[2] = oldPx[3];
      //tmpPx[3] = (tmpPx[3] > oldPx[3]) ? tmpPx[3] : oldPx[3];
      tmpPx[3] = (u8)min((u32)0xFF, (u32)tmpPx[3] + (u32)oldPx[3]);
    }
  }

  // crop the radius padding
  const u32 xSizeNew = xsize + (2 * radius);
  const u32 ySizeNew = ysize + (2 * radius);
  iluCrop(radPad, radPad, 0, xSizeNew, ySizeNew, 1);

  // adjust the parameters
  xsize = xSizeNew;
  ysize = ySizeNew;
  oxn -= radius;
  oyn -= radius;
  oxp += radius;
  oyp += radius;
  txp += (2 * radius);
  typ += (2 * radius);
  pixels = ilGetData();

  // NOTE: advance is not adjusted

  ilDeleteImages(1, &img);
  img = newImg;
  
  return true;
}


/*******************************************************************************/
/*******************************************************************************/
//
//  Debugging routines
//

static void PrintFaceInfo(FT_Face& face)
{
  printf("family name = %s\n", face->family_name);
  printf("style  name = %s\n", face->style_name);
  printf("num_faces   = %i\n", (int)face->num_faces);

  printf("numglyphs    = %i\n", (int)face->num_glyphs);
  printf("fixed sizes  = %i\n", (int)face->num_fixed_sizes);
  printf("units_per_EM = %i\n", (int)face->units_per_EM);
  for (int i = 0; i < face->num_fixed_sizes; i++) {
    printf("  size[%i]\n", i);
    FT_Bitmap_Size bs = face->available_sizes[i];
    printf("    height = %i\n", (int)bs.height);
    printf("    width  = %i\n", (int)bs.width);
    printf("    size   = %i\n", (int)bs.size);
    printf("    x_ppem = %i\n", (int)bs.x_ppem);
    printf("    y_ppem = %i\n", (int)bs.y_ppem);
  }
  printf("face height        = %i\n", (int)face->height);
  printf("max_advance_width  = %i\n", (int)face->max_advance_width);
  printf("max_advance_height = %i\n", (int)face->max_advance_height);
}


static void PrintGlyphInfo(FT_GlyphSlot& glyph, int g)
{
  printf("Glyph: %i  '%c'\n", g, g);
  printf("  bitmap.width         = %i\n", (int)glyph->bitmap.width);
  printf("  bitmap.rows          = %i\n", (int)glyph->bitmap.rows);
  printf("  bitmap.pitch         = %i\n", (int)glyph->bitmap.pitch);
  printf("  bitmap.pixel_mode    = %i\n", (int)glyph->bitmap.pixel_mode);
  printf("  bitmap.palette_mode  = %i\n", (int)glyph->bitmap.palette_mode);
  printf("  bitmap_left          = %i\n", (int)glyph->bitmap_left);
  printf("  bitmap_top           = %i\n", (int)glyph->bitmap_top);
  printf("  advance.x            = %i\n", (int)glyph->advance.x);
  printf("  advance.y            = %i\n", (int)glyph->advance.y);
  FT_Glyph_Metrics metrics = glyph->metrics;
  printf("  metrics.width        = %i\n", (int)metrics.width);
  printf("  metrics.height       = %i\n", (int)metrics.height);
  printf("  metrics.horiBearingX = %i\n", (int)metrics.horiBearingX);
  printf("  metrics.horiBearingY = %i\n", (int)metrics.horiBearingY);
  printf("  metrics.horiAdvance  = %i\n", (int)metrics.horiAdvance);
  printf("  metrics.vertBearingX = %i\n", (int)metrics.vertBearingX);
  printf("  metrics.vertBearingY = %i\n", (int)metrics.vertBearingY);
}


/*******************************************************************************/
/*******************************************************************************/
