For how to use the CSAI Global AI, please see http://taspring.clan-sy.com/wiki/AI:CSAI

For documentation on C# Interface to build your own Global AIs, please see http://taspring.clan-sy.com/wiki/AI:CSAIInterface



New doc:

requirements:

- Microsoft C++ 2008 Express (free beer download from Microsoft)
- Microsoft SDK (free beer download from Microsoft)
- nant
- .net framework 2
- mingw etc, all the requirements to build spring basically, including MINGDIR path and so on
- taspring sourcecode, from svn

1. CSAIInterfaces: C# reference classes for GlobalAI interfaces

global for all AIs, only one copy needed, only changes if Spring AI API changes (or if we
implement more functions)

Change into CSAI\CSAIInterfaces directory, and type:

nant

The resulting csaiinterfaces.dll/.pdb are copied into the "game" directory

2.ABICCompatiblityLayer

Change into CSAI\ABICompatiblityLayer directory

Type:

buildabic csaiabic.dll csailoader.dll

- csaiabic.dll is the dll that you will load in the lobby
- csaiabic.dll will load csailoader.dll
- csailoader.dll should have a C interface conform to what the ABIC layer is expecting

There are copied into root of game\AI\Skirmish/impls bot-libs directory

3. abicwrappers

- Change into csai\abicwrappers directory
- check/tweak/change paths in generate.bat
- run:

   generate

-> should create a bunch of xxx_generated.h files in the same directory

4. CSAiLoader

change paths in buildcsailoader.bat
run "buildcsailoader.bat"

creates csailoader.dll, which will be loaded by csaiabic.dll, and which loads the C# AI itself

There is a config file with the same name as the csailoader dll, which decides what C# AI to load

ie, you can rename csailoader.dll to anything you like (rebuild abiccompatibilitylayer to match the new name),
then copy csailoader.dll to the same name but with ".xml" instead of ".dll",eg 

mycsailoader.dll
mycsailoader.xml  -> which could point to mycsai.dll

It's copied into root of Taspring , along with the .xml file

5. CSAI

- make sure to add csaiinterfaces.dll as a reference to csai project
- you can find this in the root of the taspring directory, or in the csaiinterfaces project directory

- Place the resulting dll in a subfolder of taspring\ai such as taspring\ai\csai
- make sure your csailoader xml file points to this directory, filename, classname

Be sure to copy msvcr90.dll and so on into "game" directory

6. Run

Open lobby, select dll, cross fingers


