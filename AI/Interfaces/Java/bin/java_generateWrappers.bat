@ECHO OFF
REM
REM Generates the Java JNA wrapper source files
REM

SET SPRING_SOURCE=../../../../rts
SET MY_SOURCE_JAVA=../java/src/
SET GENERATED_SOURCE_DIR=../java/generated
SET JAVA_PKG=com/clan_sy/spring/ai

SET AWK=gawk.exe

REM ##############################################
REM ### do not change anything below this line ###

SET C_CALLBACK=%SPRING_SOURCE%/ExternalAI/Interface/SAICallback.h
SET C_EVENTS=%SPRING_SOURCE%/ExternalAI/Interface/AISEvents.h
SET C_COMMANDS=%SPRING_SOURCE%/ExternalAI/Interface/AISCommands.h
SET JNA_CALLBACK=%MY_SOURCE_JAVA%%JAVA_PKG%/AICallback.java

SET VARS="-v GENERATED_SOURCE_DIR=%GENERATED_SOURCE_DIR%"

REM ECHO "	generating source files ..."

IF NOT EXIST %MY_SOURCE_JAVA%%JAVA_PKG%/event   mkdir %MY_SOURCE_JAVA%%JAVA_PKG%/event
IF NOT EXIST %MY_SOURCE_JAVA%%JAVA_PKG%/command mkdir %MY_SOURCE_JAVA%%JAVA_PKG%/command
IF NOT EXIST %MY_SOURCE_JAVA%%JAVA_PKG%/oo      mkdir %MY_SOURCE_JAVA%%JAVA_PKG%/oo

%AWK% %VARS% -f jna_wrappEvents.awk -f common.awk -f commonDoc.awk %C_EVENTS%

%AWK% %VARS% -f jna_wrappCommands.awk -f common.awk -f commonDoc.awk %C_COMMANDS%

%AWK% %VARS% -f jna_wrappCallback.awk -f common.awk -f commonDoc.awk %C_CALLBACK%

%AWK% %VARS% -f java_wrappCallbackOO.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk %JNA_CALLBACK%

