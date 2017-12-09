/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COMMAND_COLORS_H
#define _COMMAND_COLORS_H

#include <string>
#include "System/UnorderedMap.hpp"

class CCommandColors {
public:
	CCommandColors();

	bool LoadConfigFromFile(const std::string& filename);
	bool LoadConfigFromString(const std::string& cfg);

	// for command queue lines
	bool         AlwaysDrawQueue()   const { return alwaysDrawQueue;   }
	bool         UseQueueIcons()     const { return useQueueIcons;     }
	float        QueueIconAlpha()    const { return queueIconAlpha;    }
	float        QueueIconScale()    const { return queueIconScale;    }
	bool         UseColorRestarts()  const { return useColorRestarts;  }
	bool         UseRestartColor()   const { return useRestartColor;   }
	float        RestartAlpha()      const { return restartAlpha;      }

	float        QueuedLineWidth()   const { return queuedLineWidth;   }
	unsigned int QueuedBlendSrc()    const { return queuedBlendSrc;    }
	unsigned int QueuedBlendDst()    const { return queuedBlendDst;    }
	unsigned int StipplePattern()    const { return stipplePattern;    }
	unsigned int StippleFactor()     const { return stippleFactor;     }
	float        StippleSpeed()      const { return stippleSpeed;      }

	float        SelectedLineWidth() const { return selectedLineWidth; }
	unsigned int SelectedBlendSrc()  const { return selectedBlendSrc;  }
	unsigned int SelectedBlendDst()  const { return selectedBlendDst;  }
	bool         BuildBoxesOnShift() const { return buildBoxesOnShift; }

	float        MouseBoxLineWidth() const { return mouseBoxLineWidth; }
	unsigned int MouseBoxBlendSrc()  const { return mouseBoxBlendSrc;  }
	unsigned int MouseBoxBlendDst()  const { return mouseBoxBlendDst;  }

	float        UnitBoxLineWidth()  const { return unitBoxLineWidth;  }

	// custom command queue rendering
	struct DrawData {
		int cmdIconID = 0;
		float color[4];
		bool showArea = false;

		DrawData()
		{
			for (int i = 0; i < 4; ++i) { color[i] = 1.0f; }
		}

		DrawData(int cii, const float c[4], bool area) : cmdIconID(cii), showArea(area)
		{
			for (int i = 0; i < 4; ++i) { color[i] = c[i]; }
		}
	};

	void SetCustomCmdData(int cmdID, int cmdIconID, const float color[4], bool showArea) { customCmds[cmdID] = DrawData(cmdIconID, color, showArea); }
	void ClearCustomCmdData(int cmdID) { customCmds.erase(cmdID); }

	/// get custom command line parameters
	/// @return NULL if no line defined, a pointer to a DrawData otherwise
	/// NB: do not call {Set,Clear}CustomCmdData while holding this pointer
	const DrawData* GetCustomCmdData(int cmdID) const {
		const auto it = customCmds.find(cmdID);

		if (it == customCmds.end())
			return nullptr;

		return &(it->second);
	}

	// the colors
	const float* unitBox;
	const float* buildBox;
	const float* allyBuildBox;
	const float* mouseBox;

	// for command queue rendering
	const float* start;
	const float* restart;
	const float* stop;
	const float* wait;
	const float* build;
	const float* move;
	const float* attack;
	const float* fight;
	const float* guard;
	const float* patrol;
	const float* capture;
	const float* repair;
	const float* reclaim;
	const float* restore;
	const float* resurrect;
	const float* load;
	const float* unload;
	const float* deathWait;
	const float* customArea;

	// for selected unit range rendering
	const float* rangeAttack;
	const float* rangeBuild;
	const float* rangeRadar;
	const float* rangeSonar;
	const float* rangeSeismic;
	const float* rangeJammer;
	const float* rangeSonarJammer;
	const float* rangeShield;
	const float* rangeDecloak;
	const float* rangeExtract;
	const float* rangeKamikaze;
	const float* rangeSelfDestruct;
	const float* rangeInterceptorOn;
	const float* rangeInterceptorOff;

private:
	enum ColorIndices {
		startIndex = 0,
		restartIndex,
		stopIndex,
		waitIndex,
		buildIndex,
		moveIndex,
		attackIndex,
		fightIndex,
		guardIndex,
		patrolIndex,
		captureIndex,
		repairIndex,
		reclaimIndex,
		restoreIndex,
		resurrectIndex,
		loadIndex,
		unloadIndex,
		deathWaitIndex,
		customAreaIndex,

		rangeAttackIndex,
		rangeBuildIndex,
		rangeRadarIndex,
		rangeSonarIndex,
		rangeSeismicIndex,
		rangeJammerIndex,
		rangeSonarJammerIndex,
		rangeShieldIndex,
		rangeDecloakIndex,
		rangeExtractIndex,
		rangeKamikazeIndex,
		rangeSelfDestructIndex,
		rangeInterceptorOnIndex,
		rangeInterceptorOffIndex,

		unitBoxIndex,
		buildBoxIndex,
		allyBuildBoxIndex,
		mouseBoxIndex,

		ColorCount
	};

	float colors[ColorCount][4];

	// for command queue lines
	bool alwaysDrawQueue   = false;
	bool  useQueueIcons    = true;
	bool  useColorRestarts = true;
	bool  useRestartColor  = true;
	bool buildBoxesOnShift = true;

	float queueIconAlpha = 0.5f;
	float queueIconScale = 1.0f;
	float   restartAlpha = 0.25f;

	float   queuedLineWidth = 1.49f;
	float selectedLineWidth = 1.49f;
	float mouseBoxLineWidth = 1.49f;
	float  unitBoxLineWidth = 1.49f;
	float      stippleSpeed = 1.0f;

	unsigned int stipplePattern = 0xffffffff;
	unsigned int stippleFactor = 1;

	unsigned int queuedBlendSrc;
	unsigned int queuedBlendDst;

	unsigned int selectedBlendSrc;
	unsigned int selectedBlendDst;

	unsigned int mouseBoxBlendSrc;
	unsigned int mouseBoxBlendDst;

	spring::unordered_map<std::string, int> colorNames;
	spring::unordered_map<int, DrawData> customCmds;
};


extern CCommandColors cmdColors;


#endif // _COMMAND_COLORS_H
