// The universal build tags, this helps define what a task is aka what it does. Very important
enum btype {
	B_POWER,
	B_MEX,
	B_RAND_ASSAULT,
	B_ASSAULT,
	B_FACTORY_CHEAP,
	B_FACTORY,
	B_BUILDER,
	B_GEO,
	B_SCOUT,
	B_RANDOM,
	B_DEFENCE,
	B_RADAR,
	B_ESTORE,
	B_TARG,
	B_MSTORE,
	B_SILO,
	B_JAMMER,
	B_SONAR,
	B_ANTIMISSILE,
	B_ARTILLERY,
	B_FOCAL_MINE,
	B_SUB,
	B_AMPHIB,
	B_MINE,
	B_CARRIER,
	B_METAL_MAKER,
	B_FORTIFICATION,
	B_DGUN,
	B_REPAIR,
	B_IDLE,
	B_CMD,
	B_RANDMOVE,
	B_RESURECT,
	B_GLOBAL,
	B_RULE,
	B_RULE_EXTREME,
	B_RULE_EXTREME_CARRY,
	B_RETREAT,
	B_GUARDIAN,
	B_BOMBER,
	B_SHIELD,
	B_MISSILE_UNIT,
	B_FIGHTER,
	B_GUNSHIP,
	B_GUARD_FACTORY,
	B_GUARD_LIKE_CON,
	B_RECLAIM,
	B_RULE_EXTREME_NOFACT,
	B_WAIT,
	B_METAFAILED,
	B_SUPPORT,
	B_HUB,
	B_AIRSUPPORT,
	B_OFFENSIVE_REPAIR_RETREAT,
	B_GUARDIAN_MOBILES,
	B_NA
};

enum t_priority {
	// Part of the command caching system, maybe this should be
	// removed as these values are ignored
	tc_instant,
	tc_pseudoinstant,
	tc_assignment,
	tc_low,
	tc_na
};

enum t_direction {// Used in Map->WhichCorner()
	t_NE,
	t_NW,
	t_SE,
	t_SW,
	t_NA
};

enum cdgun {// dont think this is actually used anymore
	cd_no,
	cd_yes,
	cd_unsure
};
