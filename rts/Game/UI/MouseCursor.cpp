/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <array>
#include <cstring>

#include "System/bitops.h"
#include "CommandColors.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "MouseCursor.h"
#include "HwMouseCursor.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/UnorderedMap.hpp"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/SimpleParser.h"

CMouseCursor::CMouseCursor()
 : hwCursor(nullptr)

 , animTime(0.0f)
 , animPeriod(0.0f)
 , currentFrame(0)

 , xmaxsize(0)
 , ymaxsize(0)

 , xofs(0)
 , yofs(0)

 , hwValid(false)
{}

CMouseCursor::CMouseCursor(const std::string& name, HotSpot hs)
{
	// default-initialize
	*this = CMouseCursor();

	hwCursor = GetNewHwCursor();
	hwCursor->hotSpot = (hotSpot = hs);

	if (!BuildFromSpecFile(name))
		BuildFromFileNames(name, 123456);

	if (frames.empty())
		return;

	hwValid = hwCursor->IsValid();

	for (FrameData& frame: frames) {
		frame.startTime = animPeriod;
		frame.endTime = (animPeriod += frame.length);
		xmaxsize = std::max(frame.image.xOrigSize, xmaxsize);
		ymaxsize = std::max(frame.image.yOrigSize, ymaxsize);
	}

	if (hotSpot == Center) {
		xofs = xmaxsize / 2;
		yofs = ymaxsize / 2;
	}
}


CMouseCursor::~CMouseCursor()
{
	spring::SafeDelete(hwCursor);

	for (auto it = images.begin(); it != images.end(); ++it)
		glDeleteTextures(1, &it->texture);
}


bool CMouseCursor::BuildFromSpecFile(const std::string& name)
{
	assert(hwCursor != nullptr);

	const std::string specFile = "anims/" + name + ".txt";
	CFileHandler specFH(specFile);
	if (!specFH.FileExists())
		return false;

	CSimpleParser parser(specFH);
	int lastFrame = 123456789;

	const std::array<std::pair<std::string, int>, 3> commandsMap = {{{"frame", 0}, {"hotspot", 1}, {"lastframe", 2}}};
	      spring::unsynced_map<std::string, int>     imageIdxMap;

	while (true) {
		const std::string& line = parser.GetCleanLine();
		if (line.empty())
			break;

		const std::vector<std::string>& words = parser.Tokenize(line, 2);
		const std::string& command = StringToLower(words[0]);

		const auto cmdPred = [&](const std::pair<std::string, int>& p) { return (p.first == command); };
		const auto cmdIter = std::find_if(commandsMap.cbegin(), commandsMap.cend(), cmdPred);

		if (cmdIter == commandsMap.end()) {
			LOG_L(L_ERROR, "[MouseCursor::%s] unknown command \"%s\" in file \"%s\"", __func__, command.c_str(), specFile.c_str());
			continue;
		}

		if ((cmdIter->second == 0) && (words.size() >= 2)) {
			const std::string& imageName = words[1];
			float length = minFrameLength;

			if (words.size() >= 3)
				length = std::max(length, float(std::atof(words[2].c_str())));

			const auto imageIdxIt = imageIdxMap.find(imageName);

			if (imageIdxIt != imageIdxMap.end()) {
				frames.emplace_back(images[imageIdxIt->second], length);
				hwCursor->PushFrame(imageIdxIt->second, length);
				continue;
			}

			ImageData image;

			if (LoadCursorImage(imageName, image)) {
				hwCursor->SetDelay(length);
				imageIdxMap[imageName] = images.size();

				images.push_back(image);
				frames.emplace_back(image, length);
			}

			continue;
		}

		if ((cmdIter->second == 1) && (!words.empty())) {
			if (words[1] == "topleft") {
				hwCursor->hotSpot = (hotSpot = TopLeft);
				continue;
			}
			if (words[1] == "center") {
				hwCursor->hotSpot = (hotSpot = Center);
				continue;
			}

			LOG_L(L_ERROR, "[MouseCursor::%s] unknown hotspot \"%s\" in file \"%s\"", __func__, words[1].c_str(), specFile.c_str());
			continue;
		}

		if ((cmdIter->second == 2) && (!words.empty())) {
			lastFrame = atoi(words[1].c_str());
			continue;
		}
	}

	if (frames.empty())
		return BuildFromFileNames(name, lastFrame);

	hwCursor->Finish();
	return true;
}


