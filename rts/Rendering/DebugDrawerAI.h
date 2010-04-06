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

private:
	struct Graph {
	public:
		Graph(const float3& mins = ZeroVector, const float3& maxs = ZeroVector);
		~Graph() {}

		void Draw();
		void AddPoint(int, float, float);
		void DelPoints(int, int);
		void Clear();

		void SetPos(float x, float y) { pos.x = x; pos.y = y; }
		void SetSize(float w, float h) { size.x = w; size.y = h; }
		void SetColor(int, const float3&);
		void SetLabel(int, const std::string&);

	private:
		struct GraphLine {
		public:
			GraphLine(const float3& mins = ZeroVector, const float3& maxs = ZeroVector):
				lineMin(mins), lineMax(maxs), lineWidth(2.0f) {}

			std::list<float3> lineData;

			float3 lineMin;
			float3 lineMax;

			float3 lineColor;
			float lineWidth;

			std::string lineLabel;
		};

		std::map<int, GraphLine> lines;

		float3 pos;
		float3 size;
		float3 scale;    // maxScale - minScale
		float3 minScale;
		float3 maxScale;
	};

	std::vector<Graph> graphs;
	bool draw;
};

#define debugDrawerAI (DebugDrawerAI::GetInstance())

#endif
