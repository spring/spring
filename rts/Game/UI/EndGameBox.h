/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _END_GAME_BOX_H_
#define _END_GAME_BOX_H_

#include "InputReceiver.h"
#include "Rendering/GL/myGL.h"

#include <vector>


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

	static bool enabled;
	static void Create(const std::vector<unsigned char>& winningAllyTeams) { if (endGameBox == nullptr) new CEndGameBox(winningAllyTeams);}
	static void Destroy() { if (endGameBox != nullptr) { delete endGameBox; endGameBox = nullptr; } }

protected:
	static CEndGameBox* endGameBox;
	void FillTeamStats();

	TRectangle<float> box;

	TRectangle<float>   exitBox;
	TRectangle<float> playerBox;
	TRectangle<float>    sumBox;
	TRectangle<float>    difBox;

	bool moveBox = false;

	int dispMode = 0;

	int stat1 =  1;
	int stat2 = -1;

	struct Stat {
		Stat(const char* s) : name(s), max(1), maxdif(1) {}

		void AddStat(int team, float value) {
			max = std::max(max, value);

			if (team >= 0 && static_cast<size_t>(team) >= values.size())
				values.resize(team + 1);

			if (values[team].size() > 0)
				maxdif = std::max(math::fabs(value - values[team].back()), maxdif);

			values[team].push_back(value);
		}

		const char* name;
		float max;
		float maxdif;

		std::vector< std::vector<float> > values;
	};

	std::vector<unsigned char> winners;
	std::vector<Stat> stats;

	GLuint graphTex = 0;
};

#endif // _END_GAME_BOX_H_
