#!/bin/bash
#
# Generates the Java JNA wrapper source files
#

SPRING_SOURCE=../../../../rts
MY_SOURCE_JAVA=../java/src/
JAVA_PKG=com/clan_sy/spring/ai

C_CALLBACK=${SPRING_SOURCE}/ExternalAI/Interface/SAICallback.h
C_EVENTS=${SPRING_SOURCE}/ExternalAI/Interface/AISEvents.h
C_COMMANDS=${SPRING_SOURCE}/ExternalAI/Interface/AISCommands.h
JNA_CALLBACK=${MY_SOURCE_JAVA}${JAVA_PKG}/AICallback.java

AWK=awk

##############################################
### do not change anything below this line ###

#echo "	generating source files ..."

mkdir -p ${MY_SOURCE_JAVA}${JAVA_PKG}/event
mkdir -p ${MY_SOURCE_JAVA}${JAVA_PKG}/command
mkdir -p ${MY_SOURCE_JAVA}${JAVA_PKG}/oo

$AWK -f jna_wrappEvents.awk ${C_EVENTS}

$AWK -f jna_wrappCommands.awk ${C_COMMANDS}

$AWK -f jna_wrappCallback.awk ${C_CALLBACK}

$AWK -f java_wrappCallbackOO.awk ${JNA_CALLBACK}

