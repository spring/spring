/* Author: Tobi Vollebregt */
/* based on original los code in LosHandler.{cpp,h} and RadarHandler.{cpp,h} */

#include "StdAfx.h"
#include "LosMap.h"

#include <algorithm>
#include <cstring>

#include "float3.h"


//////////////////////////////////////////////////////////////////////


void CLosMap::SetSize(int2 newSize)
{
	size = newSize;
	map.clear();
	map.resize(size.x * size.y, 0);
}


void CLosMap::AddMapArea(int2 pos, int radius, int amount)
{
	const int sx = std::max(0, pos.x - radius);
	const int ex = std::min(size.x - 1, pos.x + radius);
	const int sy = std::max(0, pos.y - radius);
	const int ey = std::min(size.y - 1, pos.y + radius);

	const int rr = (radius * radius);

	for (int y = sy; y <= ey; ++y) {
		const int rrx = rr - ((pos.y - y) * (pos.y - y));
		for (int x = sx; x <= ex; ++x) {
			if (((pos.x - x) * (pos.x - x)) <= rrx) {
				map[(y * size.x) + x] += amount;
			}
		}
	}
}


void CLosMap::AddMapSquares(const std::vector<int>& squares, int amount)
{
	std::vector<int>::const_iterator lsi;
	for (lsi = squares.begin(); lsi != squares.end(); ++lsi) {
		map[*lsi] += amount;
	}
}


//////////////////////////////////////////////////////////////////////
namespace {
//////////////////////////////////////////////////////////////////////


#define MAX_LOS_TABLE 110

typedef std::vector<int2> TPoints;
typedef std::vector<int2> LosLine;
typedef std::vector<LosLine> LosTable;


class CLosTables
{
public:
	static const LosTable& GetForLosSize(int losSize) {
		static CLosTables instance;
		const int tablenum = std::min(MAX_LOS_TABLE, losSize);
		return instance.lostables[tablenum - 1];
	}

private:
	std::vector<LosTable> lostables;

	CLosTables();
	int Round(float num);
	void DrawLine(char* PaintTable, int x,int y,int Size);
	LosLine OutputLine(int x,int y,int line);
	void OutputTable(int table);
};


CLosTables::CLosTables()
{
	for (int a = 1; a <= MAX_LOS_TABLE; ++a) {
		OutputTable(a);
	}
}


struct int2_comparer
{
	bool operator () (const int2& a, const int2& b) const
	{
		if (a.x != b.x)
			return a.x < b.x;
		else
			return a.y < b.y;
	}
};


void CLosTables::OutputTable(int Table)
{
	TPoints Points;
	LosTable lostable;

	int Radius = Table;
	char* PaintTable = new char[(Radius+1)*Radius];
	memset(PaintTable, 0 , (Radius+1)*Radius);
	int2 P;

	int x, y, r2;

	P.x = 0;
	P.y = Radius;
	Points.push_back(P);
//  DrawLine(0, Radius, Radius);
	for(float i=Radius; i>=1; i-=0.5f) {
		r2 = (int)(i * i);

		y = (int)i;
		x = 1;
		y = (int) (sqrt((float)r2 - 1) + 0.5f);
		while (x < y) {
			if(!PaintTable[x+y*Radius]) {
				DrawLine(PaintTable, x, y, Radius);
				P.x = x;
				P.y = y;
				Points.push_back(P);
			}
			if(!PaintTable[y+x*Radius]) {
				DrawLine(PaintTable, y, x, Radius);
				P.x = y;
				P.y = x;
				Points.push_back(P);
			}

			x += 1;
			y = (int) (sqrt((float)r2 - x*x) + 0.5f);
		}
		if (x == y) {
			if(!PaintTable[x+y*Radius]) {
				DrawLine(PaintTable, x, y, Radius);
				P.x = x;
				P.y = y;
				Points.push_back(P);
			}
		}
	}

	std::sort(Points.begin(), Points.end(), int2_comparer());

	int Line = 1;
	int Size = Points.size();
	for(int j=0; j<Size; j++) {
		lostable.push_back(OutputLine(Points.back().x, Points.back().y, Line));
		Points.pop_back();
		Line++;
	}

	lostables.push_back(lostable);

	delete[] PaintTable;
}


LosLine CLosTables::OutputLine(int x, int y, int Line)
{
	LosLine losline;

	int x0 = 0;
	int y0 = 0;
	int dx = x;
	int dy = y;

	if (abs(dx) > abs(dy)) {                    // slope <1
		float m = (float) dy / (float) dx;      // compute slope
		float b = y0 - m*x0;
		dx = (dx < 0) ? -1 : 1;
		while (x0 != x) {
			x0 += dx;
			losline.push_back(int2(x0, Round(m*x0 + b)));
		}
	} else if (dy != 0) {                       // slope = 1
		float m = (float) dx / (float) dy;      // compute slope
		float b = x0 - m*y0;
		dy = (dy < 0) ? -1 : 1;
		while (y0 != y) {
			y0 += dy;
			losline.push_back(int2(Round(m*y0 + b), y0));
		}
	}
	return losline;
}


void CLosTables::DrawLine(char* PaintTable, int x, int y, int Size)
{
	int x0 = 0;
	int y0 = 0;
	int dx = x;
	int dy = y;

	if (abs(dx) > abs(dy)) {                    // slope <1
		float m = (float) dy / (float) dx;      // compute slope
		float b = y0 - m*x0;
		dx = (dx < 0) ? -1 : 1;
		while (x0 != x) {
			x0 += dx;
			PaintTable[x0+Round(m*x0 + b)*Size] = 1;
		}
	} else if (dy != 0) {                       // slope = 1
		float m = (float) dx / (float) dy;      // compute slope
		float b = x0 - m*y0;
		dy = (dy < 0) ? -1 : 1;
		while (y0 != y) {
			y0 += dy;
			PaintTable[Round(m*y0 + b)+y0*Size] = 1;
		}
	}
}


int CLosTables::Round(float Num)
{
	if ((Num - (int)Num) < 0.5f)
		return (int)Num;
	else
		return (int)Num+1;
}


//////////////////////////////////////////////////////////////////////
}; // end of anon namespace
//////////////////////////////////////////////////////////////////////


