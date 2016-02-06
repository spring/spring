/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


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
#include "System/Util.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/SimpleParser.h"

using std::string;

//////////////////////////////////////////////////////////////////////
// CMouseCursor Class
//////////////////////////////////////////////////////////////////////

CMouseCursor* CMouseCursor::New(const string &name, HotSpot hs)
{
	CMouseCursor* c = new CMouseCursor(name, hs);
	if (c->frames.empty()) {
		delete c;
		return NULL;
	}
	return c;
}


CMouseCursor* CMouseCursor::GetNullCursor()
{
	static CMouseCursor nullCursor("", Center);
	return &nullCursor;
}


CMouseCursor::CMouseCursor(const string& name, HotSpot hs)
 : animated(false)
 , hwValid(false)
 , animTime(0.0f)
 , animPeriod(0.0f)
 , currentFrame(0)
 , xmaxsize(0)
 , ymaxsize(0)
 , xofs(0)
 , yofs(0)
{
	hwCursor = GetNewHwCursor();
	hwCursor->hotSpot = hs;
	hotSpot = hs;

	if (!BuildFromSpecFile(name))
		BuildFromFileNames(name, 123456);

	if (frames.empty())
		return;

	animated = !frames.empty();
	hwValid  = hwCursor->IsValid();

	for (int f = 0; f < (int)frames.size(); f++) {
		FrameData& frame = frames[f];
		frame.startTime = animPeriod;
		animPeriod += frame.length;
		frame.endTime = animPeriod;
		xmaxsize = std::max(frame.image.xOrigSize, xmaxsize);
		ymaxsize = std::max(frame.image.yOrigSize, ymaxsize);
	}

	if (hotSpot == TopLeft) {
		xofs = 0;
		yofs = 0;
	} else {
		xofs = xmaxsize / 2;
		yofs = ymaxsize / 2;
	}
}


CMouseCursor::~CMouseCursor()
{
	delete hwCursor;

	std::vector<ImageData>::iterator it;
	for (it = images.begin(); it != images.end(); ++it)
		glDeleteTextures(1, &it->texture);
}


bool CMouseCursor::BuildFromSpecFile(const string& name)
{
	const string specFile = "anims/" + name + ".txt";
	CFileHandler specFH(specFile);
	if (!specFH.FileExists()) {
		return false;
	}

	CSimpleParser parser(specFH);
	int lastFrame = 123456789;
	std::map<std::string, int> imageIndexMap;

	while (true) {
		const string line = parser.GetCleanLine();
		if (line.empty()) {
			break;
		}
		const std::vector<std::string> &words = parser.Tokenize(line, 2);
		const std::string command = StringToLower(words[0]);

		if ((command == "frame") && (words.size() >= 2)) {
			const std::string imageName = words[1];
			float length = minFrameLength;
			if (words.size() >= 3)
				length = std::max(length, (float)atof(words[2].c_str()));
			std::map<std::string, int>::iterator iit = imageIndexMap.find(imageName);
			if (iit != imageIndexMap.end()) {
				FrameData frame(images[iit->second], length);
				frames.push_back(frame);
				hwCursor->PushFrame(iit->second,length);
			}
			else {
				ImageData image;
				if (LoadCursorImage(imageName, image)) {
					hwCursor->SetDelay(length);
					imageIndexMap[imageName] = images.size();
					images.push_back(image);
					FrameData frame(image, length);
					frames.push_back(frame);
				}
			}
		}
		else if ((command == "hotspot") && (!words.empty())) {
			if (words[1] == "topleft") {
				hotSpot = TopLeft;
				hwCursor->hotSpot = TopLeft;
			}
			else if (words[1] == "center") {
				hotSpot = Center;
				hwCursor->hotSpot = Center;
			}
			else {
				LOG_L(L_ERROR, "%s: unknown hotspot (%s)",
						specFile.c_str(), words[1].c_str());
			}
		}
		else if ((command == "lastframe") && (!words.empty())) {
			lastFrame = atoi(words[1].c_str());
		}
		else {
			LOG_L(L_ERROR, "%s: unknown command (%s)",
					specFile.c_str(), command.c_str());
		}
	}

	if (frames.empty()) {
		return BuildFromFileNames(name, lastFrame);
	}

	hwCursor->Finish();

	return true;
}


