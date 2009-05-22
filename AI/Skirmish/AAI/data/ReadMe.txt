Author: 	Alexander 		Icq: 138100896					email:	alexander.seizinger
		Seizinger		Jabber: alexander.seizinger@jabber.org				@gmx.net


Installation: 	Windows users:

		Simply extract the archive into your spring\ai\ folder and make sure, the following subfolders exist:
		
		\ai\aai			-> readme, faq and different logs 
		\ai\aai\cache		-> map data (metal spots, water, etc)
		\ai\aai\cfg\map		-> map config files
		\ai\aai\cfg\mod		-> mod config files
		\ai\aai\learn\map	-> map learn files
		\ai\aai\learn\mod	-> mod learn files
		\ai\aai\log		-> log files 

		\ai\bot-libs\aai.dll 	-> main ai dll

		If one of these folders does not exist, AAI will crash!

		
		Linux users: 

		AAI will get the path of a writable directory from spring and load/save its files there
		e.g.  /usr/local/games/taspring 

		Apart from that the structure is the same as for windows users.



		Please note, that there are hardly any buildtables included in this release. AAI should make better 
		unit choices after a while. For further information, have a look at the AAI FAQ.

		Furthermore I want to emphasize that there are quite some opportunities to change/improve AAI's way of
		playing the game. Have a look at the cfg/help/mod.cfg for further information. If you think you created 
		a cfg file which makes AAI play a certain much better than the default one mail it to me to include it 
		in future releases.



License:	Since AAI v0.70 all files have been released under the GPL



Mod Support:	AAI must know the internal names of the starting units of the different sides (e.g. ARMCOM 
		and CORCOM for most TA-mods). There are also some constants you may want to play with, so 
		AAI tries to load the config from a file in the aai subfolder. The name must be the same as 
		the modname, with ".cfg" extension (e.g. xta_se_v066.cfg for XTA mod). If you are not sure 
		about the correct filename, just start a game running that mod. AAI will complain about 
		missing cfg files and put the desired cfg filename in the log files.

		See example.cfg for more information.
		
		By default, AAI has cfg files to support the following mods:
		- XTA
		- TA:WD 5.65
		- AA 2.23 Standard
		- AA 2.23 Forge
		- Balanced AA
		- OTA Shiny 5.5
		- OTA Classic 5.5
		- BoTA 1.3
		- KuroTA 0.47
		- Star Wars 1.0
		- TLL 1.04
		- FF 1.19		
		- Gundam Annihilation 
		- Expand & Exterminate 0.163
		- Spring 1944
		- TA Battle Fleet


		
	!!!	Remember due to updates of these mods the filenames might change as well, so you have	!!!
	!!!	to rename the mod.cfg file yourself							!!!




Known bugs/	- AAI sometimes suffers from a bug in the pathfinding (units sometimes get stuck when close 
Limitations:	  to buildings/objects)

		- AAI does not use nukes, antinukes, emp missiles



Thanks to: 	- Yuritch for testing and providing me with improved mod config files, numerous ideas and suggestions
		  and coding contributions (since AAI v0.83) 

		- Nicklas Marcusson for porting/compiling the linux version of AAI, lots of help with debugging

		- TA Spring devs for creating the best open source rts game I know

		- Jelmer 'JC' Cnossen for some functions and a lot of helpful discussions

		- Krogothe for his mex spot algorithm

		- AccidUK for debugging and some coding suggestions

		- Tournesol and Firenu for helping me getting debugging to work
		
		- All other testers who are not listed here
		
		- All other people who gave some feedback 

		- Brandon Potter for his TBT 12 cfg file


AAI v0.90 	- Completly redone scouting algorithm

		- Enemy units on radar are taken into account now as well

		- AAI will delay non-combat-critial buildtasks & upgrading of buildings when under pressure

		- Redesigned the way attacks are launched. AAI should play more aggressively now, attack poorly defended areas 
		  more often and better coordinate attacks on maps with more than one landmass (e.g. island maps)

		- Improved the factory/builder selection algorithm

		- Improved construction and placement of radar towers

		- Fixed a bug that crashed the game when a mobile factory (= construction unit that may buil other units) was 
		  killed on its way to the buildsite


