/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <array>
#include <cstring>

#include "System/bitops.h"
#include "CommandColors.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/Bitmap.h"
#include "MouseCursor.h"
#include "HwMouseCursor.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/UnorderedMap.hpp"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/SimpleParser.h"


// FIXME: "array must be initialized with a brace-enclosed initializer" if constexpr
static const VA_TYPE_TC CURSOR_VERTS[] = {
	VA_TYPE_TC{{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // tl
	VA_TYPE_TC{{0.0f, 1.0f, 0.0f}, 0.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // bl
	VA_TYPE_TC{{1.0f, 1.0f, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // br

	VA_TYPE_TC{{1.0f, 1.0f, 0.0f}, 1.0f, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // br
	VA_TYPE_TC{{1.0f, 0.0f, 0.0f}, 1.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // tr
	VA_TYPE_TC{{0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // tl
};


CMouseCursor::CMouseCursor(const std::string& name, HotSpot hs): hotSpot(hs)
{
	frames.reserve(8);
	images.reserve(8);

	Build(name);

	for (FrameData& frame: frames) {
		frame.startTime = animPeriod;
		frame.endTime = (animPeriod += frame.length);

		xmaxsize = std::max(images[frame.imageIdx].xOrigSize, xmaxsize);
		ymaxsize = std::max(images[frame.imageIdx].yOrigSize, ymaxsize);
	}

	if (hotSpot == Center) {
		xofs = xmaxsize / 2;
		yofs = ymaxsize / 2;
	}
}

CMouseCursor::~CMouseCursor()
{
	if (hwCursor != nullptr) {
		hwCursor->Kill();
		IHardwareCursor::Free(hwCursor);
	}

	for (const auto& image: images) {
		glDeleteTextures(1, &image.texture);
	}
}


CMouseCursor& CMouseCursor::operator = (CMouseCursor&& mc) noexcept {
	images = std::move(mc.images);
	frames = std::move(mc.frames);

	if (mc.hwCursor != nullptr) {
		memcpy(hwCursorMem, mc.hwCursorMem, sizeof(hwCursorMem));
		memset(mc.hwCursorMem, 0, sizeof(hwCursorMem));

		hwCursor = reinterpret_cast<IHardwareCursor*>(hwCursorMem);
		mc.hwCursor = nullptr;
	}

	hotSpot = mc.hotSpot;

	animTime = mc.animTime;
	animPeriod = mc.animPeriod;
	currentFrame = mc.currentFrame;

	xmaxsize = mc.xmaxsize;
	ymaxsize = mc.ymaxsize;

	xofs = mc.xofs;
	yofs = mc.yofs;

	hwValid = mc.hwValid;
	return *this;
}


bool CMouseCursor::Build(const std::string& name)
{
	int lastFrame = 1000;

	hwCursor = IHardwareCursor::Alloc(hwCursorMem);
	hwCursor->Init(hotSpot);

	if (!BuildFromSpecFile(name, lastFrame))
		BuildFromFileNames(name, lastFrame);

	hwCursor->Finish();

	return (hwValid = hwCursor->IsValid(), IsValid());
}

bool CMouseCursor::BuildFromSpecFile(const std::string& name, int& lastFrame)
{
	const std::string specFileName = "anims/" + name + ".txt";

	if (!CFileHandler::FileExists(specFileName, SPRING_VFS_RAW_FIRST))
		return false;

	CFileHandler specFile(specFileName);
	CSimpleParser specParser(specFile);

	const std::array<std::pair<std::string, int>, 3> commandsMap = {{{"frame", 0}, {"hotspot", 1}, {"lastframe", 2}}};
		  spring::unsynced_map<std::string, int>     imageIdxMap;

	for (std::string line = std::move(specParser.GetCleanLine()); !line.empty(); line = std::move(specParser.GetCleanLine())) {
		const std::vector<std::string>& words = specParser.Tokenize(line, 2);
		const std::string& command = StringToLower(words[0]);

		const auto cmdPred = [&](const std::pair<std::string, int>& p) { return (p.first == command); };
		const auto cmdIter = std::find_if(commandsMap.cbegin(), commandsMap.cend(), cmdPred);

		if (cmdIter == commandsMap.end()) {
			LOG_L(L_ERROR, "[MouseCursor::%s] unknown command \"%s\" in file \"%s\"", __func__, command.c_str(), specFileName.c_str());
			continue;
		}

		if ((cmdIter->second == 0) && (words.size() >= 2)) {
			const std::string& imageName = words[1];
			float length = MIN_FRAME_LENGTH;

			if (words.size() >= 3)
				length = std::max(length, float(std::atof(words[2].c_str())));

			const auto imageIdxIt = imageIdxMap.find(imageName);

			if (imageIdxIt != imageIdxMap.end()) {
				frames.emplace_back(imageIdxIt->second, length);
				hwCursor->PushFrame(imageIdxIt->second, length);
				continue;
			}

			ImageData image;

			if (LoadCursorImage(imageName, image)) {
				hwCursor->SetDelay(length);
				imageIdxMap[imageName] = images.size();

				images.push_back(image);
				frames.emplace_back(images.size() - 1, length);
			}

			continue;
		}

		if ((cmdIter->second == 1) && (!words.empty())) {
			if (words[1] == "topleft") {
				hwCursor->SetHotSpot(hotSpot = TopLeft);
				continue;
			}
			if (words[1] == "center") {
				hwCursor->SetHotSpot(hotSpot = Center);
				continue;
			}

			LOG_L(L_ERROR, "[MouseCursor::%s] unknown hotspot \"%s\" in file \"%s\"", __func__, words[1].c_str(), specFileName.c_str());
			continue;
		}

		if ((cmdIter->second == 2) && (!words.empty())) {
			lastFrame = atoi(words[1].c_str());
			continue;
		}
	}

	return (IsValid());
}


bool CMouseCursor::BuildFromFileNames(const std::string& name, int lastFrame)
{
	// find the image file type to use
	const char* ext = "";
	const char* exts[] = {"png", "tga", "bmp"};

	char animFrameStr[512];

	for (auto& e: exts) {
		memset(animFrameStr, 0, sizeof(animFrameStr));
		snprintf(animFrameStr, sizeof(animFrameStr) - 1, "anims/%s_%d.%s", name.c_str(), 0, ext = e);

		if (CFileHandler::FileExists(animFrameStr, SPRING_VFS_RAW_FIRST))
			break;
	}

	while (int(frames.size()) < lastFrame) {
		memset(animFrameStr, 0, sizeof(animFrameStr));
		snprintf(animFrameStr, sizeof(animFrameStr) - 1, "anims/%s_%d.%s", name.c_str(), static_cast<int>(frames.size()), ext);

		ImageData image;

		if (!LoadCursorImage(animFrameStr, image))
			break;

		images.push_back(image);
		frames.emplace_back(images.size() - 1, float(DEF_FRAME_LENGTH));
	}

	return (IsValid());
}

#if 0
bool CMouseCursor::LoadDummyImage()
{
	ImageData id;
	CBitmap bn;

	bn.Alloc(16, 16, 4);
	bn.Fill(SColor(255, 255 * 0, 255, 255));

	id.texture = bn.CreateTexture();
	id.xOrigSize = bn.xsize;
	id.yOrigSize = bn.ysize;
	id.xAlignedSize = bn.xsize;
	id.yAlignedSize = bn.ysize;

	images.emplace_back(id);
	frames.emplace_back(images.size() - 1, float(DEF_FRAME_LENGTH));

	hwCursor->PushImage(bn.xsize, bn.ysize, bn.GetRawMem());
}
#endif

bool CMouseCursor::LoadCursorImage(const std::string& name, ImageData& image)
{
	if (!CFileHandler::FileExists(name, SPRING_VFS_RAW_FIRST))
		return false;

	CBitmap b;
	if (!b.Load(name)) {
		LOG_L(L_ERROR, "[MouseCursor::%s] bad image file \"%s\"", __func__, name.c_str());
		return false;
	}

	// hardcoded bmp transparency mask
	if (FileSystem::GetExtension(name) == "bmp")
		b.SetTransparent(SColor(84, 84, 252));

	if (hwCursor->NeedsYFlip()) {
		// WINDOWS
		b.ReverseYAxis();
		hwCursor->PushImage(b.xsize, b.ysize, b.GetRawMem());
	} else {
		// X11
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

	const FrameData& frame = frames[currentFrame];
	const ImageData& image = images[frame.imageIdx];

	GL::RenderDataBufferTC* buffer = GL::GetRenderBufferTC();
	Shader::IProgramObject* shader = buffer->GetShader();

	const float3 winCoors = {x * 1.0f, (globalRendering->viewSizeY - y) * 1.0f, 0.0f};
	const float2 winScale = {std::max(scale, -scale), 1.0f};
	const float4& matParams = CalcFrameMatrixParams(winCoors, winScale);

	CMatrix44f cursorMat;
	cursorMat.Translate(matParams.x, matParams.y, 0.0f);
	cursorMat.Scale({matParams.z, matParams.w, 1.0f});

	glAttribStatePtr->EnableBlendMask();
	glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, image.texture);

	shader->Enable();
	shader->SetUniformMatrix4x4<float>("u_movi_mat", false, cursorMat);
	shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
	shader->SetUniform("u_alpha_test_ctrl", 0.01f, 1.0f, 0.0f, 0.0f); // test > 0.01
	buffer->SafeAppend(&CURSOR_VERTS[0], sizeof(CURSOR_VERTS) / sizeof(CURSOR_VERTS[0]));
	buffer->Submit(GL_TRIANGLES);
	shader->SetUniform("u_alpha_test_ctrl", 0.0f, 0.0f, 0.0f, 1.0f); // no test
	shader->Disable();
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

	const FrameData& frame = frames[currentFrame];
	const ImageData& image = images[frame.imageIdx];

	glBindTexture(GL_TEXTURE_2D, image.texture);
}

void CMouseCursor::BindHwCursor() const
{
	assert(hwCursor != nullptr);
	hwCursor->Bind();
}


float4 CMouseCursor::CalcFrameMatrixParams(const float3& winCoors, const float2& winScale) const
{
	if (winCoors.z > 1.0f || frames.empty())
		return {0.0f, 0.0f, 0.0f, 0.0f};

	const FrameData& frame = frames[currentFrame];
	const ImageData& image = images[frame.imageIdx];

	const float qis = winScale.x;
	const float  xs = image.xAlignedSize * qis;
	const float  ys = image.yAlignedSize * qis;

	const float rxs = xs * globalRendering->pixelX;
	const float rys = ys * globalRendering->pixelY;

	// center on hotspot
	const float xp = (winCoors.x -                    (xofs * qis )) * globalRendering->pixelX;
	const float yp = (winCoors.y - winScale.y * (ys - (yofs * qis))) * globalRendering->pixelY;

	return {xp, yp, rxs, rys};
}

