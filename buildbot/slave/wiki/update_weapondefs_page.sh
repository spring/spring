#!/bin/bash

#########################
# Config
SPRING="spring"
PAGE="User:jk/WeaponDefs"
SECTIONNAME="List"

if [ ! -f ~/.ssh/spring_wiki_account ]; then
	echo "couldn't find ~/.ssh/spring_wiki_account with proper username & password!"
	echo "please create with `echo \"$USERNAME:$PASSWORD\" > ~/.ssh/spring_wiki_account`"
	exit 1
fi
USERANDPASS=$(cat ~/.ssh/spring_wiki_account)
USERNAME=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[1]}')
USERPASS=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[2]}')

. buildbot/slave/prepare.sh $*
SPRING="${BUILDDIR}/${SPRING}"

#########################
# Parse & Transform JSON
SPRINGCFG_JSON=`$SPRING --list-def-tags`

PYCODE=$(cat <<EOF
import json, sys
d=json.load(sys.stdin)
for cfgtag,t in sorted(d['WeaponDefs'].iteritems()):
	tmp = "|name=" + cfgtag
	if "fallbackName" in t:
		tmp += "|fallback=" + t['fallbackName']
	defval=""
	if "defaultValue" in t:
		defval=t['defaultValue']
	if t['type'] == "std::string":
		tmp += "|type=string"
		tmp += "|defvalue=\"" + str(defval) + "\""
	elif t['type'] == "bool":
		tmp += "|type=" + t['type']
		tmp += "|defvalue=" + ("true" if (defval > 0) else "false")
	else:
		tmp += "|type=" + t['type']
		tmp += "|defvalue=" + str(defval)
	if "minimumValue" in t:
		tmp += "|minvalue=" + str(t['minimumValue'])
	if "maximumValue" in t:
		tmp += "|maxvalue=" + str(t['maximumValue'])
	desc=""
	if "description" in t:
		desc = "|description=" + t['description'].encode("utf8")
	print "{{ConfigValue", tmp, desc, "}}"

EOF
)

TEMPLATE_CONTENT=`echo "$SPRINGCFG_JSON" | python2 -c "$PYCODE"`
TEMPLATE_CONTENT=$(echo -e "=${SECTIONNAME}=\n<center><span class=warning>'''THIS SECTION IS AUTOMATICALLY GENERATED! DON'T EDIT IT!'''</span></center>\n${TEMPLATE_CONTENT}")

if [ $? != 0 ]; then
	echo "python parsing failed"
	exit 1
fi

#########################
# Upload
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE}" "${SECTIONNAME}" "$TEMPLATE_CONTENT"

exit $?
