#!/bin/bash

# Compatibilty for macosx
wget="wget"
dsftool=""
if [ "$( uname -s )" = "Darwin" ] ; then
        # Dsf tool
        dsftool="$( dirname "$0" )/ext_app/xptools_apr08_mac/DSFTool"

        # convert command
        export MAGICK_HOME="$( dirname "$0" )/ext_app/ImageMagick-6.4.2"
        export PATH="$MAGICK_HOME/bin:$PATH"
        export DYLD_LIBRARY_PATH="$MAGICK_HOME/lib"

        # wget command
        wget="$( dirname "$0" )/ext_app/wget/wget"
        # sed command
        seq(){
                cnt="$1"
                end=$[ $2 + 1 ]

                [  -z "$2" ] && end="$[ $1 + 1 ]" && cnt="1"

                while [ "$cnt" != "$end" ] ; do
                        echo "$cnt"
                        cnt="$[ $cnt + 1 ]"
                done

        }
fi
if [ "$( uname -s )" = "Linux" ] ; then
        # Dsf tool

        # set wine env
        export WINE="$( dirname "$0" )/ext_app/wine/usr"
        export PATH="$WINE/bin:$PATH"
        export LD_LIBRARY_PATH="$WINE/lib"
        dsftool="$( dirname "$0" )/ext_app/xptools_apr08_win/DSFTool.exe"
fi

if [ -z "$2" ] ; then
	echo "Usage: $(basename $0 ) \"/path/to/scenary/Earth nav data/xxxxxxx/textfile.dsf.txt\" \"Output Dirctory\""
	exit 1
fi



tiles_dir="$( dirname "$0" )/cache"
if [ -d "$tiles_dir" ] ; then
        echo "Directory \"$tiles_dir\" already exits..."
else
        echo "Create directory \"$tiles_dir\"..."
        mkdir "$tiles_dir"
fi



output_index="0"
output=()


ALTITUDE_DB=()
ALTITUDE_INDEX="0"

pointInPolygon(){
        x="${1%,*}"
        y="${1#*,}"
        polySides=( $( echo $2 ) )
        oddNodes="out"
        i="0"
        for xy in  ${polySides[*]} ; do
                polyX[$i]="${xy%,*}" 
                polyY[$i]="${xy#*,}"
                i=$[ $i + 1 ]
        done

        i="0"
        j="$[ ${#polyX[*]} - 1 ]"
        if [ "${polyX[0]}" = "${polyX[$j]}" ] && [ "${polyY[0]}" = "${polyY[$j]}" ] ; then
                unset polyX[$j]
                unset polyY[$j]
                j="$[ ${#polyX[*]} - 1 ]"

        fi
        
        while [ $i -lt "${#polyX[*]}"  ] ;  do
                if [ "$( echo "scale=8;  ${polyY[$i]} < $y && ${polyY[$j]} >= $y  || ${polyY[$j]} < $y && ${polyY[$i]} >= $y" | bc -l )" == 1 ] ; then
                        if [ "$( echo "scale=8; ${polyX[$i]} + ($y - ${polyY[$i]})/(${polyY[$j]} - ${polyY[$i]})*(${polyX[$j]} - ${polyX[$i]}) < $x" | bc -l )" == 1 ] ; then
                                if [ "$oddNodes" = "out" ] ; then
                                        oddNodes="in"
                                else
                                        oddNodes="out"
                                fi
                        fi

                fi
                j="$i";
                i=$[ $i + 1 ]
        done
        echo "$oddNodes"
}



addLine(){
	line="$1"
	[ -z "$line" ] && line=" "

	output[$output_index]="$line"
	output_index=$[ $output_index + 1 ]
}

makeWaterMesh(){
	UL=( $2 $1 )
	UR=( $4 $3 )
	LR=( $6 $5 )
	LL=( $8 $7 )

	echo -n "${LL[@]}" | tr " " ","
	echo -n " "
	echo -n "${UR[@]}" | tr " " ","
	echo -n " "
	echo -n "${LR[@]}" | tr " " ","
	echo -n " "
	echo -n "${LL[@]}" | tr " " ","
	echo -n " "
	echo -n "${UL[@]}" | tr " " ","
	echo -n " "
	echo -n "${UR[@]}" | tr " " ","


}