AAI v0.88	- Current ammount of land/water within the base is taken into account when selecting factories (e.g. dont order 
		  water factories on a land map anymore)

		- Improved AAIs behaviour on water maps with start positions on islands/land masses

		- Improved metal extractor construction algorithm (spots closer to available builder, faster algorithm, 
		  works better if there are land and sea spots available at the same time)

		- Continent file version bumped to 0.87

		- Fixed a crash on shut down introduced in Spring v0.77


AAI v0.87	- Proper detection of amphibious ground units (was causing AAI to freeze on water maps)

		- Prevent AAI from blocking buildqueues for combat units by requesting too many scouts

		- Added proper handling of resurrected units (however AAI does not build resurrectors atm)	

		- Added some unit specific combat behaviour: Units with high ranged weapons  will now try to keep enemies distant (if 
		  their turnrate is not too low)

		- Added some new variables to mod config file:

		  FALLBACK_DIST_RATIO 0.9 - units will try keep enemies at this ratio of their maximum weapons' range

		  MIN_FALLBACK_RANGE 500  - units with lower weapons' range will not try fall back at all

		  MAX_FALLBACK_RANGE 800  - units with higher weapons' range will not try fall back at all   

  		  MIN_FALLBACK_TURNRATE 250 - units with lower turnrate will not try fall back at all	

		- Added submarine defence map -> AAI will now store its defence capabilities vs submarines separately from other 
		  naval units

		- AAI now takes continents into account when sending scouts to positions (e.g. it does not try send ground scouts to 
		  positions on other islands/continents anymore)

		- AAI now takes continents into account when sending builder/assistant builders to positions (e.g. it does not try send 
		  ground builders to positions on other islands/continents anymore (unless they are amphibious))

		- Fixed a bug that caused builders to leave the buildsite during construction 

		- Fixed various freezes that have been caused by AAI flooding the ai interface with thousands of orders per frame

		- Fixed a bug that could prevent AAI from rebuilding destroyed factories

		- Bumped mod learning file version to 0.86 due to some changes in handling of amphibious units


AAI v0.86	- Added NON_AMPHIB_MAX_WATERDEPTH float (default is 15) statement to mod cfg file, specifies the max water depth 
		  non amphibious ground units can cross

		- Added detection of amphibious units 

		- Added new continent detection system: AAI will divide the map into continents marking connected land(water)masses
		  AAI will make a lot of use of this knowledge in future versions
		  -> map cache file version bumped to 0.86

		- Improved base expansion on water maps (e.g. AAI should no longer expand into a small pond on Small Supreme Battlefield
		  rather than the big ocean next to it) 

		- Added COST_MULTIPLIER key word to mod cfg file, it allows to override the costs of a unit, e.g. 
		  COST_MULTIPLIER armzeus 0.5 causes AAI to treat Arm Zeus as if it would cost half as much as it really does
		  This feature has been requested by Argh to porperly deal with unit squads (building a single unit will 
		  provide the player with several independent units)

		- Fixed a stupid bug that prevented AAI from placing static defences according to terrain if map has already 
		  been played before (bug in loading routine of map cache file)  


