/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "LegacyAtlasAlloc.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"


// texture spacing in the atlas (in pixels)
#define TEXMARGIN 2

static const size_t maxysize = 2048;
static const size_t maxxsize = 2048;



inline int CLegacyAtlasAlloc::CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2)
{
	//sort in reverse order
	if ((tex1)->size.y == (tex2)->size.y) {
		return ((tex1)->size.x > (tex2)->size.x);
	}
	return ((tex1)->size.y > (tex2)->size.y);
}


bool CLegacyAtlasAlloc::IncreaseSize()
{
	if (atlasSize.y < atlasSize.x) {
		if (atlasSize.y < maxysize) {
			atlasSize.y *= 2;
			return true;
		}
		if (atlasSize.x < maxxsize) {
			atlasSize.x *= 2;
			return true;
		}
	} else {
		if (atlasSize.x < maxxsize) {
			atlasSize.x *= 2;
			return true;
		}
		if (atlasSize.y < maxysize) {
			atlasSize.y *= 2;
			return true;
		}
	}

	return false;
}


void CLegacyAtlasAlloc::Allocate()
{
	atlasSize.x = 2;
	atlasSize.y = 2;

	std::vector<SAtlasEntry*> memtextures;
	for (std::map<std::string, SAtlasEntry>::iterator it = entries.begin(); it != entries.end(); ++it) {
		memtextures.push_back(&it->second);
	}
	sort(memtextures.begin(), memtextures.end(), CLegacyAtlasAlloc::CompareTex);

	bool success = true;
	int maxx = 0;
	int curx = 0;
	int maxy = 0;
	int cury = 0;
	std::list<int2> nextSub;
	std::list<int2> thisSub;
	bool recalc = false;
	for (int a = 0; a < static_cast<int>(memtextures.size()); ++a) {
		SAtlasEntry* curtex = memtextures[a];

		bool done = false;
		while (!done) {
			if (thisSub.empty()) {
				if (nextSub.empty()) {
					maxx = std::max(maxx, curx);
					cury = maxy;
					maxy += curtex->size.y + TEXMARGIN;
					if (maxy > atlasSize.y) {
						if (IncreaseSize()) {
 							nextSub.clear();
							thisSub.clear();
							cury = maxy = curx = 0;
							recalc = true;
							break;
						} else {
							success = false;
							break;
						}
					}
					thisSub.push_back(int2(0, cury));
				} else {
					thisSub = nextSub;
					nextSub.clear();
				}
			}

			if (thisSub.front().x + curtex->size.x > atlasSize.x) {
				thisSub.clear();
				continue;
			}
			if (thisSub.front().y + curtex->size.y > maxy) {
				thisSub.pop_front();
				continue;
			}

			//ok found space for us
			//FIXME when starting again!
			curtex->texCoords.x = thisSub.front().x;
			curtex->texCoords.y = thisSub.front().y;
			curtex->texCoords.z = thisSub.front().x + curtex->size.x;
			curtex->texCoords.w = thisSub.front().y + curtex->size.y;

			done = true;

			if (thisSub.front().y + curtex->size.y + TEXMARGIN < maxy) {
				nextSub.push_back(int2(thisSub.front().x + TEXMARGIN, thisSub.front().y + curtex->size.y + TEXMARGIN));
			}

			thisSub.front().x += curtex->size.x + TEXMARGIN;
			while (thisSub.size()>1 && thisSub.front().x >= (++thisSub.begin())->x) {
				(++thisSub.begin())->x = thisSub.front().x;
				thisSub.erase(thisSub.begin());
			}

		}
		if (recalc) {
			recalc = false;
			a = -1;
			continue;
		}
	}

/*
	if (globalRendering->supportNPOTs && !debug) {
		maxx = std::max(maxx,curx);
		//atlasSize.x = maxx;
		atlasSize.y = maxy;
	}
*/
	//FIXME
	//return success;
}
