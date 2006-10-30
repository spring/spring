#ifndef __COMMAND_COLORS_H__
#define __COMMAND_COLORS_H__
// CommandColors.h: interface for the CCommandColors class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)


#include <map>
#include <string>

class CCommandColors {
	public:
		CCommandColors();
		~CCommandColors();

		bool LoadConfig(const std::string& filename);

		// for command queue lines
		bool         AlwaysDrawQueue()   const { return alwaysDrawQueue; }
		bool         UseQueueIcons()     const { return useQueueIcons; }
		float        QueueIconAlpha()    const { return queueIconAlpha; }
		float        QueueIconScale()    const { return queueIconScale; }
		bool         UseColorRestarts()  const { return useColorRestarts; }
		bool         UseRestartColor()   const { return useRestartColor; }
		float        RestartAlpha()      const { return restartAlpha; }

		float        QueuedLineWidth()   const { return queuedLineWidth; }
		unsigned int QueuedBlendSrc()    const { return queuedBlendSrc; }
		unsigned int QueuedBlendDst()    const { return queuedBlendDst; }

		float        SelectedLineWidth() const { return selectedLineWidth; }
		unsigned int SelectedBlendSrc()  const { return selectedBlendSrc; }
		unsigned int SelectedBlendDst()  const { return selectedBlendDst; }
		bool         BuildBoxesOnShift() const { return buildBoxesOnShift; }

		float        MouseBoxLineWidth() const { return mouseBoxLineWidth; }
		unsigned int MouseBoxBlendSrc()  const { return mouseBoxBlendSrc; }
		unsigned int MouseBoxBlendDst()  const { return mouseBoxBlendDst; }

		float        UnitBoxLineWidth()  const { return unitBoxLineWidth; }

    // the colors		
		const float* unitBox;
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
		const float* deathWatch;
		// for selected unit range rendering
		const float* rangeAttack;
		const float* rangeBuild;
		const float* rangeDecloak;
		const float* rangeExtract;
		const float* rangeKamikaze;
		const float* rangeSelfDestruct;

	private:

		std::map<std::string, int> colorNames;

		enum ColorIndices {
			start_index = 0,
			restart_index,
			stop_index,
			wait_index,
			build_index,
			move_index,
			attack_index,
			fight_index,
			guard_index,
			patrol_index,
			capture_index,
			repair_index,
			reclaim_index,
			restore_index,
			resurrect_index,
			load_index,
			unload_index,
			deathWatch_index,
			rangeAttack_index,
			rangeBuild_index,
			rangeDecloak_index,
			rangeExtract_index,
			rangeKamikaze_index,
			rangeSelfDestruct_index,
			unitBox_index,
			mouseBox_index,
			ColorCount
		};

		float colors[ColorCount][4];

		// for command queue lines
		bool alwaysDrawQueue;
		float queueIconAlpha;
		float queueIconScale;
		bool useQueueIcons;
		bool useColorRestarts;
		bool useRestartColor;
		float restartAlpha;

		float queuedLineWidth;
		unsigned int queuedBlendSrc;
		unsigned int queuedBlendDst;

		float selectedLineWidth;
		unsigned int selectedBlendSrc;
		unsigned int selectedBlendDst;
		bool buildBoxesOnShift;

		float mouseBoxLineWidth;
		unsigned int mouseBoxBlendSrc;
		unsigned int mouseBoxBlendDst;

		float unitBoxLineWidth;
};


extern CCommandColors cmdColors;


#endif // __COMMAND_COLORS_H__
