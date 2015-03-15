#!/bin/bash


# north=44.920523&south=44.623974&east=11.973813&west=11.355621
BASE_URL="http://www.informationfreeway.org/api/0.6"
BASE_URL="http://xapi.openstreetmap.org/api/0.5"


left="11.355621"
bottom="44.623974"
right="11.973813"
top="44.920523"

# ROAD DEFINITION

ROAD_TYPE="
railway-rail		56
highway-residential	13
highway-secondary	44
highway-tertiary	47

highway-unclassified	51
"
 

QUERY_URL="${BASE_URL}/map?bbox=$left,$bottom,$right,$top"

#wget -O output.log "$QUERY_URL"

XML_OUTPUT="$( cat output.log )"

echo "Get ways list..." 1>&2
WAYS_START=( $( echo "$XML_OUTPUT" | grep -n "<way id=" | awk -F:  {'print $1'} | tr "\n" " " ) )
WAYS_END=(   $( echo "$XML_OUTPUT" | grep -n "</way>"   | awk -F:  {'print $1'} | tr "\n" " " ) )

echo "Get nodes list..." 1>&2
NODES="$(       echo "$XML_OUTPUT" | grep "node id="    | awk -F\' {'print $2" "$4" "$6'} )"

i="0"
begin="621"

while [ ! -z "${WAYS_START[$i]}" ]  ; do

	CONTENT="$( echo "$XML_OUTPUT" 	| head -n "${WAYS_END[$i]}" | tail -n $[ ${WAYS_END[$i]} - ${WAYS_START[$i]} + 1 ] )" 


	TAG_KEY=( $(    echo "$CONTENT" | grep "<tag k="  | awk -F\' {'print $2'} | tr "\n" " " ) )
	TAG_VALUE=( $(	echo "$CONTENT"	| grep "<tag k="  | awk -F\' {'print $4'} | tr "\n" " " ) )

	if [ ! -z "$( echo "${TAG_KEY[@]}" | grep -i "waterway" )" ] ; then
		echo "waterway skip..."	1>&2
		i="$[ $i + 1 ]"
		continue
	fi
	if [ ! -z "$( echo "${TAG_KEY[@]}" | grep -i "cycleway" )" ] ; then
		echo "cycleway skip..."	1>&2
		i="$[ $i + 1 ]"
		continue
	fi


	rtype="$( echo "$ROAD_TYPE" | grep "^${TAG_KEY[0]}-${TAG_VALUE[0]}" | awk {'print $2'} )"


	if [ -z "$rtype" ] ; then
		echo "Road ${TAG_KEY[0]}-${TAG_VALUE[0]} unknown"
		exit
	fi

	REF="$(     echo "$CONTENT" 	| grep "<nd ref=" | awk -F\' {'print $2'} | tr "\n" " " )"

	cnt="0"
	WAY=()
	for r in $REF ; do
		echo -n "." 1>&2
		WAY[$cnt]="$( echo "$NODES" | grep "^$r" | awk {'print $3" "$2'} )"
		cnt=$[ $cnt + 1 ]
	done

	cnt="1"
	echo
	echo "BEGIN_SEGMENT 0 $rtype $begin ${WAY[0]} 10.00000000"
	while [ "$cnt" -lt $[ ${#WAY[@]} - 1 ] ] ; do		
		echo "SHAPE_POINT ${WAY[$cnt]} 10.00000000"
		cnt=$[ $cnt + 1 ]
	done
	begin=$[ $begin + 1 ]
	echo "END_SEGMENT $begin ${WAY[$cnt]} 10.00000000"
	begin=$[ $begin + 1 ]


	i="$[ $i + 1 ]"
	echo 1>&2
	echo "--------" 1>&2
done



