#!/bin/bash

#########################
# Config
USERNAME=$1
USERPASS=$2
PAGE=$3
SECTION=$4
CONTENT=$5
WIKIAPI="https://springrts.com/mediawiki/api.php"
cookie_jar="wikicj"

#########################
# Login
echo "Logging into $WIKIAPI as $USERNAME..."

#Login part 1
echo "Logging in (1/2)..."
CR=$(curl -s \
        --location \
        --retry 2 \
        --retry-delay 5\
        --cookie $cookie_jar \
        --cookie-jar $cookie_jar \
        --user-agent "Curl Shell Script" \
        --keepalive-time 60 \
        --header "Accept-Language: en-us" \
        --header "Connection: keep-alive" \
        --compressed \
        --data-urlencode "lgname=${USERNAME}" \
        --data-urlencode "lgpassword=${USERPASS}" \
        --request "POST" "${WIKIAPI}?action=login&format=txt")

CR2=($CR)
if [ "${CR2[9]}" = "[token]" ]; then
        TOKEN=${CR2[11]}
else
        echo "Login part 1 failed."
        echo $CR
        exit 1
fi

#Login part 2
echo "Logging in (2/2)..."
CR=$(curl -s \
        --location \
        --cookie $cookie_jar \
        --cookie-jar $cookie_jar \
        --user-agent "Curl Shell Script" \
        --keepalive-time 60 \
        --header "Accept-Language: en-us" \
        --header "Connection: keep-alive" \
        --compressed \
        --data-urlencode "lgname=${USERNAME}" \
        --data-urlencode "lgpassword=${USERPASS}" \
        --data-urlencode "lgtoken=${TOKEN}" \
        --request "POST" "${WIKIAPI}?action=login&format=txt")

#TODO-Add login part 2 check
echo "Successfully logged in as $USERNAME."


#########################
# Fetch Token

echo "Fetching edit token..."
CR=$(curl -s \
        --location \
        --cookie $cookie_jar \
        --cookie-jar $cookie_jar \
        --user-agent "Curl Shell Script" \
        --keepalive-time 60 \
        --header "Accept-Language: en-us" \
        --header "Connection: keep-alive" \
        --compressed \
	--form "prop=info" \
	--form "intoken=edit" \
	--form "titles=$PAGE" \
        --request "POST" "${WIKIAPI}?action=query&format=txt")

getvalue() {
	STR=${1:-""}
	KEY=${2:-""}

	if [[ ! `echo "$STR" | grep "$KEY"` ]]; then
		echo ${3:-""}
		return
	fi

	STR=${STR##*${KEY}}
	STR=${STR%% *};

	echo ${STR:-${4:-""}}
}

CR2=($CR)
EDITTOKEN=`getvalue "$CR" "\[edittoken\] => " "parse error"`
if [ ${#EDITTOKEN} = 34 ]; then
        echo "Edit token is: $EDITTOKEN"
else
        echo "Edit token not set."
        echo $CR
        exit 1
fi

#########################
# Translate section name to number

if [ "$SECTION" != "" ]; then
	CR=$(curl -s \
		--location \
		--cookie $cookie_jar \
		--cookie-jar $cookie_jar \
		--user-agent "Curl Shell Script" \
		--keepalive-time 60 \
		--header "Accept-Language: en-us" \
		--header "Connection: keep-alive" \
		--header "Expect:" \
		--form "page=$PAGE" \
		--form "prop=sections" \
		--request "POST" "${WIKIAPI}?action=parse&format=json&")

	PYCODE="import json, sys
d=json.load(sys.stdin)
s=d['parse']['sections']
for t in s:
	if 'line' in t:
		if t['line'] == '$SECTION':
			print str(t['index'])
			exit
	"

	SECTION_NUM=`echo "$CR" | python2 -c "$PYCODE"`

	if [ $? != 0 ] || [ "$SECTION_NUM" == "" ]; then
		echo "python-json section parsing failed"
		#echo "$CR"
		exit 1
	fi

	echo "Section number for \"${SECTION}\" is \"${SECTION_NUM}\""
fi

#########################
# Upload

if [ -f "$CONTENT" ]; then
	CR=$(curl -s \
		--location \
		--cookie $cookie_jar \
		--cookie-jar $cookie_jar \
		--user-agent "Curl Shell Script" \
		--keepalive-time 60 \
		--header "Accept-Language: en-us" \
		--header "Connection: keep-alive" \
		--header "Expect:" \
		--form "title=$PAGE" \
		--form "bot=1" \
		--form "text=<$CONTENT" \
		--form "token=${EDITTOKEN}" \
		--request "POST" "${WIKIAPI}?action=edit&format=txt&")
		#FIXME --form "section=${SECTION_NUM-0}" \
else
	CR=$(curl -s \
		--location \
		--cookie $cookie_jar \
		--cookie-jar $cookie_jar \
		--user-agent "Curl Shell Script" \
		--keepalive-time 60 \
		--header "Accept-Language: en-us" \
		--header "Connection: keep-alive" \
		--header "Expect:" \
		--form "title=$PAGE" \
		--form "bot=1" \
		--form "text=$CONTENT" \
		--form "section=${SECTION_NUM-0}" \
		--form "token=${EDITTOKEN}" \
		--request "POST" "${WIKIAPI}?action=edit&format=txt&")
fi

echo $CR

exit 0
