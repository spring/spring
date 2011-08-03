/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef E_LEVEL_OF_SUPPORT_H
#define	E_LEVEL_OF_SUPPORT_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @brief level of support
 *
 * This is used by Skirmish AIs for example.
 * The engine can pass some info to the AI,
 * eg. a mod name plus version and the engine version,
 * and the AI will report with a value from this enum,
 * to indicate the level of support it can serve for a
 * game using all of the supplied info.
 * If the AI is unable to determine its level of support,
 * it should return LOS_Unknown.
 */
enum LevelOfSupport {
	LOS_None,		// 0: will (possibly) result in a crash
	LOS_Bad,		// 1: does not work correctly, eg.: does nothing, just stand around, but neither does crash
	LOS_Working,	// 2: does work, but may not use all info the engine and ai interface supply
	LOS_Compleet,	// 3: does work and use the engine and ai interface to the fullest,
					//    but is optimised for newer versions
	LOS_Optimal,	// 4: is made optimally suitet for this engine and ai interface version
	LOS_Unknown		// 5: not evaluated (used eg when a library is not accessed directly,
					//    but info is loaded from a file)
};

#ifdef	__cplusplus
} // extern "C"
#endif

#endif // E_LEVEL_OF_SUPPORT_H
