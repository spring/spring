// Copyright Jelmer Cnossen, Hugh Perkins 2006
// hughperkins@gmail.com http://manageddreams.com
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURVector3E. See the GNU General Public License for
//  more details.
//
// You should have received a copy of the GNU General Public License along
// with this program in the file licence.txt; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-
// 1307 USA
// You can find the licence also on the web at:
// http://www.opensource.org/licenses/gpl-license.php
//
// ======================================================================================
//
// Wraps Mono runtime, loads CSAI via Mono runtime.  Yay!

#include <stdexcept>
#include <string>
#include <iostream>
using namespace std;

extern "C" {
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/class.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/loader.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/mono-debug.h>
}

#include "lib/streflop/streflop.h"
#include "lib/streflop/FPUSettings.h"
using namespace streflop;

//#include "FPUCheck.h"

#include "ExternalAI/aibase.h"

#include "ExternalAI/GlobalAICInterface/AbicAICallback.h"

extern "C"{
gpointer mono_delegate_to_ftnptr (MonoDelegate *delegate); 
}

/////////////////////////////////////////////////////////////////////////////

// default calling convention is different on each platform
// Mono uses the default calling convention of a platform
#ifdef _WIN32
#define PLATFORMCALLINGCONVENTION __stdcall
#else
#define PLATFORMCALLINGCONVENTION __cdecl
#endif

// #define ACTIVATEMONODEBUG

static MonoAssembly *assembly = 0;
static MonoObject *launcher = 0;
static MonoDomain *domain = 0;
static MonoImage *image = 0;
static MonoClass *launcherClass = 0;

void WriteLine( std::string message )
{
    FILE *file = fopen("outn.log", "a+" );
    fprintf( file, message.c_str() );
    fprintf( file, "\n" );
    fclose( file );
}

//static struct IAICallback *thisaicallback = 0;

// callbacks/GlobalAI events=============================================================

typedef void( PLATFORMCALLINGCONVENTION *INITAI)( void *, int );
typedef void( PLATFORMCALLINGCONVENTION *UNITCREATED)( int );

class AIProxy
{
public:
    INITAI InitAI;
    UNITCREATED UnitCreated;
};

static AIProxy *thisaiproxy = 0; // used to ensure callbacks go to right place, and to avoid passing parameters to Bind

void SetInitAICallback( void *fnpointer )
{
    WriteLine( "SetInitAICallback() >>>" );
	thisaiproxy->InitAI = (INITAI)fnpointer;
    WriteLine( "SetInitAICallback() <<<" );
}

void SetUnitCreatedCallback( void *fnpointer ){ thisaiproxy->UnitCreated = (UNITCREATED)fnpointer; }

// Functionwrappers=============================================================

#include "IAICallbackdefinitions_generated.gpp"
#include "IFeatureDefdefinitions_generated.gpp"
#include "IMoveDatadefinitions_generated.gpp"
#include "IUnitDefdefinitions_generated.gpp"

const void *_UnitDef_get_movedata( void *unitdef )
{
    return UnitDef_get_movedata( ( UnitDef *)unitdef );
}

const void *_IAICallback_GetUnitDefByTypeId( void *aicallback, int unittypeid )
{
    return IAICallback_GetUnitDefByTypeId( ( IAICallback *)aicallback, unittypeid );
}

const void *_IAICallback_GetUnitDef( void *aicallback, int unitdeployedid )
{
    return IAICallback_GetUnitDef( ( IAICallback *)aicallback, unitdeployedid );
}

void _IAICallback_GetUnitPos( void *aicallback, float &x, float &y, float &z, int unit )
{
    IAICallback_GetUnitPos( ( IAICallback *)aicallback, x, y, z, unit );
}

void _IAICallback_ClosestBuildSite( void *aicallback, float &nx, float &ny, float &nz, void *unitdef, float x, float y, float z, float searchRadius, int minDist, int facing )
{
    IAICallback_ClosestBuildSite( ( IAICallback *)aicallback, nx, ny, nz, (UnitDef *)unitdef, x, y, z,searchRadius, minDist, facing );
}
        
