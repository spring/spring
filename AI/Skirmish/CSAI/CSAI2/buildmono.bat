set MONODIR=h:\bin\Mono1.2
set SPRINGSOURCE=..\..\..\..\..

set PATH=%PATH%;%MONODIR%\bin

copy /y %SPRINGSOURCE%\rts\ExternalAI\GlobalAIInterfaces\GlobalAIInterfaces.dll .
call gmcs -debug -out:CSAI.dll -t:library -reference:GlobalAIInterfaces.dll *.cs mapping\*.cs packcoordinators\*.cs utils\*.cs basicttypes\*.cs unitdata\*.cs

copy /y CSAI.dll %SPRINGSOURCE%\game\AI\CSAI
copy /y CSAI.pdb %SPRINGSOURCE%\game\AI\CSAI