AAI v0.85	- Added TRANSPORTERS keyword to mod cfg file, works like SCOUTS or ATTACKERS. Note that AAI does not use transporters
		  yet, but it may be used in future times

		- Redone scout selection, AAI may now use cloaked scouts, reduced GROUND/AIR/SEA/HOVER_SCOUT categories to one 
		  SCOUT category 
		
		-> Due to those changes buildtable version has been bumped to 0.85

		- Fixed a bug that prevented AAI from building builders under certain conditions (S44 was suffering from that)

		- AAI will remove sectors where it lost all its buildings from base (may add them again at any time later though)

		- Completly redone defence building placement (AAI will now take terrain into account as well as already existing 
		  defences) It should give much better results now especially on maps that dont have rather flat terrain
		  
		- AAI will take allied structures into account when looking for rally points for its combat units (helps to
		  better prevent AAI from blocking allied bases with its units)

		- Different instances of AAI now share several kind of map information (saves ram space) and do not expand to 
		  sectors occupied by allied AAIs anymore

		- Added DONT_BUILD keyword to mod cfg files, it works like SCOUTS or ATTACKERS, units listed after DONT_BUILD will 
		  not be build by AAI in any case (and will not been taken into account for internal statistics)			

		- Added ATTACKERS keyword to mod cfg file to force AAI to use specified units as combat units despite all other 
		  capabilities of the unit (e.g. buildoptions)  (implemented by Yuritch)

		- Resurrectors now may ressurect corpses instead of reclaiming them (however ressurectors chase newly resurrected 
		  units trying to repair them - needs some work) (implemented by Yuritch)

		- Improved mex and power plant building rules a bit

		- AAI will upgrade the least effective metal extractors first (only important if a mod has more than two metal 
		  extractors with different efficiencies)

		- Fixed a few potential crash bugs


AAI v0.80	- Improved/tweaked/fixed attack behaviour, AAI will try to withdraw when facing much stronger enemy forces

		- Added attack behaviour for air only mods

		- Added support for different reading/writing paths (only important for linux users)

		- Completly redesigned factory/builder management to support mobile factories and other stuff in the future
		  (EE hub system is NOT supported though)

		- Extended mod learning about usage of different unit categories with respect to elapsed game time	

		- Reduced unit categories: LAND/SEA/HOVER/AIR_BUILDER are now in the same category GROUND/SEA_FACTORY 
		  are now in the same category (slight performace increase and nicer code)

		- Improved mex spot selection (to prevent unnecessary long ways for builders), AAI will now also build better 
		  extractors (e.g. moho mines) outside the base (according to safety)

		- Improved placement of stationary defences and tweaked selection of them a bit

		- Cleaned up buildtables, removed some old stuff, mod learning filesize decreased

		- Decreased map-learning filesize by removing stationary artillery learning and some other unnecessary stuff
		  (map-leraning version changed)

		- AAI no longer crashes when one of its units get captured

		- AAI now builds air bases/refuel pads (thx to yuritch for finding the bug)

		- Fixed various issues/bugs that could cause stalling / prevent AAI from building further factories, builders 

		- Fixed a few bugs that could cause erroneus behaviour on water maps as well as a submarine related bug
	
		- Fixed lots of crash bugs

		- Fixed a possible memory leak in AAIBuildTable

		- Increased mod learning file version 