void _IAICallback_GiveOrder( void * aicallback, int id, int commandid, int numparams, float p1, float p2, float p3, float p4 )
{
    IAICallback_GiveOrder( ( IAICallback *)aicallback, id, commandid, numparams, p1, p2, p3, p4 );
}
        
void _IAICallback_DrawUnit( void * aicallback, MonoString *name, float x, float y, float z,float rotation,
                            int lifetime, int team,bool transparent,bool drawBorder,int facing)
{
    char *name_utf8string = mono_string_to_utf8(name );
    IAICallback_DrawUnit( ( IAICallback *)aicallback, name_utf8string, x, y, z, rotation, lifetime, team, transparent, drawBorder, facing );
    g_free( name_utf8string );
}

// Monostuff=============================================================

MonoObject* InvokeMethod(const char *method, MonoObject *obj, void **params)
{
	MonoMethodDesc *mdesc = mono_method_desc_new(method, FALSE);
	MonoClass *cls = mono_object_get_class (obj);
	MonoMethod *m = mono_method_desc_search_in_class(mdesc, cls);

	MonoObject *exc = 0;
	MonoObject *ret = 0;
	if (m)
		ret = mono_runtime_invoke(m, obj, params, &exc);
	else
		WriteLine ((std::string("Method ") + method + std::string(" not found.\n")).c_str());

	mono_method_desc_free(mdesc);
	return ret;
}

void InitMono(const char *monoDir, const char *dll, const char *namespacename, const char*classname)
{
    if( domain == 0 )
    {
        mono_set_dirs (monoDir, NULL);
    
        domain = mono_jit_init (dll); // pass in a 2.0 assembly to initialize runtime correctly
        if (!domain)
            WriteLine( "Couldn't create mono JIT runtime.");
    
        #ifdef ACTIVATEMONODEBUG
        mono_debug_init(MONO_DEBUG_FORMAT_MONO);
        mono_debug_init_1( domain );
        #endif    

        // load the managed dll
        assembly = mono_domain_assembly_open (domain, dll);
        if (!assembly) 
            WriteLine(std::string( "failed to load ") + std::string(dll));
    
        #ifdef ACTIVATEMONODEBUG
        mono_debug_init_2( assembly );
        #endif
        
        image = mono_assembly_get_image(assembly);
    
        // register native methods callable by managed code
    #define PREFIX "CSharpAI.CSAICInterface::"
        mono_add_internal_call(PREFIX "SetInitAICallback", (void*)SetInitAICallback);
        mono_add_internal_call(PREFIX "SetUnitCreatedCallback", (void*)SetUnitCreatedCallback);
    #undef PREFIX 
        
    #define PREFIX "CSharpAI.ABICInterface::"
        #include "IAICallbackbindings_generated.gpp"
        #include "IFeatureDefbindings_generated.gpp"
        #include "IMoveDatabindings_generated.gpp"
        #include "IUnitDefbindings_generated.gpp"

        // note: call in C# has no underscore prefix, wrapper call in C does (to avoid conflict with ABIC itself)
        mono_add_internal_call(PREFIX "IAICallback_GetUnitDef", (void*)_IAICallback_GetUnitDef);
        mono_add_internal_call(PREFIX "IAICallback_GetUnitDefByTypeId", (void*)_IAICallback_GetUnitDefByTypeId);
        mono_add_internal_call(PREFIX "IAICallback_GetUnitPos", (void*)_IAICallback_GetUnitPos);
        mono_add_internal_call(PREFIX "IAICallback_ClosestBuildSite", (void*)_IAICallback_ClosestBuildSite);
        mono_add_internal_call(PREFIX "IAICallback_GiveOrder", (void*)_IAICallback_GiveOrder);
        mono_add_internal_call(PREFIX "IAICallback_DrawUnit", (void*)_IAICallback_DrawUnit);
        mono_add_internal_call(PREFIX "UnitDef_get_movedata", (void*)_UnitDef_get_movedata);
    #undef PREFIX 
    
        // create an instance of the launcher class
        launcherClass = mono_class_from_name(image, namespacename, classname);
        if (!launcherClass)
            WriteLine("failed to find CSharpAI.CSAICInterface class.");
    
    }
}

