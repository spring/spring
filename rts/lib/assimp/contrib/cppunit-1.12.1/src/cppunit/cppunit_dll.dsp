# Microsoft Developer Studio Project File - Name="cppunit_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cppunit_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cppunit_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cppunit_dll.mak" CFG="cppunit_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cppunit_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cppunit_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cppunit_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cppunit_dll___Win32_Release"
# PROP BASE Intermediate_Dir "cppunit_dll___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseDll"
# PROP Intermediate_Dir "ReleaseDll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zd /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_BUILD_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"..\..\lib\cppunit_dll.pdb" /machine:I386
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetDir=.\ReleaseDll
TargetPath=.\ReleaseDll\cppunit_dll.dll
TargetName=cppunit_dll
SOURCE="$(InputPath)"
PostBuild_Desc=Copying target to lib/
PostBuild_Cmds=copy "$(TargetPath)" ..\..\lib\$(TargetName).dll	copy "$(TargetDir)\$(TargetName).lib" ..\..\lib\$(TargetName).lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "cppunit_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cppunit_dll___Win32_Debug"
# PROP BASE Intermediate_Dir "cppunit_dll___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugDll"
# PROP Intermediate_Dir "DebugDll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPPUNIT_BUILD_DLL" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"..\..\lib\cppunitd_dll.pdb" /debug /machine:I386 /out:"DebugDll\cppunitd_dll.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetDir=.\DebugDll
TargetPath=.\DebugDll\cppunitd_dll.dll
TargetName=cppunitd_dll
SOURCE="$(InputPath)"
PostBuild_Desc=Copying target to lib/
PostBuild_Cmds=copy "$(TargetPath)" ..\..\lib\$(TargetName).dll	copy "$(TargetDir)\$(TargetName).lib" ..\..\lib\$(TargetName).lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "cppunit_dll - Win32 Release"
# Name "cppunit_dll - Win32 Debug"
# Begin Group "DllSpecific"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DllMain.cpp
# End Source File
# End Group
# Begin Group "extension"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\ExceptionTestCaseDecorator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\Orthodox.h
# End Source File
# Begin Source File

SOURCE=.\RepeatedTest.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\RepeatedTest.h
# End Source File
# Begin Source File

SOURCE=.\TestCaseDecorator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestCaseDecorator.h
# End Source File
# Begin Source File

SOURCE=.\TestDecorator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestDecorator.h
# End Source File
# Begin Source File

SOURCE=.\TestSetUp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSetUp.h
# End Source File
# End Group
# Begin Group "helper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\AutoRegisterSuite.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\HelperMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestCaller.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestFactory.h
# End Source File
# Begin Source File

SOURCE=.\TestFactoryRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestFactoryRegistry.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestFixtureFactory.h
# End Source File
# Begin Source File

SOURCE=.\TestNamer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestNamer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSuiteBuilder.h
# End Source File
# Begin Source File

SOURCE=.\TestSuiteBuilderContext.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSuiteBuilderContext.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TestSuiteFactory.h
# End Source File
# Begin Source File

SOURCE=.\TypeInfoHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\TypeInfoHelper.h
# End Source File
# End Group
# Begin Group "core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AdditionalMessage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\AdditionalMessage.h
# End Source File
# Begin Source File

SOURCE=.\Asserter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Asserter.h
# End Source File
# Begin Source File

SOURCE=.\Exception.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Exception.h
# End Source File
# Begin Source File

SOURCE=.\Message.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Message.h
# End Source File
# Begin Source File

SOURCE=.\SourceLine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\SourceLine.h
# End Source File
# Begin Source File

SOURCE=.\SynchronizedObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\SynchronizedObject.h
# End Source File
# Begin Source File

SOURCE=.\Test.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Test.h
# End Source File
# Begin Source File

SOURCE=.\TestAssert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestAssert.h
# End Source File
# Begin Source File

SOURCE=.\TestCase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestCase.h
# End Source File
# Begin Source File

SOURCE=.\TestComposite.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestComposite.h
# End Source File
# Begin Source File

SOURCE=.\TestFailure.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestFailure.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestFixture.h
# End Source File
# Begin Source File

SOURCE=.\TestLeaf.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestLeaf.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestListener.h
# End Source File
# Begin Source File

SOURCE=.\TestPath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestPath.h
# End Source File
# Begin Source File

