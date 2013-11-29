/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RowAtlasAlloc.h"
#include <vector>
#include "System/bitops.h"

// texture spacing in the atlas (in pixels)
static const int ATLAS_PADDING = 1;


CRowAtlasAlloc::CRowAtlasAlloc()
: nextRowPos(0)
{
	atlasSize.x = 256;
	atlasSize.y = 256;
}


inline int CRowAtlasAlloc::CompareTex(SAtlasEntry* tex1, SAtlasEntry* tex2)
{
	// sort by large to small
	if (tex1->size.y == tex2->size.y) {
		return (tex1->size.x > tex2->size.x);
	}
	return (tex1->size.y > tex2->size.y);
}


void CRowAtlasAlloc::EstimateNeededSize()
{
	int spaceNeeded = 0;
	int spaceFree = atlasSize.x * (atlasSize.y - nextRowPos);
	for (auto it: entries) {
		spaceNeeded += it.second.size.x * it.second.size.y;
	}
	for(auto& row: imageRows) {
		spaceFree += row.height * (atlasSize.x - row.width);
	}

	while (spaceFree < spaceNeeded * 1.2f) {
		if (atlasSize.x>=maxsize.x && atlasSize.y>=maxsize.y) {
			break;
		}

		// Resize texture
		if ((atlasSize.x << 1) <= maxsize.x) {
			spaceFree += atlasSize.x * atlasSize.y;
			atlasSize.x = std::min(maxsize.x, atlasSize.x << 1);
		}
		if ((atlasSize.y << 1) <= maxsize.y) {
			spaceFree += atlasSize.x * atlasSize.y;
			atlasSize.y = std::min(maxsize.y, atlasSize.y << 1);
		}
	}
}


CRowAtlasAlloc::Row* CRowAtlasAlloc::AddRow(int glyphWidth, int glyphHeight)
{
	const int wantedRowHeight = glyphHeight;
	while (atlasSize.y < (nextRowPos + wantedRowHeight)) {
		if (atlasSize.x>=maxsize.x && atlasSize.y>=maxsize.y) {
			//throw texture_size_exception();
			return nullptr;
		}

		// Resize texture
		atlasSize.x = std::min(maxsize.x, atlasSize.x << 1);
		atlasSize.y = std::min(maxsize.y, atlasSize.y << 1);
	}

	Row newrow(nextRowPos, wantedRowHeight);
	nextRowPos += wantedRowHeight;
	imageRows.push_back(newrow);
	return &imageRows.back();
}


bool CRowAtlasAlloc::Allocate()
{
	bool success = true;

	if (npot) {
		// revert the used height clamping at the bottom of this function
		// else for the case when Allocate() is called multiple times, the width would grew faster than height
		// also AddRow() only works with PowerOfTwo values.
		atlasSize.y = next_power_of_2(atlasSize.y);
	}

	// it gives much better results when we resize the available space before starting allocation
	// esp. allocation is more horizontal and so we can clip more free space at bottom
	EstimateNeededSize();

	// sort new entries by height from large to small
	std::vector<SAtlasEntry*> memtextures;
	for (auto it = entries.begin(); it != entries.end(); ++it) {
		memtextures.push_back(&it->second);
	}
	sort(memtextures.begin(), memtextures.end(), CRowAtlasAlloc::CompareTex);

	// find space for them
	for (auto& curtex: memtextures) {
		Row* row = FindRow(curtex->size.x + ATLAS_PADDING, curtex->size.y + ATLAS_PADDING);
		if (!row) {
			success = false;
			continue;
		}

		curtex->texCoords.x1 = row->width;
		curtex->texCoords.y1 = row->position;
		curtex->texCoords.x2 = row->width + curtex->size.x;
		curtex->texCoords.y2 = row->position + curtex->size.y;
		row->width += curtex->size.x + ATLAS_PADDING;
	}

	if (npot) {
		atlasSize.y = nextRowPos;
	} else {
		atlasSize.y = next_power_of_2(nextRowPos);
	}

	return success;
}


CRowAtlasAlloc::Row* CRowAtlasAlloc::FindRow(int glyphWidth, int glyphHeight)
{
	int   best_width = atlasSize.x;
	float best_ratio = 10000.0f;
	Row*  best_row   = nullptr;

	// first try to find a row with similar height
	for(auto& row: imageRows) {
		// Check if there is enough space in this row
		if (glyphWidth > (atlasSize.x - row.width))
			continue;
		if (glyphHeight > row.height)
			continue;

		const float ratio = float(row.height) / glyphHeight;
		if ((ratio < best_ratio) || ((ratio == best_ratio) && (row.width < best_width))) {
			best_width = row.width;
			best_ratio = ratio;
			best_row   = &row;
		}
	}

	if (best_row != nullptr)
		return best_row;

	// no row found create a new one
	return AddRow(glyphWidth, glyphHeight);
}