// declare that we present a C interface.  Spring checks for this.
DLL_EXPORT bool IsCInterface()
{
    return true;
}

DLL_EXPORT int GetGlobalAiVersion()
{
	return GLOBALAI_C_INTERFACE_VERSION;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name, "CSAILoaderMono" );
}

static int numexec = 0;
DLL_EXPORT void *InitAI( struct IAICallback *aicallback, int team)
{
    AIProxy *aiproxy = new AIProxy(); // set this variable, so the callbacks go to right place
    thisaiproxy = aiproxy;  // so Set callbacks work ok
    if( numexec < 2 )
    {
        try
        {
        //::thisaicallback = aicallback;
    WriteLine( "InitAI >>>" );
    InitMono(".", "CSAIMono.dll", "CSharpAI", "CSAICInterface"); // note to self: make this a little more configurable...
    streflop_init<streflop::Simple>(); // do some magic to avoid assert.

    launcher = mono_object_new (domain, launcherClass);
    mono_runtime_object_init(launcher);

    WriteLine( "InitAI invoking bind..." );
    IAICallback_SendTextMsg( aicallback, "test message 2", 0 );
	InvokeMethod(":Bind", launcher, 0);
    IAICallback_SendTextMsg( aicallback, "test message 3", 0 );
    WriteLine( "InitAI calling initai..." );
    aiproxy->InitAI( aicallback, team );
        numexec++;
    WriteLine( "InitAI <<<" );
        }
        catch( const std::runtime_error& e)
            {
		WriteLine("Exception:");
		WriteLine(e.what());
		WriteLine("\n");
	}
    }
    return aiproxy;
}

DLL_EXPORT void UnitCreated( void *ai, int unit)									//called when a new unit is created on ai team
{
    WriteLine( "UnitCreated >>>" );
    ( (AIProxy *)ai )->UnitCreated( unit );
    WriteLine( "UnitCreated <<<" );
}

DLL_EXPORT void UnitFinished(void *ai, int unit)							//called when an unit has finished building
{
}

DLL_EXPORT void UnitDestroyed(void *ai, int unit,int attacker)								//called when a unit is destroyed
{
}

DLL_EXPORT void EnemyEnterLOS(void *ai, int enemy)
{
}

DLL_EXPORT void EnemyLeaveLOS(void *ai, int enemy)
{
}

DLL_EXPORT void EnemyEnterRadar(void *ai, int enemy)		
{
}

DLL_EXPORT void EnemyLeaveRadar(void *ai, int enemy)
{
}

DLL_EXPORT void EnemyDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)	//called when an enemy inside los or radar is damaged
{
}

DLL_EXPORT void EnemyDestroyed(void *ai, int enemy,int attacker)							//will be called if an enemy inside los or radar dies (note that leave los etc will not be called then)
{
}

DLL_EXPORT void UnitIdle(void *ai, int unit)										//called when a unit go idle and is not assigned to any group
{
}

DLL_EXPORT void GotChatMsg(void *ai, const char* msg,int player)					//called when someone writes a chat msg
{
}

DLL_EXPORT void UnitDamaged(void *ai, int damaged,int attacker,float damage, float dirx, float diry, float dirz)					//called when one of your units are damaged
{
}

DLL_EXPORT void UnitMoveFailed(void *ai, int unit)
{
}

//int HandleEvent (int msg,const void *data); // todo

//called every frame
DLL_EXPORT void Update(void *ai )
{
}
