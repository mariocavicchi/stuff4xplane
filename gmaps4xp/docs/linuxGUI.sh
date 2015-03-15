#!/bin/bash

win="$( which zenity 2> /dev/null )"

if [ -z "$win" ] ; then
	echo "Zenity missing... I can't work!"
	exit 1
fi

KML_FILE="$( $win  --title="Select KML file..."  --file-selection --file-filter=*.kml )"
[ -z "$KML_FILE" ] && exit 2


OUTPUT_DIRECTORY="$( $win  --title="Select output Directory..."  --file-selection --directory )"
[ -z "$OUTPUT_DIRECTORY" ] && exit 2
