#!/bin/sh
#
# Generates the Java JNA wrapper source files
#

# It is guaranteed that this script can assume
# that CWD is set to where this script is located,
# so we can define relative paths to this scripts parent dir.
# This should be: AI/Interfaces/Java/bin

SPRING_SOURCE=../../../../rts
MY_SOURCE_JAVA=../java/src
GENERATED_SOURCE_DIR=../java/generated
JAVA_PKG=com/springrts/ai

# using the default awk flavor of the system
# you may want to change this to one of the following:
# awk, mawk, gawk, nawk, ...
AWK="awk"

MKDIR="mkdir -p"

##############################################
### do not change anything below this line ###

AWK_COMMON_SCRIPTS_DIR=${SPRING_SOURCE}/AI/Wrappers/CUtils/bin
C_CALLBACK=${SPRING_SOURCE}/ExternalAI/Interface/SSkirmishAICallback.h
C_EVENTS=${SPRING_SOURCE}/ExternalAI/Interface/AISEvents.h
C_COMMANDS=${SPRING_SOURCE}/ExternalAI/Interface/AISCommands.h
JNA_CALLBACK=${GENERATED_SOURCE_DIR}/${JAVA_PKG}/AICallback.java

VARS="-v GENERATED_SOURCE_DIR=${GENERATED_SOURCE_DIR}"

#echo "	generating source files ..."

CWD_BACKUP=$(pwd)
THIS_DIR=$(dirname ${0})
cd ${THIS_DIR}

${MKDIR} ${GENERATED_SOURCE_DIR}/${JAVA_PKG}/event
${MKDIR} ${GENERATED_SOURCE_DIR}/${JAVA_PKG}/command
${MKDIR} ${GENERATED_SOURCE_DIR}/${JAVA_PKG}/oo

# To make the following lines shorter
ACSD=${AWK_COMMON_SCRIPTS_DIR}

${AWK} ${VARS} -f jna_wrappEvents.awk -f ${ACSD}/common.awk -f ${ACSD}/commonDoc.awk ${C_EVENTS}

${AWK} ${VARS} -f jna_wrappCommands.awk -f ${ACSD}/common.awk -f ${ACSD}/commonDoc.awk ${C_COMMANDS}

${AWK} ${VARS} -f jna_wrappCallback.awk -f ${ACSD}/common.awk -f ${ACSD}/commonDoc.awk ${C_CALLBACK}

${AWK} ${VARS} -f java_wrappCallbackOO.awk -f ${ACSD}/common.awk -f ${ACSD}/commonDoc.awk -f ${ACSD}/commonOOCallback.awk ${JNA_CALLBACK}

cd ${CWD_BACKUP}

