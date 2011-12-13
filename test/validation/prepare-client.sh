#!/bin/sh

set -e #abort on error

if [ $# -ne 3 ]; then
	echo "Usage: $0 Username IP Port"
	exit 1
fi
USER="$1"
IP="$2"
PORT="$3"

cat <<EOD
[GAME]
{
        HostIP=$IP;
        HostPort=$PORT;
        IsHost=0;
        MyPlayerName=$USER;
}
EOD