void CLosAlgorithm::LosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	pos.x = std::max(0, std::min(size.x - 1, pos.x));
	pos.y = std::max(0, std::min(size.y - 1, pos.y));

	if ((pos.x - radius < 0) || (pos.x + radius >= size.x) ||
	    (pos.y - radius < 0) || (pos.y + radius >= size.y)) {
		SafeLosAdd(pos, radius, baseHeight, squares);
	} else {
		UnsafeLosAdd(pos, radius, baseHeight, squares);
	}
}


#define MAP_SQUARE(pos) \
	((pos).y * size.x + (pos).x)

#define LOS_ADD(_square, _maxAng) \
	{ \
		const int square = _square; \
		const float dh = heightmap[square] - baseHeight; \
		float ang = (dh + extraHeight) * invR; \
		if(ang > _maxAng) { \
			squares.push_back(square); \
			ang = dh * invR; \
			if(ang > _maxAng) _maxAng = ang; \
		} \
	}


void CLosAlgorithm::UnsafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	const int mapSquare = MAP_SQUARE(pos);
	const LosTable& table = CLosTables::GetForLosSize(radius);

	baseHeight += heightmap[mapSquare];

	size_t neededSpace = squares.size() + 1;
	for(LosTable::const_iterator li = table.begin(); li != table.end(); ++li) {
		neededSpace += li->size() * 4;
	}
	squares.reserve(neededSpace);

	squares.push_back(mapSquare);

	for(LosTable::const_iterator li = table.begin(); li != table.end(); ++li) {
		const LosLine& line = *li;
		float maxAng1 = minMaxAng;
		float maxAng2 = minMaxAng;
		float maxAng3 = minMaxAng;
		float maxAng4 = minMaxAng;
		float r = 1;

		for(LosLine::const_iterator linei = line.begin(); linei != line.end(); ++linei) {
			const float invR = 1.0f / r;

			LOS_ADD(mapSquare + linei->x + linei->y * size.x, maxAng1);
			LOS_ADD(mapSquare - linei->x - linei->y * size.x, maxAng2);
			LOS_ADD(mapSquare - linei->x * size.x + linei->y, maxAng3);
			LOS_ADD(mapSquare + linei->x * size.x - linei->y, maxAng4);

			r++;
		}
	}
}


void CLosAlgorithm::SafeLosAdd(int2 pos, int radius, float baseHeight, std::vector<int>& squares)
{
	const int mapSquare = MAP_SQUARE(pos);
	const LosTable& table = CLosTables::GetForLosSize(radius);

	baseHeight += heightmap[mapSquare];

	squares.push_back(mapSquare);

	for (LosTable::const_iterator li = table.begin(); li != table.end(); ++li) {
		const LosLine& line = *li;
		float maxAng1 = minMaxAng;
		float maxAng2 = minMaxAng;
		float maxAng3 = minMaxAng;
		float maxAng4 = minMaxAng;
		float r = 1;

		for(LosLine::const_iterator linei = line.begin(); linei != line.end(); ++linei) {
			const float invR = 1.0f / r;

			if ((pos.x + linei->x < size.x) && (pos.y + linei->y < size.y)) {
				LOS_ADD(mapSquare + linei->x + linei->y * size.x, maxAng1);
			}
			if ((pos.x - linei->x >= 0) && (pos.y - linei->y >= 0)) {
				LOS_ADD(mapSquare - linei->x - linei->y * size.x, maxAng2);
			}
			if ((pos.x + linei->y < size.x) && (pos.y - linei->x >= 0)) {
				LOS_ADD(mapSquare - linei->x * size.x + linei->y, maxAng3);
			}
			if ((pos.x - linei->y >= 0) && (pos.y + linei->x < size.y)) {
				LOS_ADD(mapSquare + linei->x * size.x - linei->y, maxAng4);
			}

			r++;
		}
	}
}
