@ECHO OFF
REM
REM Generates the Java JNA wrapper source files
REM

SET SPRING_SOURCE=../../../../rts
SET MY_SOURCE_JAVA=../java/src/
SET JAVA_PKG=com/clan_sy/spring/ai

SET AWK=gawk.exe

REM ##############################################
REM ### do not change anything below this line ###

SET C_CALLBACK=%SPRING_SOURCE%/ExternalAI/Interface/SAICallback.h
SET C_EVENTS=%SPRING_SOURCE%/ExternalAI/Interface/AISEvents.h
SET C_COMMANDS=%SPRING_SOURCE%/ExternalAI/Interface/AISCommands.h
SET JNA_CALLBACK=%MY_SOURCE_JAVA%%JAVA_PKG%/AICallback.java

REM ECHO "	generating source files ..."

IF NOT EXIST %MY_SOURCE_JAVA%%JAVA_PKG%/event   mkdir %MY_SOURCE_JAVA%%JAVA_PKG%/event
IF NOT EXIST %MY_SOURCE_JAVA%%JAVA_PKG%/command mkdir %MY_SOURCE_JAVA%%JAVA_PKG%/command
IF NOT EXIST %MY_SOURCE_JAVA%%JAVA_PKG%/oo      mkdir %MY_SOURCE_JAVA%%JAVA_PKG%/oo

%AWK% -f jna_wrappEvents.awk -f common.awk -f commonDoc.awk %C_EVENTS%

%AWK% -f jna_wrappCommands.awk -f common.awk -f commonDoc.awk %C_COMMANDS%

%AWK% -f jna_wrappCallback.awk -f common.awk -f commonDoc.awk %C_CALLBACK%

%AWK% -f java_wrappCallbackOO.awk -f common.awk -f commonDoc.awk -f commonOOCallback.awk %JNA_CALLBACK%

