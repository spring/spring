/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DEBUG_DRAWER_AI_HDR
#define DEBUG_DRAWER_AI_HDR

#include <list>
#include <map>
#include <vector>

#include "System/Vec2.h"
#include "System/float3.h"

class DebugDrawerAI {
public:
	DebugDrawerAI();
	~DebugDrawerAI();

	static DebugDrawerAI* GetInstance();

	void Draw();

	void SetDraw(bool b) { draw = b; }
	bool GetDraw() const { return draw; }

	void AddGraphPoint(int, int, float, float);
	void DelGraphPoints(int, int, int);
	void SetGraphPos(int, float, float);
	void SetGraphSize(int, float, float);
	void SetGraphLineColor(int, int, const float3&);
	void SetGraphLineLabel(int, int, const std::string&);

	int AddOverlayTexture(int, const float*, int, int);
	void UpdateOverlayTexture(int, int, const float*, int, int, int, int);
	void DelOverlayTexture(int, int);
	void SetOverlayTexturePos(int, int, float, float);
	void SetOverlayTextureSize(int, int, float, float);
	void SetOverlayTextureLabel(int, int, const std::string&);

private:
	struct Graph {
	public:
		Graph(const float3& mins = ZeroVector, const float3& maxs = ZeroVector);
		~Graph() {}
		void Clear();

		void Draw();
		void AddPoint(int, float, float);
		void DelPoints(int, int);

		void SetPos(float x, float y) { pos.x = x; pos.y = y; }
		void SetSize(float w, float h) { size.x = w; size.y = h; }
		void SetColor(int, const float3&);
		void SetLabel(int, const std::string&);

	private:
		struct GraphLine {
		public:
			GraphLine(const float3& mins = ZeroVector, const float3& maxs = ZeroVector):
				lineMin(mins), lineMax(maxs),
				lineWidth(2.0f),
				lineLabelSize(0),
				lineLabelWidth(0.0f),
				lineLabelHeight(0.0f) {
			}

			std::list<float3> lineData;

			float3 lineMin;
			float3 lineMax;

			float3 lineColor;
			float lineWidth;

			std::string lineLabel;
			int lineLabelSize;
			float lineLabelWidth;
			float lineLabelHeight;
		};

		std::map<int, GraphLine> lines;

		float3 pos;
		float3 size;
		float3 scale;    // maxScale - minScale
		float3 minScale;
		float3 maxScale;

		int minLabelSize; float minLabelWidth;
		int maxLabelSize; float maxLabelWidth;
	};

	struct TexSet {
	public:
		TexSet(): curTexHandle(0) {}
		~TexSet() {}
		void Clear();

		void Draw();
		int AddTexture(const float*, int, int);
		void UpdateTexture(int, const float*, int, int, int, int);
		void DelTexture(int);
		void SetTexturePos(int, float, float);
		void SetTextureSize(int, float, float);
		void SetTextureLabel(int, const std::string&);

	private:
		struct Texture {
		public:
			Texture(int, int, const float*);
			~Texture();

			unsigned int GetID() const { return id; }

			int GetWidth() const { return xsize; }
			int GetHeight() const { return ysize; }

			const float3& GetPos() const { return pos; }
			const float3& GetSize() const { return size; }
			const std::string& GetLabel() const { return label; }

			float GetLabelWidth() const { return labelWidth; }
			float GetLabelHeight() const { return labelHeight; }

			void SetPos(const float3& p) { pos = p; }
			void SetSize(const float3& s) { size = s; }
			void SetLabel(const std::string&);

			bool operator < (const Texture& t) const { return (id < t.id); }

		private:
			unsigned int id;

			int xsize;   // in pixels
			int ysize;   // in pixels

			float3 pos;  // in relative screen-space
			float3 size; // in relative screen-space

			std::string label;
			float labelWidth;
			float labelHeight;
		};

		std::map<int, Texture*> textures;
		int curTexHandle;
	};

	std::vector<Graph> graphs;
	std::vector<TexSet> texsets;

	bool draw;
};

#define debugDrawerAI (DebugDrawerAI::GetInstance())

#endif
