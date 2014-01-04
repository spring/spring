/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LegacyAtlasAlloc.h"
#include <vector>
#include <list>


// texture spacing in the atlas (in pixels)
#define TEXMARGIN 2


inline int CLegacyAtlasAlloc::CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2)
{
	// sort in reverse order
	if ((tex1)->size.y == (tex2)->size.y) {
		return ((tex1)->size.x > (tex2)->size.x);
	}
	return ((tex1)->size.y > (tex2)->size.y);
}


bool CLegacyAtlasAlloc::IncreaseSize()
{
	if (atlasSize.y < atlasSize.x) {
		if ((atlasSize.y * 2) <= maxsize.y) {
			atlasSize.y *= 2;
			return true;
		}
		if ((atlasSize.x * 2) <= maxsize.x) {
			atlasSize.x *= 2;
			return true;
		}
	} else {
		if ((atlasSize.x * 2) <= maxsize.x) {
			atlasSize.x *= 2;
			return true;
		}
		if ((atlasSize.y * 2) <= maxsize.y) {
			atlasSize.y *= 2;
			return true;
		}
	}

	return false;
}


bool CLegacyAtlasAlloc::Allocate()
{
	atlasSize.x = 32;
	atlasSize.y = 32;

	std::vector<SAtlasEntry*> memtextures;
	for (std::map<std::string, SAtlasEntry>::iterator it = entries.begin(); it != entries.end(); ++it) {
		memtextures.push_back(&it->second);
	}
	sort(memtextures.begin(), memtextures.end(), CLegacyAtlasAlloc::CompareTex);

	bool success = true;
	int2 max;
	int2 cur;
	std::list<int2> nextSub;
	std::list<int2> thisSub;
	bool recalc = false;
	for (int a = 0; a < static_cast<int>(memtextures.size()); ++a) {
		SAtlasEntry* curtex = memtextures[a];

		bool done = false;
		while (!done) {
			if (thisSub.empty()) {
				if (nextSub.empty()) {
					cur.y = max.y;
					max.y += curtex->size.y + TEXMARGIN;

					if (max.y > atlasSize.y) {
						if (IncreaseSize()) {
							nextSub.clear();
							thisSub.clear();
							cur.y = max.y = cur.x = 0;
							recalc = true;
							break;
						} else {
							success = false;
							break;
						}
					}
					thisSub.push_back(int2(0, cur.y));
				} else {
					thisSub = nextSub;
					nextSub.clear();
				}
			}

			if ((thisSub.front().x + curtex->size.x + TEXMARGIN) > atlasSize.x) {
				thisSub.clear();
				continue;
			}
			if (thisSub.front().y + curtex->size.y > max.y) {
				thisSub.pop_front();
				continue;
			}

			// found space in both dimensions s.t. texture
			// PLUS margin fits within current atlas bounds
			curtex->texCoords.x1 = thisSub.front().x;
			curtex->texCoords.y1 = thisSub.front().y;
			curtex->texCoords.x2 = thisSub.front().x + curtex->size.x - 1;
			curtex->texCoords.y2 = thisSub.front().y + curtex->size.y - 1;

			cur.x = thisSub.front().x + curtex->size.x + TEXMARGIN;
			max.x = std::max(max.x, cur.x);

			done = true;

			if ((thisSub.front().y + curtex->size.y + TEXMARGIN) < max.y) {
				nextSub.push_back(int2(thisSub.front().x + TEXMARGIN, thisSub.front().y + curtex->size.y + TEXMARGIN));
			}

			thisSub.front().x += (curtex->size.x + TEXMARGIN);

			while (thisSub.size()>1 && thisSub.front().x >= (++thisSub.begin())->x) {
				(++thisSub.begin())->x = thisSub.front().x;
				thisSub.erase(thisSub.begin());
			}
		}

		if (recalc) {
			// reset all existing texcoords
			for (std::vector<SAtlasEntry*>::iterator it = memtextures.begin(); it != memtextures.end(); ++it) {
				(*it)->texCoords = float4();
			}
			recalc = false;
			a = -1;
			continue;
		}
	}

	if (npot) {
		atlasSize = max;
	}

	return success;
}