bool CMouseCursor::BuildFromFileNames(const string& name, int lastFrame)
{
	// find the image file type to use
	const char* ext = "";
	const char* exts[] = { "png", "tga", "bmp" };
	const int extCount = sizeof(exts) / sizeof(exts[0]);
	for (int e = 0; e < extCount; e++) {
		ext = exts[e];
		std::ostringstream namebuf;
		namebuf << "anims/" << name << "_0." << ext;
		CFileHandler* f = new CFileHandler(namebuf.str());
		if (f->FileExists()) {
			delete f;
			break;
		}
		delete f;
	}

	while (int(frames.size()) < lastFrame) {
		std::ostringstream namebuf;
		namebuf << "anims/" << name << "_" << frames.size() << "." << ext;
		ImageData image;
		if (!LoadCursorImage(namebuf.str(), image))
			break;
		images.push_back(image);
		FrameData frame(image, defFrameLength);
		frames.push_back(frame);
	}

	hwCursor->Finish();

	return true;
}


bool CMouseCursor::LoadCursorImage(const string& name, ImageData& image)
{
	CFileHandler f(name);
	if (!f.FileExists()) {
		return false;
	}

	CBitmap b;
	if (!b.Load(name)) {
		LOG_L(L_ERROR, "CMouseCursor: Bad image file: %s", name.c_str());
		return false;
	}

	// hardcoded bmp transparency mask
	if (FileSystem::GetExtension(name) == "bmp") {
		b.SetTransparent(SColor(84, 84, 252));
	}

	if (hwCursor->NeedsYFlip()) {
		//WINDOWS
		b.ReverseYAxis();
		hwCursor->PushImage(b.xsize,b.ysize,&b.mem[0]);
	}else{
		//X11
		hwCursor->PushImage(b.xsize,b.ysize,&b.mem[0]);
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
	if (frames.empty()) {
		return;
	}

	if (scale<0) scale=-scale;

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
	glColor4f(1,1,1,1);

	glViewport(xp, globalRendering->viewSizeY - yp, xs, ys);

	static float vertices[] = {0.f, 0.f, 0.f,
				   0.f, 1.f, 0.f,
				   1.f, 1.f, 0.f,
				   1.f, 0.f, 0.f};
	static float texcoords[] = {0.f, 0.f,
				    0.f, 1.f,
				    1.f, 1.f,
				    1.f, 0.f};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glVertexPointer(3, GL_FLOAT, 0, vertices);

	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glViewport(globalRendering->viewPosX, 0, globalRendering->viewSizeX, globalRendering->viewSizeY);
}


void CMouseCursor::DrawQuad(int x, int y) const
{
	if (frames.empty()) {
		return;
	}

	const float scale = cmdColors.QueueIconScale();

	const FrameData& frame = frames[currentFrame];
	const int xs = int(float(frame.image.xAlignedSize) * scale);
	const int ys = int(float(frame.image.yAlignedSize) * scale);

	//Center on hotspot
	const int xp = int(float(x) - (float(xofs) * scale));
	const int yp = int(float(y) - (float(ys) - (float(yofs) * scale)));

	glViewport(globalRendering->viewPosX + xp, yp, xs, ys);

	static float vertices[] = {0.f, 0.f, 0.f,
				   0.f, 1.f, 0.f,
				   1.f, 1.f, 0.f,
				   1.f, 0.f, 0.f};
	static float texcoords[] = {0.f, 0.f,
				    0.f, 1.f,
				    1.f, 1.f,
				    1.f, 0.f};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glVertexPointer(3, GL_FLOAT, 0, vertices);
//FIXME pass va?
	glDrawArrays(GL_QUADS, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}


void CMouseCursor::Update()
{
	if (frames.empty()) {
		return;
	}

	animTime = math::fmod(animTime + globalRendering->lastFrameTime * 0.001f, animPeriod);

	if (animTime < frames[currentFrame].startTime) {
		currentFrame = 0;
		return;
	}

	while (animTime > frames[currentFrame].endTime) {
		currentFrame++;
		if (currentFrame >= int(frames.size())) {
			currentFrame = 0;
		}
	}
}


void CMouseCursor::BindTexture() const
{
	if (frames.empty()) {
		return;
	}

	const FrameData& frame = frames[currentFrame];
	glBindTexture(GL_TEXTURE_2D, frame.image.texture);
}

void CMouseCursor::BindHwCursor() const
{
	hwCursor->Bind();
}
