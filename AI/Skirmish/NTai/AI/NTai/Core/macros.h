// Written by AF, under GPL V2


// Prevent VS2005 spamming messages about insecure functions and suggesting alternatives
// which won't work under linux
#define _CRT_SECURE_NO_WARNINGS

// This 'slash' macro is somewhat redundant as all platforms accept the / folder separator
// anyway, \ was just a convention IBM forced on windows 1.0 as / was used for program
// arguements, by the time they had the chance to officially change it people had already
// gotten used to it. But I didn't know this back then and it's easier to leave this macro
// in for the moment
#define slash "/"


//#define EXCEPTION

// Enables exception handling. Not much point in it though as spring uses mingw32 which means
// no structured exception handling


#define TC_SOURCE

// adds a string to the command structures so that they can be tracked from their origin
// useful when looking through commands in the debugger to track their source


//#define TNLOG

// Adds a Log command to the start of every function for huge logfiles so that errors and the
// flow of the program can be better tracked on remote computers without access to a debug
// build. Log files may be 10MB-4GB+ depending on how long the game is


// GAIA management macros
// used to filter out code based on whether it's set up as a gaia AI
#define ISGAIA (G->info->gaia == true)
#define ISNOTGAIA (G->info->gaia == false)
#define IF_GAIA if(G->info->gaia == true)
#define IF_NOTGAIA if(G->info->gaia == false)

#define GAIA_ONLY(a) {if(G->info->gaia == false) return a;}
#define NO_GAIA(a) {if(G->info->gaia == true) return a;}



// Used for macros where you dont wanna put something in but you cant use null/void
#define NA


// PI macros
// Used mainly in OTAI code that was ported, e.g. DT Ring Hnalding
#define PIx2                6.283185307L    // 360 Degrees
//#define PI                  3.141592654L    // 180 Degrees
#define PI_2                1.570796327L    //  90 Degrees
#define PI_4                0.785398163L    //  45 Degrees

// Game Time macros
//
// used as ( 4 SECONDS) remember to put brackets around otherwise a statement such as
// EVERY_(4 SECONDS) is interpreted as EVERY_(4) which is every 4 frames.
// This is all to simplify  time management in the 30 frames per second system spring uses.
#define SECONDS *30
#define SECOND *30
#define MINUTES *1800
#define MINUTE *1800
#define HOURS *108000
#define HOUR *108000
#define EVERY_SECOND (G->GetCurrentFrame()%30 == G->Cached->team)
#define EVERY_MINUTE (G->GetCurrentFrame()%1800 == G->Cached->team)
#define EVERY_(a) (G->GetCurrentFrame()%a == 0)
#define FRAMES

// converts from radians to degrees
#define DEGREES *(PI/180)

// Used to give end lines to the logger as endl cannot be passed to it
#define endline "\n"

// used to debug functions, functions log there names when called and the function that crashed the AI is the last one in the log listed
// CLOG is used before the logging class is initialized
// NLOG is used afterwards.
// CLOG relies on SendTxtMsg and is logged into infolog.txt and visible to the user
// NLOG only prints to the Log file
// Uncomment #define TNLOG further up to enable this extensive logging
#ifdef TNLOG
#define NLOG(a) (G->L.print(string("<")+to_string(G->Cached->team)+string(">")+string("function :: ") + a + string(endline)))
#define CLOG(a) {cb->SendTextMsg(a,1);}
#endif
#ifndef TNLOG
#define NLOG(a)
#define CLOG(a)
#endif

#define ECONRATIO 0.85f

// The max n# of orders sent on each cycle by the command cache.
// Commands are cached to prevent flooding the engine with too many commands at once, which
// can cause intense lag for minutes at a time.
#define BUFFERMAX 8

#define CMD_IDLE 23456

// Metal to energy ratio for cost calculations
#define METAL2ENERGY			45

// Minimum stocks for a "feasible" construction (ratio of storage)
#define	FEASIBLEMSTORRATIO		0.4
#define	FEASIBLEESTORRATIO		0.6

// Exception handling macros, uncomment #define EXCEPTION to enable or disable exception handling.
#ifdef EXCEPTION
    #define START_EXCEPTION_HANDLING try{

    #define END_EXCEPTION_HANDLING_AND(b,c) \
        }catch(boost::spirit::parser_error<EnumTdfErrors, char *> & e ){\
            c;\
			G->L.eprint(string("error in ")+b);\
            G->L.eprint(e.what());\
			string message= "";\
			switch(e.descriptor) {\
				case semicolon_expected: message = "semicolon expected"; break;\
				case equals_sign_expected: message = "equals sign in name value pair expected"; break;\
				case square_bracket_expected: message = "square bracket to close section name expected"; break;\
				case brace_expected: message = "brace or further name value pairs expected"; break;\
			};\
			G->L.eprint(message);\
        }catch(const std::exception& e){\
            c;\
            G->L.eprint(string("error in ")+b);\
            G->L.eprint(e.what());\
        }catch(string s){\
            c;\
            G->L.eprint(string("error in ")+b);\
            G->L.eprint(s);\
        }catch(...){\
            c;\
            G->L.eprint(string("error in ")+b);\
        }

    #define END_EXCEPTION_HANDLING(b) \
        }catch(boost::spirit::parser_error<EnumTdfErrors, char *> & e ){\
            G->L.eprint(string("error in ")+b);\
            G->L.eprint(e.what());\
			string message= "";\
			switch(e.descriptor) {\
				case semicolon_expected: message = "semicolon expected"; break;\
				case equals_sign_expected: message = "equals sign in name value pair expected"; break;\
				case square_bracket_expected: message = "square bracket to close section name expected"; break;\
				case brace_expected: message = "brace or further name value pairs expected"; break;\
			};\
			G->L.eprint(message);\
        }catch(const std::exception& e){\
            G->L.eprint(string("error in ")+b);\
            G->L.eprint(e.what());\
        }catch(string s){\
            G->L.eprint(string("error in ")+b);\
            G->L.eprint(s);\
        }catch(...){\
            G->L.eprint(string("error in ")+b);\
        }

#else
    #define START_EXCEPTION_HANDLING /* */
    #define END_EXCEPTION_HANDLING_AND(b,c) /* */
    #define END_EXCEPTION_HANDLING(b) /* */

#endif
