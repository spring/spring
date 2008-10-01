// Copyright TASpring project, Hugh Perkins 2006
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

// The CSAIProxy class proxies between the Spring rts main exe, and the C# AI
// CSAIProxy will be used to generate a dll, which will be loaded by Spring
// CSAIProxy will in turn load the C# AI dll

// This class is pretty much done; just missing HandleEvent
// Most of the work will go into writing AICallbackForCS.h

// reads a config file with the same name as dll, and following structure
// <root>
// <config csaidirectory="AI/CSAI" csaidllname="CSAI.dll" csaiclassname="CSAI" >
// </config>
// </root>

#using <mscorlib.dll>
#using <System.Xml.dll>

#include <vcclr.h>

//using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Xml;

//#using <CSAIInterfaces.dll>

#include "CSAIProxy.h"
#include "ExternalAI/IGlobalAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

#include "AbicAICallbackWrapper.h"
#include "AbicUnitDefWrapper.h"
#include "AbicMoveDataWrapper.h"

#include "CSAIProxyAICallback.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}

//void WriteLine( char *message )
//{
    //FILE *file = fopen("out.log", "a+" );
    //fprintf( file, message );
    //fprintf( file, "\n" );
  //  fclose( file );
//}

// For last ditch debugging of C#/C++ interfacing.  Prefer to write messages from C#.
void WriteLine( System::String *message )
{
    StreamWriter *sw = new StreamWriter( "outm.log", true );
    sw->WriteLine( message );
    sw->Close();    
}

System::Byte ReadFile( System::String *filename )[]
{
    FileStream *fs = new FileStream( filename, FileMode::Open );
    BinaryReader *br = new BinaryReader( fs );
    System::Byte bytes[] = br->ReadBytes( (int)fs->Length );
    br->Close();
    return bytes;
}

System::Object *getResult( System::Byte assemblybytes[], System::Byte pdbbytes[], System::String *targettypename, System::String *methodname )
{
    Assembly *a;
    if( pdbbytes != 0 )
    {
        a = Assembly::Load( assemblybytes, pdbbytes );
    }
    else
    {
        a = Assembly::Load( assemblybytes );
    }
    Type *t = a->GetType( targettypename );
    MethodInfo *mi = t->GetMethod( methodname );    
    System::Object *result = mi->Invoke( 0, 0 );
    WriteLine( result->GetType()->ToString() );
    return result;
}

System::Object *DynLoad( System::String *assemblyfilename, System::String *pdbfilename, System::String *targettypename, System::String *methodname )
{
    WriteLine( "reading assembly..." );
    System::Byte assemblybytes[] = ReadFile( assemblyfilename );
    WriteLine( "... assembly read" );
    System::Byte pdbbytes[] = 0;
    if( File::Exists( pdbfilename ) )
    {
        WriteLine( "reading pdb..." );
        pdbbytes = ReadFile( pdbfilename );
    WriteLine( "... pdb read" );
    }
    return getResult( assemblybytes, pdbbytes, targettypename, methodname );
}

CSharpAI::IGlobalAI *LoadCSAI()
{
    System::String *configfilename = Assembly::GetCallingAssembly()->CodeBase->ToLower()->Replace( ".dll", ".xml" );
    WriteLine( String::Concat( "configfile: [", configfilename, "]" ) );
    XmlDocument *configdom = new XmlDocument();
    configdom->Load( configfilename );
    WriteLine("loaded dom" );
    XmlElement *configelement = dynamic_cast< XmlElement * >( configdom->SelectSingleNode( "/root/config" ) );
    WriteLine("loaded dom" );
    System::String *CSAIDirectory = configelement->GetAttribute("csaidirectory" );
    System::String *CSAIDllName = configelement->GetAttribute("csaidllname" );
    System::String *PdbName = CSAIDllName->Replace( ".dll", ".pdb" );
    System::String *CSAIClassName = configelement->GetAttribute("csaiclassname" );
    WriteLine("got attributes" );
    
    System::String *dllpath = Path::Combine( CSAIDirectory, CSAIDllName );
    System::String *pdbpath = Path::Combine( CSAIDirectory, PdbName );
    WriteLine(dllpath );
    WriteLine(pdbpath );
    System::Object *aiexecobject = DynLoad( dllpath, pdbpath,  CSAIClassName, "GetInstance" );
    WriteLine("Did load" );
    //WriteLine( aiexecobject->GetType()->ToString() );
    return dynamic_cast< CSharpAI::IGlobalAI *>( aiexecobject );
}

