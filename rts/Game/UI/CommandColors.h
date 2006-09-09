#ifndef __COMMAND_COLORS_H__
#define __COMMAND_COLORS_H__
// CommandColors.h: interface for the CCommandColors class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)


class CCommandColors {
	public:
		CCommandColors();
		~CCommandColors();
		
		bool LoadConfig(const std::string& filename);

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
		// for selected unit range rendering
		const float* rangeAttack;
		const float* rangeBuild;
		const float* rangeDecloak;
		const float* rangeExtract;
		const float* rangeKamikaze;
		const float* rangeSelfDestruct;
		
		unsigned int QueuedBlendSrc() const { return queuedBlendSrc; }
		unsigned int QueuedBlendDst() const { return queuedBlendDst; }
		
		unsigned int SelectedBlendSrc() const { return selectedBlendSrc; }
		unsigned int SelectedBlendDst() const { return selectedBlendDst; }
		
		float RestartAlpha()     const { return restartAlpha; }
		bool  UseRestartColor()  const { return useRestartColor; }
		bool  UseColorRestarts() const { return useColorRestarts; }

	private:
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
			rangeAttack_index,
			rangeBuild_index,
			rangeDecloak_index,
			rangeExtract_index,
			rangeKamikaze_index,
			rangeSelfDestruct_index,
			ColorCount
		};

		float colors[ColorCount][4];

		unsigned int queuedBlendSrc;
		unsigned int queuedBlendDst;
		unsigned int selectedBlendSrc;
		unsigned int selectedBlendDst;
		
		// for command queue lines
		float restartAlpha;
		bool useRestartColor;
		bool useColorRestarts;
};


extern CCommandColors cmdColors;


#endif // __COMMAND_COLORS_H__