bool CMouseCursor::BuildFromFileNames(const std::string& name, int lastFrame)
{
	// find the image file type to use
	const char* ext = "";
	const char* exts[] = {"png", "tga", "bmp"};
	const int extCount = sizeof(exts) / sizeof(exts[0]);

	for (int e = 0; e < extCount; e++) {
		ext = exts[e];

		if (CFileHandler::FileExists("anims/" + name + "_0." + std::string(ext), SPRING_VFS_RAW_FIRST))
			break;
	}

	while (int(frames.size()) < lastFrame) {
		ImageData image;
		if (!LoadCursorImage("anims/" + name + "_" + IntToString(frames.size()) + "." + std::string(ext), image))
			break;

		images.push_back(image);
		frames.emplace_back(image, defFrameLength);
	}

	hwCursor->Finish();
	return true;
}


bool CMouseCursor::LoadCursorImage(const std::string& name, ImageData& image)
{
	if (!CFileHandler::FileExists(name, SPRING_VFS_RAW_FIRST))
		return false;

	CBitmap b;
	if (!b.Load(name)) {
		LOG_L(L_ERROR, "CMouseCursor: Bad image file: %s", name.c_str());
		return false;
	}

	// hardcoded bmp transparency mask
	if (FileSystem::GetExtension(name) == "bmp")
		b.SetTransparent(SColor(84, 84, 252));

	if (hwCursor->NeedsYFlip()) {
		//WINDOWS
		b.ReverseYAxis();
		hwCursor->PushImage(b.xsize, b.ysize, b.GetRawMem());
	} else {
		//X11
		hwCursor->PushImage(b.xsize, b.ysize, b.GetRawMem());
		b.ReverseYAxis();
	}


	const int nx = next_power_of_2(b.xsize);
	const int ny = next_power_of_2(b.ysize);

	if (b.xsize != nx || b.ysize != ny) {
		CBitmap bn;
		bn.Alloc(nx, ny);
		bn.CopySubImage(b, 0, ny - b.ysize);

		image.texture = bn.CreateTexture();
		image.xOrigSize = b.xsize;
		image.yOrigSize = b.ysize;
		image.xAlignedSize = bn.xsize;
		image.yAlignedSize = bn.ysize;
	} else {
		image.texture = b.CreateTexture();
		image.xOrigSize = b.xsize;
		image.yOrigSize = b.ysize;
		image.xAlignedSize = b.xsize;
		image.yAlignedSize = b.ysize;
	}

	return true;
}


void CMouseCursor::Draw(int x, int y, float scale) const
{
	if (frames.empty())
		return;

	scale = std::max(scale, -scale);

	const FrameData& frame = frames[currentFrame];
	const int xs = int(float(frame.image.xAlignedSize) * scale);
	const int ys = int(float(frame.image.yAlignedSize) * scale);

	//Center on hotspot
	const int xp = int(float(x) - (float(xofs) * scale));
	const int yp = int(float(y) + (float(ys) - (float(yofs) * scale)));

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, frame.image.texture);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.01f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glViewport(xp, globalRendering->viewSizeY - yp, xs, ys);

	static constexpr float vertices[] = {0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f};
	static constexpr float texcoors[] = {0.0f, 0.0f,         0.0f, 1.0f,         1.0f, 1.0f,         1.0f, 0.0f      };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texcoors);
	glVertexPointer(3, GL_FLOAT, 0, vertices);

	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
}


void CMouseCursor::DrawQuad(int x, int y) const
{
	if (frames.empty())
		return;

	const float scale = cmdColors.QueueIconScale();

	const FrameData& frame = frames[currentFrame];
	const int xs = int(float(frame.image.xAlignedSize) * scale);
	const int ys = int(float(frame.image.yAlignedSize) * scale);

	//Center on hotspot
	const int xp = int(float(x) - (float(xofs) * scale));
	const int yp = int(float(y) - (float(ys) - (float(yofs) * scale)));

	glViewport(globalRendering->viewPosX + xp, yp, xs, ys);

	static constexpr float vertices[] = {0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f};
	static constexpr float texcoors[] = {0.0f, 0.0f,          0.0f, 1.0f,         1.0f, 1.0f,         1.0f, 0.0f      };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texcoors);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
//FIXME pass va?
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void CMouseCursor::Update()
{
	if (frames.empty())
		return;

	animTime = math::fmod(animTime + globalRendering->lastFrameTime * 0.001f, animPeriod);

	if (animTime < frames[currentFrame].startTime) {
		currentFrame = 0;
		return;
	}

	while (animTime > frames[currentFrame].endTime) {
		currentFrame += 1;
		currentFrame %= frames.size();
	}
}


void CMouseCursor::BindTexture() const
{
	if (frames.empty())
		return;

	glBindTexture(GL_TEXTURE_2D, frames[currentFrame].image.texture);
}

void CMouseCursor::BindHwCursor() const
{
	assert(hwCursor != nullptr);
	hwCursor->Bind();
}

