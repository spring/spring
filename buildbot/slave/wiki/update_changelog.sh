#!/bin/sh

#########################
# Config
SPRING="spring"
PAGE="Changelog"
SECTIONNAME=""

if [ ! -f ~/.ssh/spring_wiki_account ]; then
	echo "couldn't find ~/.ssh/spring_wiki_account with proper username & password!"
	echo "please create with `echo \"$USERNAME:$PASSWORD\" > ~/.ssh/spring_wiki_account`"
	exit 1
fi
USERANDPASS=$(cat ~/.ssh/spring_wiki_account)
USERNAME=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[1]}')
USERPASS=$(echo "${USERANDPASS}" | awk '{split($0,a,":"); print a[2]}')

#. buildbot/slave/prepare.sh $*
SPRING="${BUILDDIR}/${SPRING}"

#########################
# Parse & Transform JSON

TEMPLATE_CONTENT=`cat doc/changelog.txt`

# skip header (content starts with first -)
TEMPLATE_CONTENT=`echo "$TEMPLATE_CONTENT" |
awk '{ if (content || match($0, /^-.*/)) content=1; if (content==1) print; }'`

# replace (works for "!" too):
# - foo
#    bar
# with
# - foo<br>bar
TEMPLATE_CONTENT=`echo "$TEMPLATE_CONTENT" |
sed '{:q;N;s/\r\n/\n/g;t q}' |
sed -r 's/^\s+([^-! ].*)/<br>\1/' |
sed '{:q;N;s/\n/<newline>/g;t q}' |
sed 's/<newline><br>/<br>/g' |
sed 's/<newline>/\n/g'`

# find release headers
# "-- 95.0 --...--" -> "=95.0="
TEMPLATE_CONTENT=`echo "$TEMPLATE_CONTENT" |
awk '{ if (match($0, /^--.*--/)) { gsub(/^-*\s?/, "="); gsub(/\s?-*$/, "="); } print }'`

# translate `items` to wiki syntax
# " - item" -> "*<span>item</span>"
# " ! item" -> "*<span class=warning>item</span>"
# " - itemParent" -> "*<span>itemParent</span>"
# "  - itemSub"   -> ":*<span>itemSub</span>"
TEMPLATE_CONTENT=`echo "$TEMPLATE_CONTENT" |
awk 'function rep(str, num,     remain, result) {
    if (num < 2) {
        remain = (num == 1)
    } else {
        remain = (num % 2 == 1)
        result = rep(str, (num - remain) / 2)
    }
    return result result (remain ? str  : "")
}
{ j=0; leadingSpaces=$0; gsub(/[^ ].*/,"", leadingSpaces); j=gsub(/^(\s*)[-]/,"*<span>"); j+=gsub(/^(\s*)[!]/,"*<span class=warning>"); printf "%s%s%s\n", rep(":", length(leadingSpaces)-1), $0, rep("</span>", j)}'`

# find section headers
# "Misc:" -> "==Misc=="
TEMPLATE_CONTENT=`echo "$TEMPLATE_CONTENT" |
awk '{ if (match($0, /^[^:=*].*:/)) { gsub(/:/,""); printf "==%s==\n", $0;} else print }'`


TEMPLATE_CONTENT="__NOTOC__<b>
$TEMPLATE_CONTENT</b>[[category:Development]]"

#########################
# Upload
echo "$TEMPLATE_CONTENT" > /tmp/spring_changelog # the string is too long to pass by argument
$(dirname $0)/mediawiki_upload.sh "$USERNAME" "$USERPASS" "${PAGE}" "${SECTIONNAME}" "/tmp/spring_changelog"
rm /tmp/spring_changelog

exit $?
