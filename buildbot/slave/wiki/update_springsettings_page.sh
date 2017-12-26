#!/bin/bash

#########################
# Config
SPRING="spring-headless"
PAGE="Springsettings.cfg"
SECTIONNAME="Available Options"

if [ ! -f ~/.ssh/spring_wiki_account ]; then
	echo "couldn't find ~/.ssh/spring_wiki_account with proper username & password!"
	echo "please create with:"
	echo "echo \"\$USERNAME:\$PASSWORD\" > ~/.ssh/spring_wiki_account"
	exit 1
fi
USERANDPASS=$(cat ~/.ssh/spring_wiki_account)
USERNAME=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[1]}')
USERPASS=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[2]}')

. buildbot/slave/prepare.sh $*
SPRING="${BUILDDIR}/${SPRING}"

#########################
# Parse & Transform JSON

#TODO also reparse doc of previous spring and then check for new ones and mark them on the wiki (and maybe removed ones too)
SPRINGCFG_JSON=`$SPRING --list-config`
SPRING_VERSION=`$SPRING --sync-version`

if [ $? != 0 ]; then
	echo "spring failed"
	exit 1
fi

TEMPLATE_CONTENT=`echo "$SPRINGCFG_JSON" | python2 $(dirname $0)/spring_json_to_mediawiki.py`
TEMPLATE_CONTENT=$(echo -e "=${SECTIONNAME}=\n{{HeaderNotice|(last update: $REV)}}\n<center><span class=warning>'''THIS SECTION IS AUTOMATICALLY GENERATED! DON'T EDIT IT!'''</span></center>\n${TEMPLATE_CONTENT}")

if [ $? != 0 ]; then
	echo "python parsing failed"
	exit 1
fi

#########################
# Upload
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE}" "${SECTIONNAME}" "$TEMPLATE_CONTENT"

exit $?
