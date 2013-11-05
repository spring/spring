#!/bin/bash

#########################
# Config
PAGE_MAJOR="Template:EngineVersion:Stable:Major"
PAGE_MINOR="Template:EngineVersion:Stable:Minor"
PAGE_PATCH="Template:EngineVersion:Stable:Patchset"
PAGE_RDATE="Template:EngineVersion:Stable:ReleaseDate"

if [ ! -f ~/.ssh/spring_wiki_account ]; then
	echo "couldn't find ~/.ssh/spring_wiki_account with proper username & password!"
	echo "please create with `echo \"$USERNAME:$PASSWORD\" > ~/.ssh/spring_wiki_account`"
	exit 1
fi
USERANDPASS=$(cat ~/.ssh/spring_wiki_account)
USERNAME=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[1]}')
USERPASS=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[2]}')

. buildbot/slave/prepare.sh $*

if [ x${BRANCH} != xmaster ]; then
	echo "Don't update download page. We aren't on master branch."
	exit
fi


#########################
# Prepare content
MAJOR=$(echo "${REV}" | awk '{split($0,a,"."); print a[1]}')
MINOR=$(echo "${REV}" | awk '{split($0,a,"."); print a[2]}')
PATCH=$(echo "${REV}" | awk '{split($0,a,"."); print a[3]}')
RDATE=`LC_TIME="en_US.UTF-8" date +"%e. %B %Y"`


#########################
# Upload
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE_MAJOR}" "" "$MAJOR"
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE_MINOR}" "" "$MINOR"
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE_PATCH}" "" "$PATCH"
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE_RDATE}" "" "$RDATE"

exit $?
