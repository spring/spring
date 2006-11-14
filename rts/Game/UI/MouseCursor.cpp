#include "StdAfx.h"
#include "MouseCursor.h"
#include "CommandColors.h"
#include "InfoConsole.h"
#include "SimpleParser.h"
#include "FileSystem/FileHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "myMath.h"
#include "bitops.h"
#include "mmgr.h"


static const float minFrameLength = 0.010f;  // seconds
static const float defFrameLength = 0.100f;  // seconds


CMouseCursor* CMouseCursor::New(const string &name, HotSpot hs)
{
	CMouseCursor* c = new CMouseCursor(name, hs);
	if (c->frames.size() <= 0) {
		delete c;
		return NULL;
	}
	return c;	
}


//Would be nice if these were read from a gaf-file instead.
CMouseCursor::CMouseCursor(const string &name, HotSpot hs)
{
	if (!BuildFromSpecFile(name)) {
		BuildFromFileNames(name, 123456);
	}
	
	if (frames.size() <= 0) {
		return;
	}
	
	animTime = 0.0f;
	animPeriod = 0.0f;
	currentFrame = 0;
	xmaxsize = 0;
	ymaxsize = 0;
	
	for (int f = 0; f < (int)frames.size(); f++) {
		FrameData& frame = frames[f];
		frame.startTime = animPeriod;
		animPeriod += frame.length;
		frame.endTime = animPeriod;
		xmaxsize = max(frame.image.xOrigSize, xmaxsize); 
		ymaxsize = max(frame.image.yOrigSize, ymaxsize); 
	}

	if (hs == TopLeft) {	
		xofs = 0; 
		yofs = 0;
	} else {
		xofs = xmaxsize / 2;
		yofs = ymaxsize / 2;
	}
}


CMouseCursor::~CMouseCursor(void)
{
	vector<ImageData>::iterator it;
	for (it = images.begin(); it != images.end(); ++it) {
		glDeleteTextures(1, &it->texture);
	}
}


bool CMouseCursor::BuildFromSpecFile(const string& name)
{
	const string specFile = "anims/" + name + ".txt";
	CFileHandler specFH(specFile);
	if (!specFH.FileExists()) {
		return false;
	}

	SimpleParser::Init();

	int lastFrame = 123456;

	map<string, int> imageIndexMap;
	
	while (true) {
		const string line = SimpleParser::GetCleanLine(specFH);
		if (line.empty()) {
			break;
		}
		const vector<string> words = SimpleParser::Tokenize(line, 2);
		const string command = StringToLower(words[0]);

		if ((command == "frame") && (words.size() >= 2)) {
			const string imageName = words[1];
			const float length = max(minFrameLength, (float)atof(words[2].c_str()));
			map<string, int>::iterator iit = imageIndexMap.find(imageName);
			if (iit != imageIndexMap.end()) {
				FrameData frame(images[iit->second], length);
				frames.push_back(frame);
			}
			else {
				ImageData image;
				if (LoadImage(imageName, image)) {
					imageIndexMap[imageName] = images.size();
					images.push_back(image);
					FrameData frame(image, length);
					frames.push_back(frame);
				}
			}
		}
		else if ((command == "lastframe") && (words.size() >= 1)) {
			lastFrame = atoi(words[1].c_str());
		}
		else {
			logOutput.Print("%s: unknown command  (%s)\n",
			                specFile.c_str(), command.c_str());
		}
	}

	if (frames.size() <= 0) {
		return BuildFromFileNames(name, lastFrame);
	}

	return true;
}


bool CMouseCursor::BuildFromFileNames(const string& name, int lastFrame)
{
	char namebuf[128];
	if (name.size() > (sizeof(namebuf) - 20)) {
		logOutput.Print("CMouseCursor: Long name %s", name.c_str());
		return false;
	}
	
	// find the image file type to use
	const char* ext = "";
	const char* exts[] = { "png", "tga", "bmp" };
	const int extCount = sizeof(exts) / sizeof(exts[0]);
	for (int e = 0; e < extCount; e++) {
		ext = exts[e];
		SNPRINTF(namebuf, sizeof(namebuf), "anims/%s_%d.%s",
		         name.c_str(), 0, ext);
		CFileHandler* f = new CFileHandler(namebuf);
		if (f->FileExists()) {
			delete f;
			break;
		}
		delete f;
	}

	while (frames.size() < lastFrame) {
		SNPRINTF(namebuf, sizeof(namebuf), "anims/%s_%d.%s",
		         name.c_str(), frames.size(), ext);
		ImageData image;
		if (!LoadImage(namebuf, image)) {
			break;
		}
		images.push_back(image);
		FrameData frame(image, defFrameLength);
		frames.push_back(frame);
	}

	return true;	
}