SOURCE=.\TestResult.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestResult.h
# End Source File
# Begin Source File

SOURCE=.\TestRunner.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestRunner.h
# End Source File
# Begin Source File

SOURCE=.\TestSuite.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestSuite.h
# End Source File
# End Group
# Begin Group "output"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CompilerOutputter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\CompilerOutputter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Outputter.h
# End Source File
# Begin Source File

SOURCE=.\TestResultCollector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TestResultCollector.h
# End Source File
# Begin Source File

SOURCE=.\TextOutputter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextOutputter.h
# End Source File
# Begin Source File

SOURCE=.\XmlOutputter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\XmlOutputter.h
# End Source File
# Begin Source File

SOURCE=.\XmlOutputterHook.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\XmlOutputterHook.h
# End Source File
# End Group
# Begin Group "portability"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\..\include\cppunit\config\config-bcb5.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\cppunit\config\config-mac.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\cppunit\config\config-msvc6.h"
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\config\CppUnitApi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\portability\CppUnitDeque.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\portability\CppUnitMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\portability\CppUnitSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\portability\CppUnitStack.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\portability\CppUnitVector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Portability.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\config\SelectDllLoader.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\config\SourcePrefix.h
# End Source File
# End Group
# Begin Group "textui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\cppunit\ui\text\TestRunner.h
# End Source File
# Begin Source File

SOURCE=.\TextTestRunner.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextTestRunner.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\ui\text\TextTestRunner.h
# End Source File
# End Group
# Begin Group "listener"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BriefTestProgressListener.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\BriefTestProgressListener.h
# End Source File
# Begin Source File

SOURCE=.\TestSuccessListener.cpp
# End Source File
# Begin Source File

SOURCE=.\TextTestProgressListener.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextTestProgressListener.h
# End Source File
# Begin Source File

SOURCE=.\TextTestResult.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\TextTestResult.h
# End Source File
# End Group
# Begin Group "documentation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\..\doc\cookbook.dox
# End Source File
# Begin Source File

SOURCE=..\..\doc\FAQ
# End Source File
# Begin Source File

SOURCE=..\..\NEWS
# End Source File
# Begin Source File

SOURCE=..\..\doc\other_documentation.dox
# End Source File
# Begin Source File

SOURCE=..\..\TODO
# End Source File
# End Group
# Begin Group "plugin"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BeosDynamicLibraryManager.cpp
# End Source File
# Begin Source File

SOURCE=.\DynamicLibraryManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\plugin\DynamicLibraryManager.h
# End Source File
# Begin Source File

SOURCE=.\DynamicLibraryManagerException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\plugin\DynamicLibraryManagerException.h
# End Source File
# Begin Source File

SOURCE=.\PlugInManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\plugin\PlugInManager.h
# End Source File
# Begin Source File

SOURCE=.\PlugInParameters.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\plugin\PlugInParameters.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\plugin\TestPlugIn.h
# End Source File
# Begin Source File

SOURCE=.\TestPlugInDefaultImpl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\plugin\TestPlugInDefaultImpl.h
# End Source File
# Begin Source File

SOURCE=.\UnixDynamicLibraryManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32DynamicLibraryManager.cpp
# End Source File
# End Group
# Begin Group "tools"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\StringTools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\tools\StringTools.h
# End Source File
# Begin Source File

SOURCE=.\XmlDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\tools\XmlDocument.h
# End Source File
# Begin Source File

SOURCE=.\XmlElement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\tools\XmlElement.h
# End Source File
# End Group
# Begin Group "protector"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DefaultProtector.cpp
# End Source File
# Begin Source File

SOURCE=.\DefaultProtector.h
# End Source File
# Begin Source File

SOURCE=.\Protector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Protector.h
# End Source File
# Begin Source File

SOURCE=.\ProtectorChain.cpp
# End Source File
# Begin Source File

SOURCE=.\ProtectorChain.h
# End Source File
# Begin Source File

SOURCE=.\ProtectorContext.h
# End Source File
# End Group
# Begin Source File

SOURCE="..\..\INSTALL-WIN32.txt"
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\Makefile.am
# End Source File
# Begin Source File

SOURCE=.\Makefile.am
# End Source File
# Begin Source File

SOURCE=..\..\include\cppunit\extensions\XmlInputHelper.h
# End Source File
# End Target
# End Project
