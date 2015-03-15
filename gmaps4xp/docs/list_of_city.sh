#!/bin/bash

# http://worldaerodata.com/wad.cgi?id=IT72564&sch=bologna

xml="$( wget --timeout=10 --tries=1 --user-agent=Firefox -q -O- "http://ws.geonames.org/cities?north=44.920523&south=44.623974&east=11.973813&west=11.355621" )"

list_city=$( echo "$xml" | grep "<name>" | sed -e s/"<name>"/""/g | sed -e s/"<\/name>"/";"/g | tr " " "_" | tr -d "\n" )
list_city=( $( echo "$list_city" | tr ";" " " ) )

for i in ${list_city[*]} ; do
	city="$( echo "$i" | sed -e s/"_"/"%20"/g )"
	echo -n "$city: " | sed -e s/"%20"/" "/g
	info="$( wget --timeout=10 --tries=1 --user-agent=Firefox -q -O- --post-data "key=$city" "http://www.postmodern.com/cgi-bin/airports.pl" | grep -i "$city" | grep -vi "<H" )"
	echo "$info"

	echo
done