AAI v0.75	- Completly redone attack system: AAI will now attack more elaborately 
		  (attackers move on if are cleared, bombers returning to base when target destroyed, attack groups now 
		  retreat under several circumstances, combat groups are guarded by aa units - however it still tends 
		  to send in streams of attackers - will be adressed in one of the next versions ) Please note that at the 
		  moment the new attack system does not work with air only mods such as FF at all. AAI will build a base 
		  as well as combat units and react to the actions of the player but it will not attack in a proper way
		
		- Added MAX_ATTACKS statement to mod.cfg which determines the max number of independent attack waves at
		  the same time (set to 4 by default)

		- Modified artillery sorting in preparation of artillery support in one of the next versions.
		  Added GROUND_ARTY_RANGE, SEA_ARTY_RANGE and HOVER_ARTY_RANGE statement to mod cfg. These replace the
		  former MOBILE_ARTY_RANGE statement (it's no longer valid, remove from old cfg files)

		- For linux users: AAI will now store its files in the only writable datadirectory automatically (where 
		  spring saves all its other files) 

		- AAI now tries to get a safer rally point if combat units are killed en route 

		- Builders now try to flee when attacked 

		- AAI now takes allied buildings into account when expanding its base (to prevent AAI from building within
 		  the base of someone else)	

		- Improved AAI's building placement at the beginning of the game (buildings will not be spread out 
		  that much anymore to reduce walking time of commander (thx to Accid_UK for the idea - should have been 
		  already implemented in 0.70 but has somehow been commented out)   

		- Tweaked economy/factory/defence building placement and selection

		- Fixed a bug that prevented AAI from building naval power plants

		- Fixed a bug which sometimes caused builders to leave their construction site

		- Fixed a bug that caused AAI to temporarily run out of scouts when requesting several scouts it could 
		  not build at that time 

		- Fixed a bug that caused serious confusion concerning unit speeds (unfortunately mod learning file 
		  version had to be changed)

		- Fixed a very rare crashbug in the building placement algorithm

		- Fixed a possible crash bug in the airforce handling (thx to Nicklas Marcusson for reporting it)



AAI v0.70	- AAI now handles anti air/assault units, bombers and fighters with different groups 
		  (requires a little bit of learning to work properly)

		- Added new category SUBMARINE_ASSAULT to improve AAI's behaviour on water maps 

		- Added MAX_ANTI_AIR_GROUP_SIZE, MAX_NAVAL_GROUP_SIZE, MAX_SUBMARINE_GROUP_SIZE 
		  and MAX_ARTY_GROUP_SIZE (not in use yet) statement to mod cfg	

		- Added FAST_UNITS_RATE and HIGH_RANGE_UNITS_RATE statements to mod.cfg

		- Added different sub-groups for air only mods (light, medium, heavy & super heavy air assault)

		- Added a message being displayed from time to time when AAI has not been loaded succesfully

		- Completly new combat unit selection (in theory, aai should react more dynamically to its 
		  opponent's behaviour - requires some learning to work porperly)

		- Improved/fixed building of stationary defences - only terporary, defence placement will be reworked 
		  within the next versions (MIN_SECTOR_THREAT statement added to mod.cfg)

		- Idle builders will now try to reclaim wreckages/features in/close to the base

		- Improved unit detection a bit (some bogus weapons like mobile jammers in aa will not be considered 
		  being combat units anymore, static mobile units (like dragons claw) will be filtered out as well)		 
		
		- Improved assistance management e.g. factories will now call assisters both based on buildque length 
		  and buildtime of single units (thx to accid_uk for his suggestion)

		- Added MIN_ASSISTANCE_BUILDSPEED statement to mod cfg

		- AAI now takes the position of its base into account when placing stationary defences for 
		  extractors outside of its base

		- New power plant selection (fixed AAI not building pocket fusions in ff) 

		- Fixed a spelling bug when reading cfg files (keyword had been MAX_ASSITANTS instead of MAX_ASSISTANTS)
		  (thx to Acidd_UK for reporting this bug) 

		- Fixed a bug that prevented AAI from upgrading metal extractors

		- Fixed a bug that could cause AAI to freeze when builders get stuck

		- Fixed scout spamming bug 

		- Fixed possible buffer overflow when reading cfg files (thx to FizzWizz for reporting)

		- Fixed a bug that crashed the game on small maps when MAX_MEX_DISTANCE was greater than 
		  map size (e.g. small divide)

		- Fixed a bug that prevented AAI from rebuilding killed builders


AAI v0.63	- AAI now upgrades radars/jammers 
	
		- Added MIN_FACTORIES_FOR_RADAR_JAMMER statement to mod cfg

		- Added support for Expand&Exterminate

		- Added support for mods with buildings as starting units (like AATA)

		- Added mod cfg file for BoTA (thx to yuri)

		- Tweaked defence building selection
	
		- AAI now prefers armed metal extractors when building far away from its main base

		- Fixed some bugs in the BuildTable (buildtable version changed)

		- Fixed a bug causing the buildmap not to be cleared when a building has been destroyed

		- Fixed a bug concerning speed groups

		- Fixed various crashbugs (big thank you to nicke for helping me finding them)


AAI v0.60:	(AAI's folder structure changed as well as all cache/learning files - i heavily recommend deleting old 
		 AAI versions before installing AAI 0.60)
	
		- Experimental water map support, view FAQ for more details

		- Changed buildtable: Hover crafts got their own category (no longer part of ground units) and several
		  code cleanups to optimize speed

		- AAI remembers which usefulness of different assault categories and orders further combat units based on 
		  that (i hope this will especially help adjusting the amount of land, hover and sea units aai uses on a 
		  mixed land/water map) 
	
		- Added support for stationary artillery (e.g. berthas) (added MAX_STAT_ARTY statement to mod cfg)

		- Added support for air repair pads (added MAX_AIR_BASE statement to mod cfg)
		  edit: seems to be broken somehow 

		- Added support for several factories of the same type (added MAX_FACTORIES_PER_TYPE statement to mod cfg)

		- AAI now defends extractors outside its base up to a certain max dinstace with cheap defence buildings
		  (added MAX_MEX_DEFENCE_DISTANCE statement to mod cfg)		

		- Builder selection improved, AAI now uses closest idle builder

		- AAI now sorts combat units into groups according to their max speed 
		  (added UNITS_SPEED_SUBGROUPS statement to mod cfg)

		- Improved Air Force handling (added MAX_AIR_GROUP_SIZE, MIN_AIR_SUPPORT_EFFICIENCY statement to mod cfg)
			
		- Improved/Fixed several mod cfg files

		- Improved ressource management/AAI will now upgrade extractors to better ones

		- Improved defence building selection

		- Fixed a bug that crashed game right at the start on certain maps (eg. Battle Holmes)
		  (thx to HiEnergy for reporting this bug)

		- Fixed a few crashbugs

		- Fixed various smaller issues/bugs

		- Extended memory sharing between multiple instances of AAI and fixed a shared memory related crashbug
		

AAI v0.55:	- AAI is now compatible with the modified ai interface of spring 0.70	
	
		- Extended map learning files/every mod now creates own map learing files

		- AAI tries not to send builders in setors which turned out to be too dangerous

		- Fixed a rounding related crashbug

		- Fixed a bug causing AAI to stop constructing new buidlings

		- Fixed a bug causing AAI to build rows of sensor towers in SW:TA			

		- Fixed several bugs in the energy management causing aai not to build any further power plants


AAI v0.50:	- Completly new ressource management system (will be further improved in future versions)

		- Buildtable will now be shared by all instances of AAI (this mainly reduces aai's memory usage as well as 
		  a slight increase in loading time when running more than one aai-player at the same time) 

		- Improved factory/builder request system/modified buildtable, to be able to add support for AATA	

		- Improved radar/jammer placement

		- AAI now builds metal makers and storage buidlings
	
		- Added support for metal maps 

		- Added support for air units only mods (e.g. Final Frontier)

		- Added a general.cfg holding information that is used for all mods (e.g. allowing users to adjust how much
		  cpu-power AAI takes for scouting)
		
		- Fixed a bug when AAI stopped building after completing a few buildings

		- Fixed a bug that crashed AAI right at the beginning on certain maps (especially metal maps)
		  (thx to IMSabbel for reporting this bug)

		- Fixed a few crashbugs

		- Added mod support: Final Frontier, Gundam Annihilation
	  

AAI v0.40:	- Scouting redone
 
		- Construction units now assist other builders/factories when needed

		- Added support for mods with more than two sides -> cfg files changed, replace with new ones
		
		- Improved building placement (AAI now prevents "diagonal rows"), fixed a building placement 
		  related crashbug as well

		- AAI now uses radar

		- AAI now uses air units

		- Hopefully fixed a bug concerning blocked buildsites


AAI v0.30: 	- AAI now builds defence buildings (will be extended later on)

		- Improved expansion
	
		- All learning/cache files now contain an internal version to provide better compatibility in the future. 
		  if there will be any changes in the future newer file versions will be created automatically

		- Fixed various bugs, AAI should be running much more stable now

		- Switched to krogothe's mex spot algorithm


AAI v0.20: 	- First released version

		- Completely rewritten metal/energy management -> still not working very good

		- Improved selection of more expensive units (prevents AAI from building level 2 
		  units too early (in most cases...))

		- A certain amount of units will be build to counter air units

		- Various minor fixes and code improvements


AAI v0.10: 	(internal version)

		- Works with different mods (see config file section for more information)
		
		- AI builds a little base 

		- AI builds all kinds of units and tries to attack enemy bases

		- AI learns about important locations in a map on its own 
		  (and saves results in a learning-file)








