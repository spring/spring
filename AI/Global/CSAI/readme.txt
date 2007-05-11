For how to use the CSAI Global AI, please see http://taspring.clan-sy.com/wiki/AI:CSAI

For documentation on C# Interface to build your own Global AIs, please see http://taspring.clan-sy.com/wiki/AI:CSAIInterface



New doc:

requirements:

- visual C++ 2003 Toolkit (may work with Visual C++ 2005 Express)
- microsoft framework 1.1 (need this even if you have framework 2.0)
- taspring
- taspring sourcecode that matches the version of your taspring

1.1 vs 2

I'm using visual C++ 2003, so we have to force framework 1.1 on csaiinterfaces and csailoader.
* The CSAI itself can be framework 2.0 *

1. CSAIInterfaces: C# reference classes for GlobalAI interfaces

global for all AIs, only one copy needed, only changes if Spring AI API changes (or if we
implement more functions)

use nant with .build file

It's copied into root of Taspring directory

2.ABICCompatiblityLayer

change paths in buildabic.bat
run "buildabic csaiabic.dll csailoader.dll"

csaiabic.dll is the dll that you will load in the lobby
csaiabic.dll will load csailoader.dll
csailoader.dll should have a C interface conform to what the ABIC layer is expecting

It's copied into root of Taspring\AI\Bot-libs

3. CSAiLoader

change paths in buildcsailoader.bat
run "buildcsailoader.bat"

creates csailoader.dll, which will be loaded by csaiabic.dll, and which loads the C# AI itself

There is a config file with the same name as the csailoader dll, which decides what C# AI to load

ie, you can rename csailoader.dll to anything you like (rebuild abiccompatibilitylayer to match the new name),
then copy csailoader.dll to the same name but with ".xml" instead of ".dll",eg 

mycsailoader.dll
mycsailoader.xml  -> which could point to mycsai.dll

It's copied into root of Taspring , along with the .xml file

4. CSAI

- make sure to add csaiinterfaces.dll as a reference to csai project
- you can find this in the root of the taspring directory, or in the csaiinterfaces project directory

- Place the resulting dll in a subfolder of taspring\ai such as taspring\ai\csai
- make sure your csailoader xml file points to this directory, filename, classname


5. Run

Open lobby, select dll, cross fingers


