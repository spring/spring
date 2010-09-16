/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __END_GAME_BOX_H__
#define __END_GAME_BOX_H__

#include "InputReceiver.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"

#include <vector>


// msvc behaves really strange
#if _MSC_VER
namespace std {
	using ::fabs;
	using ::floor;
}
#endif

class CEndGameBox : public CInputReceiver
{
public:
	CEndGameBox(const std::vector<unsigned char>& winningAllyTeams);
	~CEndGameBox();

	virtual bool MousePress(int x, int y, int button);
	virtual void MouseMove(int x, int y, int dx, int dy, int button);
	virtual void MouseRelease(int x, int y, int button);
	virtual void Draw();
	virtual bool IsAbove(int x, int y);
	virtual std::string GetTooltip(int x, int y);

	static bool disabled;

protected:
	void FillTeamStats();
	ContainerBox box;

	ContainerBox exitBox;

	ContainerBox playerBox;
	ContainerBox sumBox;
	ContainerBox difBox;

	bool moveBox;

	int dispMode;

	int stat1;
	int stat2;

	struct Stat {
		Stat(std::string s) : name(s), max(1), maxdif(1) {}

		void AddStat(int team, float value) {
			if (value > max) {
				max = value;
			}
			if (team >= 0 && static_cast<size_t>(team) >= values.size()) {
				values.resize(team + 1);
			}
			if (values[team].size() > 0 && fabs(value-values[team].back()) > maxdif) {
				maxdif = fabs(value-values[team].back());
			}

			values[team].push_back(value);
		}
		std::string name;
		float max;
		float maxdif;

		std::vector< std::vector<float> > values;
	};

	std::vector<unsigned char> winners;

	std::vector<Stat> stats;
	GLuint graphTex;
	CBitmap bm;
};

#endif // __END_GAME_BOX_H__