getAltitude(){
	lon="$1"
	lat="$2"

	if [ -f "$tiles_dir/info_${lon}_${lat}.alt" ] ; then
		out="$( cat "$tiles_dir/info_${lon}_${lat}.alt" )" 
		[ -z "$out" ] && out="99999" && rm -f  "$tiles_dir/info_${lon}_${lat}.alt"
	fi
	if [ ! -f "$tiles_dir/info_${lon}_${lat}.alt" ] ; then
		tmp="$( "$wget" -q -O- "http://gisdata.usgs.gov/xmlwebservices2/elevation_service.asmx/getElevation?X_Value=$lon&Y_Value=$lat&Elevation_Units=METERS&Source_Layer=-1&Elevation_Only=true" )"

		out="$( echo "$tmp" | sed -e s/"<double>"/":"/g | sed -e s/"<\/double>"/":"/g | awk -F: {'print $2'} | tr "[a-z][A-Z]" " " | tr -d "\n " )"
		[ -z "$out" ] && out="99999"
		echo -n "$out" > "$tiles_dir/info_${lon}_${lat}.alt" 
	fi
	if [ "$out" != "${out#*.}" ] ; then
		out="${out%.*}.$( echo "${out#*.}" | cut -c -8 )"
	else
		out="$out.00000000" 
	fi
	echo -n "$out"
}

checkAltitude(){
	lon="$1"
	lat="$2"
	alt="$3"
	file="info_${lon}_${lat}.alt"

	while [ ! -z "$lon" ]  ; do
		#file_list="$( find "$tiles_dir/" -name \*.alt | grep -- "${lon}" | grep -- "${lat}" )"
		file_list="$( find "$tiles_dir/" -name "info_${lon}*.alt" | grep -- "${lat}" | rev | cut -f -1 -d "/" | rev )"
		num="$( echo "$file_list" | wc -l )"
		[ "$( echo "scale=6; $num > 1" | bc )" = "1" ] && break
		lon="${lon:0:$[ ${#lon} -1 ]}"

	#	lat="${lat:0:$[ ${#lat} -1 ]}"
	done

	file_list="$( echo "$file_list" | grep -v "file" )"
	sum="0"
	for i in $file_list ; do
		sum="$( echo "scale=8; $sum + $( cat "$tiles_dir/$i" )" | bc )" 
	done
	avarage="$( echo "scale=8; $sum / $num" | bc )"
	 [ "$( echo "scale = 6; ( $avarage == 0 )" | bc )" = "1" ] && echo "0.00000000" && return
	
	disc="$( echo "scale=8; ( $alt - $avarage ) / $avarage * 100" | bc )"

	if [ "$( echo "scale = 6; ( ( $disc < 10.0 ) || ( $disc > 10.0 ) )" | bc )" = "1" ] ; then
		echo -n "$avarage"
	else
		echo -n "$alt"
	fi
}


middlePoint(){
	points=( $* )

	points[0]="$( echo "${points[0]}" | tr "_" " " | tr "," "\t" )"
	points[1]="$( echo "${points[1]}" | tr "_" " " | tr "," "\t" )"

	point_one=( $( echo "${points[0]}" | awk {'print $1" "$2'} ) )
	point_two=( $( echo "${points[1]}" | awk {'print $1" "$2'} ) )

	pos_one=( $( echo "${points[0]}" | awk {'print $3" "$4'} ) )
	pos_two=( $( echo "${points[1]}" | awk {'print $3" "$4'} ) )


	point_tree_lon="$( echo "scale = 8; ( ${point_one[0]} + ${point_two[0]} ) / 2" | bc )"
	[ -z "$( echo "${point_tree_lon%.*}" | tr -d "-" )" ] && point_tree_lon="$( echo "$point_tree_lon" | sed -e s/"\."/"0\."/g )"

	point_tree_lat="$( echo "scale = 8; ( ${point_one[1]} + ${point_two[1]} ) / 2" | bc )"
	[ -z "$( echo "${point_tree_lat%.*}" | tr -d "-" )" ] && point_tree_lat="$( echo "$point_tree_lat" | sed -e s/"\."/"0\."/g )"

	pos_tree_x="$( echo "scale = 8; ( ${pos_one[0]} + ${pos_two[0]} ) / 2" | bc )"
	[ -z "$( echo "${pos_tree_x%.*}" | tr -d "-" )" ] && pos_tree_x="$( echo "$pos_tree_x" | sed -e s/"\."/"0\."/g )"
	[ "$pos_tree_x" = "0" ] && pos_tree_x="0.00000000"

	pos_tree_y="$( echo "scale = 8; ( ${pos_one[1]} + ${pos_two[1]} ) / 2" | bc )"
	[ -z "$( echo "${pos_tree_y%.*}" | tr -d "-" )" ] && pos_tree_y="$( echo "$pos_tree_y" | sed -e s/"\."/"0\."/g )"
	[ "$pos_tree_y" = "0" ] && pos_tree_y="0.00000000"

	echo -n "${point_tree_lon},${point_tree_lat}_${pos_tree_x},${pos_tree_y};"
}

divideSquare(){
	coorners=( $* )
	
	echo -n "${coorners[0]};"
	middlePoint ${coorners[0]} ${coorners[1]}
	middlePoint ${coorners[1]} ${coorners[3]}
	middlePoint ${coorners[3]} ${coorners[0]}
	echo -n " "

	middlePoint ${coorners[0]} ${coorners[1]}
	echo -n "${coorners[1]};"
	middlePoint ${coorners[1]} ${coorners[2]}
	middlePoint ${coorners[1]} ${coorners[3]}
	
	echo -n " "
	middlePoint ${coorners[1]} ${coorners[3]}
	middlePoint ${coorners[1]} ${coorners[2]}
	echo -n "${coorners[2]};"
	middlePoint ${coorners[2]} ${coorners[3]}

	echo -n " "
	middlePoint ${coorners[3]} ${coorners[0]}
	middlePoint ${coorners[1]} ${coorners[3]}
	middlePoint ${coorners[2]} ${coorners[3]}
	echo -n "${coorners[3]};"

}

pointsDist(){
	a=(  ${1#*,} ${1%,*} ) 
	b=(  ${2#*,} ${2%,*} ) 
		
	dist="$( echo "scale = 8; sqrt( ( ( ${b[1]} - ${a[1]} ) * ( ${b[1]} - ${a[1]} ) ) + ( ( ${b[0]} - ${a[0]} ) * ( ${b[0]} - ${a[0]} ) ) )" | bc -l )"

	[ -z "$( echo "${dist%.*}" | tr -d "-" )" ] && dist="$( echo "$dist" | sed -e s/"\."/"0\."/g )"
	echo -n "$dist"
}


mostNearPoint(){
	from="${1#*,}"
	list=( $( echo $2 ) )

	cnt="0"
	index="$cnt"
	dist="$( pointsDist "$from" "${list[$cnt]}" )"
	while [ ! -z "${list[$cnt]}" ] ; do
		new_dist="$( pointsDist "$from" "${list[$cnt]}" )"
		[ "$( echo "scale = 8; $new_dist < $dist" | bc )" = "1" ] && dist="$new_dist" && index="$cnt"
		cnt="$[ $cnt + 1 ]"
	done
	echo -n "${list[$index]}"
}

################################################################################################################33

createKMLoutput(){
        ACT="$1"
        file="$2"
        if [ -z "$file" ] ; then
                echo "ERROR: missing file name"
                return
        fi
        if [ "$ACT" = "HEAD" ] ; then
                title="$3"
                [ -z "$title" ] && title="Untitled"
                echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" > "$file"
                echo "<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\" xmlns:kml=\"http://www.opengis.net/kml/2.2\" xmlns:atom=\"http://www.w3.org/2005/Atom\">" >> "$file"
                echo "<Folder>" >> "$file"
                echo -ne "\t<name>$title</name>\n" >> "$file"

                echo -ne "\t<Style id=\"yellowLineGreenPoly\">\n" >> "$file"
                echo -ne "\t\t<LineStyle>\n" >> "$file"
                echo -ne "\t\t\t<color>7f00ffff</color>\n" >> "$file"
                echo -ne "\t\t\t<width>4</width>\n" >> "$file"
                echo -ne "\t\t</LineStyle>\n" >> "$file"
                echo -ne "\t\t<PolyStyle>\n" >> "$file"
                echo -ne "\t\t\t<color>7f00ff00</color>\n" >> "$file"
                echo -ne "\t\t</PolyStyle>\n" >> "$file"
                echo -ne "\t</Style>\n" >> "$file"

        fi
        if [ "$ACT" = "ADD" ] ; then
                if [ "$#" -lt 5 ] ; then
                        echo "ERROR: missing arguments"
                fi
		                
		args=( $* )
		i="2"
		j="0"
		while [ ! -z "${args[$i]}" ] ; do
			k="$[ $i + 1 ]"
			gmapspoly[$j]="${args[$i]},${args[$k]},0"
			j="$[ $j + 1 ]"
			i="$[ $i + 2 ]"
		done

                echo -ne "\t<Placemark>\n"  >> "$file"
                echo -ne "\t\t<name>Triangle</name>\n" >> "$file"
                echo -ne "\t\t<styleUrl>#yellowLineGreenPoly</styleUrl>\n"  >> "$file"
                echo -ne "\t\t<LineString>\n"  >> "$file"
                echo -ne "\t\t\t<coordinates>${gmapspoly[@]} ${gmapspoly[0]}</coordinates>\n"  >> "$file"

                echo -ne "\t\t</LineString>\n"  >> "$file"
                echo -ne "\t</Placemark>\n"  >> "$file"


        fi
	if [ "$ACT" = "PNT" ] ; then
		lat="${3#*,}" 
		lon="${3%,*}"

                echo -ne "\t<Placemark>\n"  >> "$file"
                #echo -ne "\t\t<name>Point</name>\n" >> "$file"
                echo -ne "\t\t<Point>\n"  >> "$file"
                echo -ne "\t\t\t<coordinates>$lon,$lat,0</coordinates>\n"  >> "$file"
		echo -ne "\t\t</Point>\n"  >> "$file"
		echo -ne "\t</Placemark>\n"  >> "$file"


	fi

        if [ "$ACT" = "END" ] ; then
        
                echo "</Folder>"  >> "$file"
                echo "</kml>"  >> "$file" 
        fi

}                         


TXTfiles="$1"
OUTPUT_DIR="$2"

echo "Read file..."
content="$( cat "$TXTfiles" )"
TER_DIR="terrain"
KML_FILE="$OUTPUT_DIR/triangles.kml"

echo "Add head file..."
addLine "A"
addLine "800"
addLine "DSF2TEXT"
addLine 

OUTPUT_FILE="$( basename -- "$TXTfiles" | sed -e s/".txt"/""/g )"

echo "Gets PROPERTY..."
PROPERTY="$( echo "$content" | grep "PROPERTY" )"
if [ -z "$PROPERTY" ] ; then
	echo "No PROPERTY found..."
	exit 1
fi
for i in west east north south planet ; do
	line="$( echo "$PROPERTY" | grep "$i" )"
	if [ -z "$line" ] ; then
		echo "No PROPERTY $i found..."
		exit 3
	fi
	addLine "$( echo "$PROPERTY" | grep "$i" )"
	line="$( echo "$line" | awk {'print $3'} ).00000000"
	case $i in
		north)
			TILE[0]="$line" 
		;; 
		south)
			TILE[1]="$line" 
		;; 

		east)
			TILE[2]="$line" 
		;; 
		west)
			TILE[3]="$line"
		;;

	esac	
done
addLine "sim/creation_agent $( basename -- "$0" )"
addLine

echo "Poligon list..."
POLYGON_DEF=( $( echo "$content" | grep "POLYGON_DEF" | awk {'print $2'} ) )
if [ -z "$POLYGON_DEF" ] ; then
	echo "No POLYGON_DEF found..."
	exit 2
fi
echo "Search .png and .pol file..."
POLYGON_DIR="$( dirname -- "$TXTfiles" )"
if [ -z "$( find "$POLYGON_DIR" -name ${POLYGON_DEF[0]} )" ] ; then
	while [ "$POLYGON_DIR" != "." ] ; do
		POLYGON_DIR="$( dirname -- "$POLYGON_DIR" )"
		[ ! -z "$( find "$POLYGON_DIR" -name ${POLYGON_DEF[0]} )" ] && break
	done
fi
if [ ! -f "$POLYGON_DIR/${POLYGON_DEF[0]}" ] ; then
	echo "No .pol files found..."
	exit 4

fi
echo "Found .pol files in $POLYGON_DIR..."

PNG_DEF=( $( echo "${POLYGON_DEF[@]}" | sed -e s/".pol"/".png"/g ) )

PNG_DIR="$( dirname -- "$TXTfiles" )"
if [ -z "$( find "$PNG_DIR" -name ${PNG_DEF[0]} )" ] ; then
	while [ "$PNG_DIR" != "." ] ; do
		PNG_DIR="$( dirname -- "$PNG_DIR" )"
		[ ! -z "$( find "$PNG_DIR" -name ${PNG_DEF[0]} )" ] && break
	done
fi
if [ ! -f "$PNG_DIR/${PNG_DEF[0]}" ] ; then
	echo "No .png files found..."
	exit 5

fi

echo "Found .png files in $PNG_DIR..."

addLine

echo "Create directory tree..."

COORD_DIR="$( basename -- "$( dirname -- "$TXTfiles" )" )"

TER_DIR="$OUTPUT_DIR/$TER_DIR"
OUTPUT_DIR="$OUTPUT_DIR/Earth nav data/$COORD_DIR"
if [ ! -d  "$OUTPUT_DIR" ] ; then
	echo "Create directory $OUTPUT_DIR..."
	mkdir -p -- "$OUTPUT_DIR"
else
	echo "Directory $OUTPUT_DIR already exists..."
fi
if [ ! -d  "$TER_DIR" ] ; then
	echo "Create directory $TER_DIR..."
	mkdir -p -- "$TER_DIR"
else
	echo "Directory $TER_DIR already exists..."
fi



coordinates=( $( echo "$content" | grep "POLYGON_POINT" | awk {'print $2","$3'} | sort -u | sort -n | tr "\n" " " ) )

lon="$( echo "${coordinates[0]}" | awk -F, {'print $1'} )"
lat="$( echo "${coordinates[0]}"  | awk -F, {'print $2'} )"

min_longitude="$lon"
max_longitude="$lon"
min_latitude="$lat"
max_latitude="$lat"

min_lon_id="0"
max_lon_id="0"
min_lat_id="0"
max_lat_id="0"


cnt="0"
num="${#coordinates[@]}"
for coord in ${coordinates[@]} ; do
        lon="$( echo "$coord" | awk -F, {'print $1'} )"
        lat="$( echo "$coord"  | awk -F, {'print $2'} )"
        alt="$( getAltitude $lon $lat )"


	[ -z "$min_altitude" ] && min_altitude="$alt"
        [ "$( echo "scale = 8; $alt < $min_altitude" | bc )" = "1" ] && min_altitude="$alt"

        #altitude[$cnt]="$lon,$lat,$alt"
        echo -ne "$[ $cnt + 1 ] / $num \t$lon / $lat -> $alt meters         \r"

        [ "$( echo "scale = 8; $lon < $min_longitude" | bc )" = "1" ] 	&& min_longitude="$lon" && min_lon_id="$cnt"
        [ "$( echo "scale = 8; $lon > $max_longitude" | bc )" = "1" ] 	&& max_longitude="$lon"	&& max_lon_id="$cnt"
                                                                                                                    
        [ "$( echo "scale = 8; $lat < $min_latitude" | bc )" = "1" ] 	&& min_latitude="$lat"	&& min_lat_id="$cnt"
        [ "$( echo "scale = 8; $lat > $max_latitude" | bc )" = "1" ] 	&& max_latitude="$lat"	&& max_lat_id="$cnt"
        cnt=$[ $cnt + 1 ]
done
echo
echo "BOX coordiantes:"
echo "WEST: $min_longitude EAST: $max_longitude"
echo "SOUTH: $min_latitude NORTH: $max_latitude"
echo "MIN ALTITUDE: $min_altitude"

echo "$min_lon_id  $max_lon_id $min_lat_id $max_lat_id"

poly=( ${coordinates[$min_lon_id]} ${coordinates[$min_lat_id]} ${coordinates[$max_lon_id]} ${coordinates[$max_lat_id]} )

echo "Search scenary perimeter..."
cnt="0"
while [ ! -z "${coordinates[$cnt]}" ] ; do
	inout="$( pointInPolygon "${coordinates[$cnt]}" "${poly[*]}" )"  
	echo -ne "$[ $cnt + 1 ] / $num   \r"

	[ "$inout" = "out" ] &&	poly=( $( echo "${poly[*]}" ) ${coordinates[$cnt]} )

	for point in ${poly[*]} ; do
		inout="$( pointInPolygon "$point" "${poly[*]}" )"
		[ "$inout" = "in" ] && poly=( $( echo "${poly[*]}" | tr " " "\n" | grep -v "$point" | tr "\n" " " ) )

	done


	cnt=$[ $cnt + 1 ]
done


X_STEP="50"
Y_STEP="50"
TOLERANCE="0.01"

lon_step="$( echo "scale = 8; ( ${TILE[2]} - ${TILE[3]} ) / $X_STEP" | bc )"
lat_step="$( echo "scale = 8; ( ${TILE[0]} - ${TILE[1]} ) / $Y_STEP" | bc )"

cnt="0"
lat_cursor="${TILE[1]}"
echo "Create grid of points..."
for y in $( seq 0 $Y_STEP ) ; do
	lon_cursor="${TILE[3]}"
	for x in $( seq 0 $X_STEP ) ; do
		if [ "$( echo "scale = 8; ( $lon_cursor > $min_longitude ) && ( $lon_cursor < $max_longitude ) && ( $lat_cursor > $min_latitude ) && ( $lat_cursor < $max_latitude )" | bc )" = "1" ] ; then
			#grid[$cnt]="$lon_cursor,$lat_cursor"
			grid[$cnt]="none,$lon_cursor,$lat_cursor"
		else
			grid[$cnt]="$lon_cursor,$lat_cursor"
		fi
		cnt="$[ $cnt + 1 ]"
		lon_cursor="$( echo "scale = 8; $lon_cursor + $lon_step" | bc )"
	done
	grid[$cnt]="stop"
	cnt="$[ $cnt + 1 ]"
	echo -ne "$y / $Y_STEP ...    \r"
	lat_cursor="$( echo "scale = 8; $lat_cursor + $lat_step" | bc )"
done
echo


createKMLoutput HEAD "perimeter.kml" "perimeter"
cnt="0"
while [ ! -z "${poly[$cnt]}"  ] ; do
	createKMLoutput PNT "perimeter.kml" "${poly[$cnt]}" 
	cnt="$[ $cnt + 1 ]"
done

createKMLoutput END   "perimeter.kml"


X_STEP="$[ $X_STEP + 1 ]"
cnt="0"
tri="0"
grid_tot="${#grid[@]}"
echo "Create grid for external scenary..."
while [ ! -z "${grid[$cnt]}"  ] ; do

	# get index for triangles points
	a="$cnt"
	b="$[ $cnt + 1 			]"
	c="$[ $cnt + $X_STEP + 1	]"

	[  -z "${grid[$c]}" ] && break

	[ "${grid[$a]}" = "stop" ] && cnt="$[ $cnt + 1 ]" && continue
	[ "${grid[$b]}" = "stop" ] && cnt="$[ $cnt + 1 ]" && continue
	[ "${grid[$c]}" = "stop" ] && cnt="$[ $cnt + 1 ]" && continue

	d="$[ $cnt + $X_STEP + 1	]"
	e="$[ $cnt + 1 			]"
	f="$[ $cnt + $X_STEP + 1 + 1	]"

	#[ "${grid[$a]%,*,*}" = "none" ] && grid[$a]="$( mostNearPoint "${grid[$a]}" "${poly[*]}" )"
	#[ "${grid[$b]%,*,*}" = "none" ] && grid[$b]="$( mostNearPoint "${grid[$b]}" "${poly[*]}" )"
	#[ "${grid[$c]%,*,*}" = "none" ] && grid[$c]="$( mostNearPoint "${grid[$c]}" "${poly[*]}" )"

	unset gridc
	[ "${grid[$c]%,*,*}" = "none" ] && gridc="$( mostNearPoint "${grid[$c]}" "${poly[*]}" )"

	#[ "${grid[$d]%,*,*}" = "none" ] && grid[$d]="$( mostNearPoint "${grid[$d]}" "${poly[*]}" )"
	#[ "${grid[$e]%,*,*}" = "none" ] && grid[$e]="$( mostNearPoint "${grid[$e]}" "${poly[*]}" )"
	#[ "${grid[$f]%,*,*}" = "none" ] && grid[$f]="$( mostNearPoint "${grid[$f]}" "${poly[*]}" )"
	

	#if [ "${grid[$a]%,*,*}" != "none" ] && [ "${grid[$b]%,*,*}" != "none" ] && [ "${grid[$c]%,*,*}" != "none" ] ; then 
	if [ "${grid[$a]%,*,*}" != "none" ] && [ "${grid[$b]%,*,*}" != "none" ] ; then 
		# store triangle
		if [ -z "$gridc" ] ; then
			triangle[$tri]="${grid[$a]} ${grid[$b]} ${grid[$c]}"
		else
			triangle[$tri]="${grid[$a]} ${grid[$b]} $gridc"
		fi
		tri="$[ $tri + 1 ]"
	fi

	if [ "${grid[$d]%,*,*}" != "none" ] && [ "${grid[$e]%,*,*}" != "none" ] && [ "${grid[$f]%,*,*}" != "none" ] ; then 
		triangle[$tri]="${grid[$d]} ${grid[$e]} ${grid[$f]}"
		tri="$[ $tri + 1 ]"
	fi

	cnt="$[ $cnt + 1 ]"
	echo -ne "$cnt / $grid_tot ...    \r"

done
echo
echo "Copy .png files and create .ter files... "
# Add water layer
addLine "TERRAIN_DEF terrain_Water"

for pol in ${POLYGON_DEF[@]} ; do
	ter="$( echo "$pol" | sed -e s/".pol"/".ter"/g )"
	png="$( echo "$pol" | sed -e s/".pol"/".png"/g )"
	if [ ! -f "$PNG_DIR/$png" ] ; then
		echo "ERROR: Scenary incomplete! Not found image $png..."
	fi
	if [ ! -f "$TER_DIR/$png" ] ; then
		echo "Copy image $png..."
		cp "$PNG_DIR/$png" "$TER_DIR"
	else
		echo "Image $png exists..."
	fi
	if [ ! -f "$TER_DIR/$ter" ] ; then
		echo "Crate $ter file..."
		echo "$TER_DIR/$ter"
		echo "A"				 > "$TER_DIR/$ter"
		echo "800"				>> "$TER_DIR/$ter"
		echo "TERRAIN"				>> "$TER_DIR/$ter"
		echo					>> "$TER_DIR/$ter"
		echo "BASE_TEX_NOWRAP $png"		>> "$TER_DIR/$ter"
	else
		echo "Terrain file $ter exists..."
	fi

	addLine "TERRAIN_DEF $( basename -- "$TER_DIR" )/$ter"
done
addLine



echo "Create water mesh..."

createKMLoutput HEAD "external.kml" "Externl Grid"
cnt="0"
while [ ! -z "${triangle[$cnt]}"  ] ; do
	createKMLoutput ADD "external.kml" $( echo "${triangle[$cnt]}" | tr "," " " ) 
	cnt="$[ $cnt + 1 ]"
done

createKMLoutput END   "external.kml"

exit


cnt="0"
tot="${#POLYGON_DEF[@]}"
num_line="$( echo "$content" | wc -l )"
altitude="$( echo "${altitude[@]}" | tr " " "\n" )"

MESH_LEVEL="2"

start_line=( $( echo "$content" | grep -n "POLYGON_DEF" | awk -F: {'print $1'} | tr "\n" " " ) )
end_line=(   $( echo "$content" | grep -n "END_POLYGON" | awk -F: {'print $1'} | tr "\n" " " ) )


FINAL_TRIANGLES=()
createKMLoutput HEAD "$KML_FILE" "Triangles List"

for pol in ${POLYGON_DEF[@]} ; do
	addLine "BEGIN_PATCH 0   0.0 -1.0    1   5"
	addLine "BEGIN_PRIMITIVE 0"
	
	overlay="$( echo "$content" | tail -n $[ $num_line - ${start_line[$cnt]} ] | head -n $[ ${end_line[$cnt]} - ${start_line[$cnt]}] )"
	POINTS=( $( echo "$overlay" | grep "POLYGON_POINT" | cut -f 2- -d " " | awk {'print $1","$2"_"$3".00000000,"$4".00000000"'} | tr "\n" " " ) )

	if [ "${#POINTS[@]}" != "4" ] ; then
		echo "ERROR: POLYGON_POINT corrupted..."
		exit 5

	fi

	mesh_level="0"
	mesh=""
	new_vertex=""
	vertex="$( echo "${POINTS[@]}"  | tr " " ";" )"
	echo -n "$[ $cnt + 1 ] / $tot Create mash "
	while [ "$mesh_level" != "$MESH_LEVEL" ] ; do
		for square in $vertex ; do
			POINTS=( $( echo "$square" | tr ";" " " ) )
			new_vertex="$new_vertex $( divideSquare ${POINTS[0]} ${POINTS[1]} ${POINTS[2]} ${POINTS[3]} )"
			echo -n "."
		done
		vertex="$new_vertex"
		mesh_level="$[ $mesh_level + 1 ]"
	done
	mesh="$vertex"
	FINAL_TRIANGLES[$cnt]="$( echo "$mesh" | tr " " "#" )"
	echo -n " Create triangles "
	for square in $mesh ; do

		vertex=( $( echo "$square" | tr ";" " " ) )

	        vertex[0]="$( echo "${vertex[0]}" | tr "_" " " | tr "," "\t" )"
	        vertex[1]="$( echo "${vertex[1]}" | tr "_" " " | tr "," "\t" )"
	        vertex[2]="$( echo "${vertex[2]}" | tr "_" " " | tr "," "\t" )"
        	vertex[3]="$( echo "${vertex[3]}" | tr "_" " " | tr "," "\t" )"

		# fetch coordinates
	        COORD[0]="$( echo "${vertex[0]}" | awk {'print $1" "$2'} )"
	        COORD[1]="$( echo "${vertex[1]}" | awk {'print $1" "$2'} )"
	        COORD[2]="$( echo "${vertex[2]}" | awk {'print $1" "$2'} )"
	        COORD[3]="$( echo "${vertex[3]}" | awk {'print $1" "$2'} )"	

		# Search altitude
		ALT[0]="$( getAltitude ${COORD[0]} )"
			[ "${ALT[0]}" = "99999.00000000" ] && ALT[0]="$( checkAltitude ${COORD[0]} ${ALT[0]} )"
		ALT[1]="$( getAltitude ${COORD[1]} )"
			[ "${ALT[1]}" = "99999.00000000" ] && ALT[1]="$( checkAltitude ${COORD[1]} ${ALT[1]} )"
		ALT[2]="$( getAltitude ${COORD[2]} )"
			[ "${ALT[2]}" = "99999.00000000" ] && ALT[2]="$( checkAltitude ${COORD[2]} ${ALT[2]} )"
		ALT[3]="$( getAltitude ${COORD[3]} )"
			[ "${ALT[3]}" = "99999.00000000" ] && ALT[3]="$( checkAltitude ${COORD[3]} ${ALT[3]} )"

		for a in ${ALT[@]} ; do
	        	[ "$( echo "scale = 8; $a < $min_altitude" | bc )" = "1" ] && min_altitude="$a"
		done


		createKMLoutput ADD "$KML_FILE" ${COORD[3]} ${COORD[1]} ${COORD[0]}
 
		addLine "PATCH_VERTEX ${COORD[3]} ${ALT[3]} 0 0"
		addLine "PATCH_VERTEX ${COORD[1]} ${ALT[1]} 0 0"
		addLine "PATCH_VERTEX ${COORD[0]} ${ALT[0]} 0 0"

		 createKMLoutput ADD "$KML_FILE" ${COORD[3]} ${COORD[2]} ${COORD[1]}

		addLine "PATCH_VERTEX ${COORD[3]} ${ALT[3]} 0 0"
		addLine "PATCH_VERTEX ${COORD[2]} ${ALT[2]} 0 0"
		addLine "PATCH_VERTEX ${COORD[1]} ${ALT[1]} 0 0"

		echo -n "."
	done
	echo
	cnt=$[ $cnt + 1 ]
	addLine "END_PRIMITIVE"
	addLine "END_PATCH"
	addLine
done
createKMLoutput END  "$KML_FILE"



# Output section
echo "Write output file $OUTPUT_DIR/$OUTPUT_FILE..."
cnt="0"
( while [ ! -z "${output[$cnt]}" ] ; do

	echo "${output[$cnt]}"
	cnt=$[ $cnt + 1 ]
done ) > "$OUTPUT_DIR/$OUTPUT_FILE.txt"

output_index="0"
output=()

tot="${#FINAL_TRIANGLES[@]}"
echo "Create .dsf file..."
cnt="0"
for triangle in ${FINAL_TRIANGLES[@]}; do
	addLine "BEGIN_PATCH $[ $cnt + 1 ]   0.0 -1.0     1 7"
	addLine "BEGIN_PRIMITIVE 0"
	mesh="$( echo "$triangle" | tr "#" " " )"
	echo -n "$[ $cnt + 1 ] / $tot Create triangles "
	for square in $mesh ; do

		vertex=( $( echo "$square" | tr ";" " " ) )

	        vertex[0]="$( echo "${vertex[0]}" | tr "_" " " | tr "," "\t" )"
	        vertex[1]="$( echo "${vertex[1]}" | tr "_" " " | tr "," "\t" )"
	        vertex[2]="$( echo "${vertex[2]}" | tr "_" " " | tr "," "\t" )"
        	vertex[3]="$( echo "${vertex[3]}" | tr "_" " " | tr "," "\t" )"

		# fetch coordinates
	        COORD[0]="$( echo "${vertex[0]}" | awk {'print $1" "$2'} )"
	        COORD[1]="$( echo "${vertex[1]}" | awk {'print $1" "$2'} )"
	        COORD[2]="$( echo "${vertex[2]}" | awk {'print $1" "$2'} )"
	        COORD[3]="$( echo "${vertex[3]}" | awk {'print $1" "$2'} )"	

		# Search altitude
		ALT[0]="$( getAltitude ${COORD[0]} )"
			[ "${ALT[0]}" = "99999.00000000" ] && ALT[0]="$( checkAltitude ${COORD[0]} ${ALT[0]} )"
		ALT[1]="$( getAltitude ${COORD[1]} )"
			[ "${ALT[1]}" = "99999.00000000" ] && ALT[1]="$( checkAltitude ${COORD[1]} ${ALT[1]} )"
		ALT[2]="$( getAltitude ${COORD[2]} )"
			[ "${ALT[2]}" = "99999.00000000" ] && ALT[2]="$( checkAltitude ${COORD[2]} ${ALT[2]} )"
		ALT[3]="$( getAltitude ${COORD[3]} )"
			[ "${ALT[3]}" = "99999.00000000" ] && ALT[3]="$( checkAltitude ${COORD[3]} ${ALT[3]} )"

	        # fetch position
	        POS[0]=$( echo "${vertex[0]}" | awk {'print $3" "$4'} )
	        POS[1]=$( echo "${vertex[1]}" | awk {'print $3" "$4'} )
	        POS[2]=$( echo "${vertex[2]}" | awk {'print $3" "$4'} )
	        POS[3]=$( echo "${vertex[3]}" | awk {'print $3" "$4'} )


		addLine "PATCH_VERTEX ${COORD[3]} ${ALT[3]} 0 0 ${POS[3]}"
		addLine "PATCH_VERTEX ${COORD[1]} ${ALT[1]} 0 0 ${POS[1]}"
		addLine "PATCH_VERTEX ${COORD[0]} ${ALT[0]} 0 0 ${POS[0]}"
		addLine "PATCH_VERTEX ${COORD[3]} ${ALT[3]} 0 0 ${POS[3]}"
		addLine "PATCH_VERTEX ${COORD[2]} ${ALT[2]} 0 0 ${POS[2]}"
		addLine "PATCH_VERTEX ${COORD[1]} ${ALT[1]} 0 0 ${POS[1]}"

		echo -n "."
	done
	echo
	cnt=$[ $cnt + 1 ]
	addLine "END_PRIMITIVE"
	addLine "END_PATCH"
	addLine
done
echo


# Output section
echo "Write output file $OUTPUT_DIR/$OUTPUT_FILE..."
cnt="0"
( while [ ! -z "${output[$cnt]}" ] ; do

	echo "${output[$cnt]}"
	cnt=$[ $cnt + 1 ]
done ) >> "$OUTPUT_DIR/$OUTPUT_FILE.txt"



if [ "$( uname -s )" = "Linux" ] ; then
        wine "$dsftool" --text2dsf "$OUTPUT_DIR/$OUTPUT_FILE.txt" "$OUTPUT_DIR/$OUTPUT_FILE" 
else
        "$dsftool" --text2dsf  "$OUTPUT_DIR/$OUTPUT_FILE.txt" "$OUTPUT_DIR/$OUTPUT_FILE"
fi

exit 0

#addLine "TERRAIN_DEF terrain_Water"
#echo "Create water mesh..."
#cnt="0"
#addLine
#addLine "BEGIN_PATCH 0 0.000000 -1.000000 1 5"
#addLine "BEGIN_PRIMITIVE 0"
#
#while [ ! -z "${triangle[$cnt]}"  ] ; do
#	for tri in ${triangle[$cnt]} ; do
#		coord="$( echo "$tri" | tr "," " " | awk {'print $1" "$2" "$3'})"
#		addLine "PATCH_VERTEX $coord 0 0"
#		echo -n "."
#	done	
#	cnt="$[ $cnt + 1 ]"
#done
#addLine "END_PRIMITIVE"
#addLine "END_PATCH"
#addLine
#echo

#
#echo "Create water mesh..."
#water_mask=( $( makeWaterMesh  $max_latitude $min_longitude $max_latitude $max_longitude $min_latitude $max_longitude $min_latitude $min_longitude ) )
#
#addLine
#addLine "BEGIN_PATCH 0   0.0 -1.0    1   5"
#addLine "BEGIN_PRIMITIVE 0"
#for coord in ${water_mask[@]} ; do
#        addLine "PATCH_VERTEX $( echo "$coord" | tr "," " " ) 0.00000000 0 0"
#done
#addLine "END_PRIMITIVE"
#addLine "END_PATCH"
#addLine
#water_mask=( $( makeWaterMesh  ${TILE[0]} ${TILE[3]} ${TILE[0]} ${TILE[2]} ${TILE[1]} ${TILE[2]} ${TILE[1]} ${TILE[3]} ) )
#addLine
#addLine "BEGIN_PATCH 1	0.0 -1.0     1 7"
##addLine "BEGIN_PATCH 1   0.0 -1.0    1   5"
#addLine "BEGIN_PRIMITIVE 0"
#for coord in ${water_mask[@]} ; do
#        addLine "PATCH_VERTEX $( echo "$coord" | tr "," " " ) $min_altitude 0 0 0 0"
#done
#addLine "END_PRIMITIVE"
#addLine "END_PATCH"
#addLine



#
#png="$( dirname "$0" )/ext_app/images/trans.png"
#ter="trans.ter"
#if [ ! -f "$TER_DIR/trans.png" ] ; then
#	echo "Copy image $png..."
#	cp "$png" "$TER_DIR"
#else
#	echo "Image $png exists..."
#fi
#if [ ! -f "$TER_DIR/$ter" ] ; then
#	echo "Crate $ter file..."
#	echo "$TER_DIR/$ter"
#	echo "A"				 > "$TER_DIR/$ter"
#	echo "800"				>> "$TER_DIR/$ter"
#	echo "TERRAIN"				>> "$TER_DIR/$ter"
#	echo					>> "$TER_DIR/$ter"
#	echo "BASE_TEX_NOWRAP trans.png"	>> "$TER_DIR/$ter"
#else
#	echo "Terrain file $ter exists..."
#fi
#addLine "TERRAIN_DEF $( basename -- "$TER_DIR" )/$ter"
#addLine