CSAIProxy::CSAIProxy()
{
    // csai is the main C# AI class.  This is the class that runs everything, from a C# point of view
    // there may possibly technically be a memory leak here, by not executing delete in our destructor,
    // but since we only construct the AI once per game it's a non-issue
  //  WriteLine("CSAIProxy");
    csai = LoadCSAI();
    WriteLine("aiexec created");    
}

CSAIProxy::~CSAIProxy()
{
    //delete csai;
}

void CSAIProxy::InitAI(AbicAICallbackWrapper *aicallback, int team)
{
    this->team = team;
	this->aicallback = aicallback;

    // -- test stuff here --    
    // -- end test stuff --    

    // we need to provide a managed class to C#, so we create one: AICallbackToGiveToCS
    // we store the unmanaged callback inside this class, so we can use it later
    aicallbacktogivetocs = new AICallbackToGiveToCS( aicallback );
    WriteLine("created aicallbacktogivetocs");
    csai->InitAI( aicallbacktogivetocs, team );
    WriteLine("ai initialized");
}

// fires for every unit, including initial commander
void CSAIProxy::UnitCreated(int unit)
{
    csai->UnitCreated( unit );
}

void CSAIProxy::UnitFinished(int unit)
{
    csai->UnitFinished( unit );
}

void CSAIProxy::UnitDestroyed(int unit,int attacker)
{
    csai->UnitDestroyed( unit, attacker );
}

void CSAIProxy::EnemyEnterLOS(int enemy)
{
    csai->EnemyEnterLOS( enemy );
}

void CSAIProxy::EnemyLeaveLOS(int enemy)
{
    csai->EnemyLeaveLOS( enemy );
}

void CSAIProxy::EnemyEnterRadar(int enemy)
{
    csai->EnemyEnterRadar( enemy );
}

void CSAIProxy::EnemyLeaveRadar(int enemy)
{
    csai->EnemyLeaveRadar( enemy );
}

void CSAIProxy::EnemyDamaged(int damaged,int attacker,float damage,float3 dir)
{
    csai->EnemyDamaged( damaged, attacker, damage, new CSharpAI::Float3( dir.x, dir.y, dir.z ) );
}

void CSAIProxy::EnemyDestroyed(int enemy,int attacker)
{
    csai->EnemyDestroyed( enemy, attacker );
}

void CSAIProxy::UnitIdle(int unit)
{
    csai->UnitIdle( unit );
}

void CSAIProxy::GotChatMsg(const char* msg,int player)
{
    if( strcmp( msg, ".reloadai" ) == 0 )
    {
        csai->Shutdown(); // release logfile lock, any threads, etc...
        
        csai = LoadCSAI();
        
        aicallbacktogivetocs = new AICallbackToGiveToCS( aicallback );
        csai->InitAI( aicallbacktogivetocs, team );
    }
    
    csai->GotChatMsg( msg, player );
}

void CSAIProxy::UnitDamaged(int damaged,int attacker,float damage,float3 dir)
{
    csai->UnitDamaged( damaged, attacker, damage, new CSharpAI::Float3( dir.x, dir.y, dir.z ) );
}

void CSAIProxy::UnitMoveFailed(int unit)
{
    csai->UnitMoveFailed( unit );
}

// TODO
int CSAIProxy::HandleEvent(int msg,const void* data)
{
	return 0;
}

void CSAIProxy::Update()
{
    csai->Update();
}
