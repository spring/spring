#!/bin/bash

#########################
# Config
SPRING="spring"
PAGE="Springsettings.cfg"
SECTIONNAME="Available Options"

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

#TODO also reparse doc of previous spring and then check for new ones and mark them on the wiki (and maybe removed ones too)
SPRINGCFG_JSON=`$SPRING --list-config`

PYCODE=$(cat <<EOF
import json, sys
d=json.load(sys.stdin)
for cfgtag,t in sorted(d.iteritems()):
	if cfgtag == 'test':
		continue
	tmp="|name=" + cfgtag
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
	if "safemodeValue" in t:
		if t['type'] == "bool":
			tmp += "|safevalue=" + ("true" if (t['safemodeValue'] > 0) else "false")
		else:
			tmp += "|safevalue=" + str(t['safemodeValue'])
	else:
		# add empty, so an empty column is shown (w/o this attr the column is hidden)
		tmp += "|safevalue="
	desc=""
	if "description" in t:
		desc = "|description=" + t['description'].encode("utf8")
	else:
		desc = "|description=[" + t['declarationFile'].replace("trunk/", "https://github.com/spring/spring/blob/develop/", 1) + "#L" + str(t['declarationLine']) + " source pos]"
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