bool CMouseCursor::LoadImage(const string& name, ImageData& image)
{
	CFileHandler* f = new CFileHandler(name);
	if (!f->FileExists()) {
		return false;
	}

	CBitmap b;
	if (!b.Load(name)) {
		logOutput.Print("CMouseCursor: Bad image file: %s", name.c_str());
		return false;
	}

	b.ReverseYAxis();

	CBitmap* final = getAlignedBitmap(b);
	
	// coded bmp transparency mask
	if ((name.size() >= 3) &&
	    (StringToLower(name.substr(name.size() - 3)) == "bmp")) {
		setBitmapTransparency(*final, 84, 84, 252);
	}

	GLuint texID = 0;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
	             final->xsize, final->ysize, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, final->mem);

	image.texture = texID;
	image.xOrigSize = b.xsize;
	image.yOrigSize = b.ysize;
	image.xAlignedSize = final->xsize;
	image.yAlignedSize = final->ysize;

	delete final;

	return true;
}


CBitmap* CMouseCursor::getAlignedBitmap(const CBitmap &orig)
{
	CBitmap *nb;

	const int nx = next_power_of_2(orig.xsize);
	const int ny = next_power_of_2(orig.ysize);

	unsigned char* data = new unsigned char[nx * ny * 4];
	memset(data, 0, nx * ny * 4);

	for (int y = 0; y < orig.ysize; ++y) {
		for (int x = 0; x < orig.xsize; ++x) {
			for (int v = 0; v < 4; ++v) {
				data[((y + (ny-orig.ysize))*nx+x)*4+v] = orig.mem[(y*orig.xsize+x)*4+v];
			}
		}
	}

	nb = new CBitmap(data, nx, ny);
	delete[] data;
	return nb;
}


void CMouseCursor::setBitmapTransparency(CBitmap &bm, int r, int g, int b)
{
	for(int y=0;y<bm.ysize;++y){
		for(int x=0;x<bm.xsize;++x){
			if ((bm.mem[(y*bm.xsize+x)*4 + 0] == r) &&
			    (bm.mem[(y*bm.xsize+x)*4 + 1] == g) &&
			    (bm.mem[(y*bm.xsize+x)*4 + 2] == b)) 
			{
				bm.mem[(y*bm.xsize+x)*4 + 3] = 0;
			}
		}
	}
}


void CMouseCursor::Draw(int x, int y, float scale)
{
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

	glViewport(xp, gu->viewSizeY - yp, xs, ys);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
	glEnd();

	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
}


void CMouseCursor::DrawQuad(int x, int y)
{
	const float scale = cmdColors.QueueIconScale();

	const FrameData& frame = frames[currentFrame];
	const int xs = int(float(frame.image.xAlignedSize) * scale);  
	const int ys = int(float(frame.image.yAlignedSize) * scale);                      

	//Center on hotspot
	const int xp = int(float(x) - (float(xofs) * scale));
	const int yp = int(float(y) - (float(ys) - (float(yofs) * scale)));

	glViewport(xp, yp, xs, ys);

	glBegin(GL_QUADS);
	 	glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
	 	glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 1.0f, 0.0f);
	 	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 0.0f);
	 	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
	glEnd();
}


void CMouseCursor::Update()
{
	if (frames.size() <= 1) {
		return;
	}

	animTime = fmodf(animTime + gu->lastFrameTime, animPeriod);

	if (animTime < frames[currentFrame].startTime) {
		currentFrame = 0;
		return;
	}

	while (animTime > frames[currentFrame].endTime) {
		currentFrame++;
		if (currentFrame >= frames.size()) {
			currentFrame = 0;
		}
	}
}


void CMouseCursor::BindTexture()
{
	const FrameData& frame = frames[currentFrame];
	glBindTexture(GL_TEXTURE_2D, frame.image.texture);
}

