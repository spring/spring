#!/bin/sh
# Author: Tobi Vollebregt
#
# Fetches TASClient (only on first run or if TASClient has been updated).
#

[ -e TASClient.txt.cur ] && cp TASClient.txt.cur TASClient.txt.old
wget -q http://www.climachine.com/spring/TASClient.txt -O TASClient.txt.cur

echo -n "old: " ; cat TASClient.txt.old ; echo
echo -n "new: " ; cat TASClient.txt.cur ; echo

if ! cmp -s TASClient.txt.cur TASClient.txt.old; then
	wget -q `awk '{print $2;}' < TASClient.txt.cur` -O TASClient.7z &&
		7z x -y TASClient.7z
fi
