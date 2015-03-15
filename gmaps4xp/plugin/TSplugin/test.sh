#!/bin/bash



stat="$( wget -q -O- "http://www.ivao.aero/whazzup/status.txt" | tr -d "\r" | grep "url0="  | grep -v ".gz" | awk -F\= {'print $2'} )"

for s in  $stat ; do
	wget -q "$s" -O-
	break
done
