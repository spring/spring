/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _COMMAND_COLORS_H
#define _COMMAND_COLORS_H

#include <map>
#include <string>

class CCommandColors {
	public:
		CCommandColors();
		~CCommandColors();

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
		struct DrawData;
		void SetCustomCmdData(int cmdID, int cmdIconID,
		                      const float color[4], bool showArea);
		void ClearCustomCmdData(int cmdID);
		/// get custom command line parameters
		/// @return NULL if no line defined, a pointer to a DrawData otherwise
		const DrawData *GetCustomCmdData(int cmdID) const;

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

	public:
		struct DrawData {
			int cmdIconID;
			float color[4];
			bool showArea;

			DrawData()
			: cmdIconID(0), showArea(false)
			{
				for (int i = 0; i<4; ++i) { color[i] = 1.0f; }
			}

			DrawData(int cii, const float c[4], bool area)
			: cmdIconID(cii), showArea(area)
			{
				for (int i = 0; i<4; ++i) { color[i] = c[i]; }
			}
		};

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
			deathWait_index,
			rangeAttack_index,
			rangeBuild_index,
			rangeRadar_index,
			rangeSonar_index,
			rangeSeismic_index,
			rangeJammer_index,
			rangeSonarJammer_index,
			rangeShield_index,
			rangeDecloak_index,
			rangeExtract_index,
			rangeKamikaze_index,
			rangeSelfDestruct_index,
			rangeInterceptorOn_index,
			rangeInterceptorOff_index,
			unitBox_index,
			buildBox_index,
			allyBuildBox_index,
			mouseBox_index,
			ColorCount
		};

		float colors[ColorCount][4];

		// for command queue lines
		bool  alwaysDrawQueue;
		float queueIconAlpha;
		float queueIconScale;
		bool  useQueueIcons;
		bool  useColorRestarts;
		bool  useRestartColor;
		float restartAlpha;

		float        queuedLineWidth;
		unsigned int queuedBlendSrc;
		unsigned int queuedBlendDst;
		unsigned int stipplePattern;
		unsigned int stippleFactor;
		float        stippleSpeed;

		float        selectedLineWidth;
		unsigned int selectedBlendSrc;
		unsigned int selectedBlendDst;
		bool         buildBoxesOnShift;

		float        mouseBoxLineWidth;
		unsigned int mouseBoxBlendSrc;
		unsigned int mouseBoxBlendDst;

		float unitBoxLineWidth;

		typedef std::map<int, DrawData> customCmds_type;
		customCmds_type customCmds;
};


extern CCommandColors cmdColors;


#endif // _COMMAND_COLORS_H
