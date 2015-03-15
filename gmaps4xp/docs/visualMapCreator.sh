#!/bin/bash

TILE_LEVEL[2]="65536"
TILE_LEVEL[4]="32768"	
TILE_LEVEL[8]="16384"
TILE_LEVEL[16]="8192"	
TILE_LEVEL[32]="4096"	
TILE_LEVEL[64]="2048"	
TILE_LEVEL[128]="1024"
TILE_LEVEL[256]="512"
TILE_LEVEL[512]="256"
TILE_LEVEL[1024]="128"
TILE_LEVEL[2048]="64"
TILE_LEVEL[4096]="32"
TILE_LEVEL[8192]="16"
TILE_LEVEL[16384]="8"
TILE_LEVEL[32768]="4"
TILE_LEVEL[65536]="2"



log(){
        echo "$(date) - $1" 1>&2
}

##############################################################################################3


if [ -z "$3" ] ; then
	echo "Usage $(basename $0) UpperLeftLat UpperLeftLon OutputDirectory"
	exit 1
fi

UpperLeftLat="$1"
UpperLeftLon="$2"


UL=( $UpperLeftLat $UpperLeftLon )
LR=( $[ $UpperLeftLat - 1 ] $[ $UpperLeftLon + 1 ] )


LEVEL="8"
OUTPUT_DIR="$3"
tolerance="1"
log "Directory Tree creation ..."

[ ! -d "$OUTPUT_DIR" ] 		&& mkdir "$OUTPUT_DIR"
[ ! -d "$OUTPUT_DIR/images" ] 	&& mkdir "$OUTPUT_DIR/images"
[ ! -d "$OUTPUT_DIR/dds" ] 	&& mkdir "$OUTPUT_DIR/dds"
[ ! -d "$OUTPUT_DIR/ter" ] 	&& mkdir "$OUTPUT_DIR/ter"
[ ! -d "$OUTPUT_DIR/tmp" ] 	&& mkdir "$OUTPUT_DIR/tmp"

WD="$( cd $( dirname $0 ); pwd )"
pixelResolution=()

##############################################################################################3

downloadTile(){
	local xcoord="$1"
	local ycoord="$2"
	local zcoord="$3"
	local file="$4"
	local server="$[ ( $xcoord % 4 )  + 1 ]"
	pid="1"
	[ -f "$OUTPUT_DIR/tmp/raw-${xT}-${yT}.png" ] && return
	while [ "$pid" -ne "0" ] ;  do
		wget --timeout=10 -q "http://visualimages${server}.paginegialle.it/xml.php/europa-orto.imgi?cmd=tile&format=png&x=${xcoord}&y=${ycoord}&z=${zcoord}&extra=2&ts=256&q=100&rdr=0&sito=visual" -O "$file"
		pid="$?"
		sec="$[ 1 + $RANDOM % 10 ]"
		[ "$pid" -ne "0" ] && log "Unable to download retry in $sec seconds ..." && sleep $sec
	done
}

# http://visualimages3.paginegialle.it/xml.php/europa-orto.imgi?cmd=tile&format=jpeg&x=1&y=1&z=32768&extra=2&ts=256&q=65&rdr=0&sito=visual
# http://visualimages3.paginegialle.it/xmlvisual.php/europa.imgi?cmd=tile&format=jpeg&x=8422&y=10628&z=8&extra=2&ts=256&q=60&rdr=0&sito=visual&v=1
# http://visualimages1.paginegialle.it/xml.php/europa-orto.imgi?cmd=tile&format=jpeg&x=8432&y=10648&z=8&extra=2&ts=256&q=65&rdr=0&sito=visual
# ftp://xftp.jrc.it/pub/srtmV4/tiff/srtm_39_04.zip


##############################################################################################3

getLongZone(){
	longitude="$1"
cat << EOM   | bc
	scale 		= 6;
	longitude 	= $longitude;
	longzone	= 0;

	if ( longitude < 0.0 ){
		longzone = ((180.0 + longitude) / 6) + 1; 
	}else{
		longzone = (longitude / 6) + 31; 
	}
	

	scale = 0;
	longzone / 1;
EOM

}

##############################################################################################3

getXY(){
	local lat="$1"
	local lon="$2"
	local zoom="$3"

cat << EOM | bc -l 
mapwidthlevel1pixel 	= 33554432;
mapwidthmeters		= 4709238.7;

define i(xt)  		{ auto s ; s = scale; scale = 0; xt /= 1; scale = s; return (xt); }
define tan(x) 		{ x = s(x) / c(x); return (x); }
define pow(at,bt)       { xt = e(l(at) * bt); return (xt); }
define ceil(xt) 	{ auto savescale; savescale = scale; scale = 0; if (xt>0) { if (xt%1>0) result = xt+(1-(xt%1)) else result = xt } else result = -1*floor(-1*xt);  scale = savescale; return result }
define floor(xt) 	{ auto savescale; savescale = scale; scale = 0; if (xt>0) result = xt-(xt%1) else result = -1*ceil(-1*xt);  scale = savescale; return result }


scale = 20;
define latlongxy(e,a){ 
	eori = e;	
	b = floor( ( e + 180 ) / 360);
	h = ( e + 180 ) - b * 360 - 180;
	t = 32;

        if (( h > 14.1285  ) && ( h < 24.143875 ) && ( a > 49.003201 ) && ( a < 54.8387 ) ) t = 33;
	if ( h < 5.763938 ) t = 31;

	mapcentreutmeasting	= 637855.35
	mapcentreutmnorthing	= 5671353.65

	
	if( t == 33 ){
		mapcentreutmnorthing 	= 5677219.33619 - 117761.5;
		mapcentreutmeasting	= 718496.723786 + 133922.35;
	}else{
		if( t == 35 ){
			mapcentreutmnorthing	= 4533619.12;
			mapcentreutmeasting	= 637855.35;
		}else{
			if( t == 31 ){	
				mapcentreutmnorthing	= 5699775.82 + 103232.5;
				mapcentreutmeasting	= 1056886.43 - 617710;
			}else{
				mapcentreutmnorthing	= 5671353.65;
				mapcentreutmeasting	= 637855.35;
			}
		}
	}
	
	p = 6378137
	n = 0.00669438
	l = 0.9996
	u = 3.14159265
	b = u * e / 180
	f = u * a / 180
	q = ( (t-1) * 6 - 180 + 3 ) * u / 180
	h = (n) / (1-n)
	g = p /  sqrt(1-n * s(f) *s(f) )
	d = tan(f) * tan(f)
	m = h * c(f) * c(f)
	o = c(f) * (b-q)
	j = p*((1-n/4-3*n*n/64-5*n*n*n/256)*f-(3*n/8+3*n*n/32+45*n*n*n/1024) * s(2*f)+(15*n*n/256+45*n*n*n/1024) * s(4*f)-(35*n*n*n/3072)*s(6*f))
	s = (l*g*(o+(1-d+m)*o*o*o/6+(5-18*d+d*d+72*m-58*h)*o*o*o*o*o/120)+500000)
	r = (l*(j+g*tan(f)*(o*o/2+(5-d+9*m+4*m*m)*o*o*o*o/24+(61-58*d+d*d+600*m-330*h)*o*o*o*o*o*o/720)));
	
	x = s;
	y = r;

	n = mapwidthmeters;
	f = mapcentreutmeasting;
	g = mapcentreutmnorthing;
	j = t;
	e = x+((n/2)-f);
	b = g + (n/2) - y;
	l = e / n;
	h = b / n;
	l * ${TILE_LEVEL[$zoom]};
	h * ${TILE_LEVEL[$zoom]};

	/*-------------------------*/	
	e = eori;

	if ( e < 0.0 ){
		t = ((180.0 + e) / 6) + 1; 
	}else{
		t = (e / 6) + 31; 
	}

	t = i(t);	

	p = 6378137
	n = 0.00669438
	l = 0.9996
	u = 3.14159265
	b = u * e / 180
	f = u * a / 180
	q = ( (t-1) * 6 - 180 + 3 ) * u / 180
	h = (n) / (1-n)
	g = p /  sqrt(1-n * s(f) *s(f) )
	d = tan(f) * tan(f)
	m = h * c(f) * c(f)
	o = c(f) * (b-q)
	j = p*((1-n/4-3*n*n/64-5*n*n*n/256)*f-(3*n/8+3*n*n/32+45*n*n*n/1024) * s(2*f)+(15*n*n/256+45*n*n*n/1024) * s(4*f)-(35*n*n*n/3072)*s(6*f))
	s = (l*g*(o+(1-d+m)*o*o*o/6+(5-18*d+d*d+72*m-58*h)*o*o*o*o*o/120)+500000)
	r = (l*(j+g*tan(f)*(o*o/2+(5-d+9*m+4*m*m)*o*o*o*o/24+(61-58*d+d*d+600*m-330*h)*o*o*o*o*o*o/720)));
	
	s;
	r;
	
	
}

a = latlongxy($lon, $lat);

EOM

}

##############################################################################################3

getDirName(){
        local lat="$1"
        local lon="$2"
        [ -z "$lat" ] && log "getDirName Latitude is empty"     && exit 1
        [ -z "$lon" ] && log "getDirName Longitude is empty"    && exit 1



        [  "$( echo "$lat < 0" | bc -l )" = 1 ] && lat="$( echo "scale = 8; $lat - 10.0" | bc -l )"

        local int="${lat%.*}"
        [ -z "$int" ] && int="0"
        lat="$( echo  "$int - ( $int % 10 )" | bc )"
        [ -z "$( echo "${lat%.*}" | tr -d "-" )" ] && lat="$( echo "$lat" | sed -e s/"\."/"0\."/g )"
        [ "$( echo "$lat > 0" | bc -l )" = 1  ] && lat="+$lat"

        [ "$( echo -n "$lat" | tr -d "+-" | wc -c | awk {'print $1'} )" = "1" ] && lat="$( echo "$lat" | sed -e s/"+"/"+0"/g |  sed -e s/"-"/"-0"/g )"


        [  "$( echo "$lon < 0" | bc -l )" = 1 ] && lon="$( echo "scale = 8; $lon - 10.0" | bc -l )"

        local int="${lon%.*}"
        [ -z "$int" ] && int="0"
        lon="$( echo  "$int - ( $int % 10 )" | bc )"
        [ -z "$( echo "${lon%.*}" | tr -d "-" )" ] && lon="$( echo "$lon" | sed -e s/"\."/"0\."/g )"
        [ "$( echo "$lon >= 0" | bc -l )" = 1  ] && lon="+$lon"



        [ "$( echo -n "$lon" | tr -d "+-" | wc -c | awk {'print $1'} )" = "1" ] && lon="$( echo "$lon" | sed -e s/"+"/"+00"/g |  sed -e s/"-"/"-00"/g )"
        [ "$( echo -n "$lon" | tr -d "+-" | wc -c | awk {'print $1'} )" = "2" ] && lon="$( echo "$lon" | sed -e s/"+"/"+0"/g |  sed -e s/"-"/"-0"/g )"

        [ "$lat" = "0" ]        && lat="+00"
        [ -z "$lon" ]           && lon="+000"
        echo "$lat$lon"
}

##############################################################################################3

getDSFName(){
        local lat="$1"
        local lon="$2"
        [ -z "$lat" ] && log "getDSFName Latitude is empty"     && exit 1
        [ -z "$lon" ] && log "getDSFName Longitude is empty"    && exit 1

        lat="$( echo "$lat" | awk -F. {'print $1'} )"
        [ -z "$lat" ] && lat="0"
        [ -z "$( echo "${lat%.*}" | tr -d "-" )" ] && lat="$( echo "$lat" | sed -e s/"\."/"0\."/g )"

        [ "$( echo "$lat < 0" | bc )" = 1  ] && lat="$( echo "$lat - 1" | bc )"
        [ "$( echo "$lat > 0" | bc )" = 1  ] && lat="+$lat"

        [ "$( echo -n "$lat" | tr -d "+-" | wc -c | awk {'print $1'} )" = "1" ] && lat="$( echo "$lat" | sed -e s/"+"/"+0"/g |  sed -e s/"-"/"-0"/g )"


        lon="$( echo "$lon" | awk -F. {'print $1'} )"
        [ -z "$lon" ] && lon="0"

        [ -z "$( echo "${lon%.*}" | tr -d "-" )" ] && lon="$( echo "$lon" | sed -e s/"\."/"0\."/g )"

        [ "$( echo "$lon < 0" | bc )" = 1  ] && lon="$( echo "$lon - 1" | bc )"
        [ "$( echo "$lon > 0" | bc )" = 1  ] && lon="+$lon"


        [ "$( echo -n "$lon" | tr -d "+-" | wc -c | awk {'print $1'} )" = "1" ] && lon="$( echo "$lon" | sed -e s/"+"/"+00"/g |  sed -e s/"-"/"-00"/g )"
        [ "$( echo -n "$lon" | tr -d "+-" | wc -c | awk {'print $1'} )" = "2" ] && lon="$( echo "$lon" | sed -e s/"+"/"+0"/g  |  sed -e s/"-"/"-0"/g  )"


        [ "$lat" = "0" ]        && lat="+00"
        [ "$lon" = "0" ]        && lon="+000"
        [ "$lon" = "-000" ]     && lon="-001"

        echo "$lat$lon.dsf"
}

##############################################################################################3

geoRef(){
	local x="$1"
	local y="$2"
	local E="$3"
	local N="$4"

	local zoom="$5"

	local pixelRes="$( echo "scale = 6; 4709238.7 / ${TILE_LEVEL[$zoom]}" | bc )"
	

	local x="$( echo "$x" | awk -F. {'print "0."$2'} )"
	local y="$( echo "$y" | awk -F. {'print "0."$2'} )"

	local ULx="$( echo "scale = 6; $E - ( $pixelRes * $x )" | bc )"
	local ULy="$( echo "scale = 6; $N + ( $pixelRes * $y )" | bc )"
	local pixelRes="$( echo "scale = 6; 4709238.7 / ${TILE_LEVEL[$zoom]} / 256" | bc )"

	echo -n "$ULx, $pixelRes, 0.0, $ULy, 0.0, -$pixelRes"

}

##############################################################################################3

imageGeoInfo(){
	local padfGeoTransform=( $( echo "$*" | tr "," " " ) )

cat << EOM | bc -l | tr -d  "\\\\\n" 
	scale 	= 6;

	dfpixel = 128;
	dfline	= 128;
	pdfgeox	= ${padfGeoTransform[0]} + dfpixel * ${padfGeoTransform[1]} + dfline * ${padfGeoTransform[2]};
	pdfgeoy	= ${padfGeoTransform[3]} + dfpixel * ${padfGeoTransform[4]} + dfline * ${padfGeoTransform[5]};
	print   pdfgeox, "," , pdfgeoy, " ";

	dfpixel = 0;
	dfline	= 0;
	pdfgeox	= ${padfGeoTransform[0]} + dfpixel * ${padfGeoTransform[1]} + dfline * ${padfGeoTransform[2]};
	pdfgeoy	= ${padfGeoTransform[3]} + dfpixel * ${padfGeoTransform[4]} + dfline * ${padfGeoTransform[5]};
	print   pdfgeox, "," , pdfgeoy, " ";

	dfpixel = 256;
	dfline	= 0;
	pdfgeox	= ${padfGeoTransform[0]} + dfpixel * ${padfGeoTransform[1]} + dfline * ${padfGeoTransform[2]};
	pdfgeoy	= ${padfGeoTransform[3]} + dfpixel * ${padfGeoTransform[4]} + dfline * ${padfGeoTransform[5]};
	print   pdfgeox, "," , pdfgeoy, " ";

	dfpixel = 256;
	dfline	= 256;
	pdfgeox	= ${padfGeoTransform[0]} + dfpixel * ${padfGeoTransform[1]} + dfline * ${padfGeoTransform[2]};
	pdfgeoy	= ${padfGeoTransform[3]} + dfpixel * ${padfGeoTransform[4]} + dfline * ${padfGeoTransform[5]};
	print   pdfgeox, "," , pdfgeoy, " ";

	dfpixel = 0;
	dfline	= 256;
	pdfgeox	= ${padfGeoTransform[0]} + dfpixel * ${padfGeoTransform[1]} + dfline * ${padfGeoTransform[2]};
	pdfgeoy	= ${padfGeoTransform[3]} + dfpixel * ${padfGeoTransform[4]} + dfline * ${padfGeoTransform[5]};
	print   pdfgeox, "," , pdfgeoy;

EOM


}

##############################################################################################

imageGeoInfoToLatLng(){
	local args=( $* )
	local zone="${args[0]}"

	scale="$[ $( echo "${tolerance#*.}" | wc -c ) - $( echo "${tolerance#*.}" | sed -e s/^0*//g  | wc -c ) + 2 ]"
	scale="${scale/-/}"
	[ -z "$scale" ] && scale="20"

	local cnt="1"
	while [ ! -z "${args[$cnt]}" ] ; do
		x="${args[$cnt]%,*}"
		y="${args[$cnt]#*,}"
cat << EOM | bc -l 
define i(xt) 		{  auto s ; s = scale; scale = 0; xt /= 1; scale = s; return (xt); }
scale = 20
define tan(xt) 		{ xt = s(xt) / c(xt); return (xt); }
define abs(xt) 		{ if ( xt < 0 ) xt = xt * -1.0; return (xt); }
define pow(at,bt)	{ xt = e(l(at) * bt); return (xt); }
define main(x,y, utmz){
	drad	= 4*a(1) / 180
	a	= 6378137.0;
	f	= 1 / 298.2572236;
	b 	= a*(1-f);
	k0	= 0.9996;			
	b	= a * ( 1 - f );		
	e 	= sqrt( 1 - ( b/a ) * (b/a) );	
	e0 	= e / sqrt(1 - e*e);		
	esq	= (1 - (b/a)*(b/a));	
	e0sq 	= e*e/(1-e*e);	
	zcm	= 3 + 6*(utmz-1) - 180;		
	e1 	= (1 - sqrt(1 - e*e))/(1 + sqrt(1 - e*e)); 
	m0 	= 0;
	m	= m0 + y/k0;
	mu	= m / (a*(1 - esq * ( 1/4 + esq * ( 3/64 + 5*esq/256))));
	phi1	= mu + e1*(3/2 - 27*e1*e1/32)* s(2*mu) + e1*e1*(21/16 -55*e1*e1/32) * s(4*mu);
	phi1	= phi1 + e1*e1*e1*(s(6*mu)*151/96 + e1*s(8*mu)*1097/512);
	c1 	= e0sq * c(phi1) * c(phi1);
	t1 	= tan(phi1) * tan(phi1);
	n1 	= a / sqrt(1 - ((e*s(phi1)) * e*s(phi1)) );
	r1 	= n1*(1-e*e)/ (1- (e*s(phi1) * e*s(phi1)) );
	d 	= (x-500000)/(n1*k0);
	phi	= (d*d)*(1/2 - d*d*(5 + 3*t1 + 10*c1 - 4*c1*c1 - 9*e0sq)/24);
	phi	= phi + d*d*d*d*d*d*(61 + 90*t1 + 298*c1 + 45*t1*t1 -252*e0sq - 3*c1*c1)/720;
	phi	= phi1 - (n1*tan(phi1)/r1)*phi;
	lat 	= ( 1000000 * phi / drad)/1000000;
	lng	= d*(1 + d*d*((-1 -2*t1 -c1)/6 + d*d*(5 - 2*c1 + 28*t1 - 3*c1*c1 +8*e0sq + 24*t1*t1)/120))/c(phi1);
	lngd	= zcm+lng/drad;
	lon	= (1000000*lngd)/1000000;


	/*
	latd = lat - i(lat); 
	lond = lon - i(lon); 

	if ( latd > 0.5 ) { 
		latd = 1.0 - latd;
		nlat = lat + 1.0; 
	} else 	nlat = lat;

	if ( lond > 0.5 ) {
		lond = 1.0 - lond;
		nlon = lon + 1.0;
	} else  nlon = lon;
	
	if ( abs( latd ) < $tolerance )  lat = i(nlat);
	if ( abs( lond ) < $tolerance )  lon = i(nlon);
	*/

	scale = $scale;
	
	if ( ( lat > ( $UpperLeftLat - 1 ) ) && ( lat < $UpperLeftLat ) && ( lon > $UpperLeftLon ) && ( lon < ( $UpperLeftLon + 1 ) ) ) {
		lon /= 1;
		lat /= 1;
		print lon, "," , lat, " ";
	} else	print " , " ;
	
	
}

r = main($x, $y, $zone);

EOM
		((cnt++))
	done
}

##############################################################################################3

downloadTexture(){
	local xoffset="$1"
	local yoffset="$2"
	local LEVEL="$3"
	local file="$4"
	local cnt="0"
	local x=""
	local y=""

	[ -f "$file" ] && return
	log "Downloading texture for $xoffset $yoffset ..."
	for y in {0..7} ; do
		yT="$[ $yoffset + $y ]"

		pidList=()
		echo -n "$(date) - Get Line $yT starting: " 1>&2
		for x in {0..7} ; do
        		xT="$[ $xoffset + $x ]"
			downloadTile "${xT}" "${yT}" "$LEVEL" "$OUTPUT_DIR/tmp/raw-${xT}-${yT}.png" & 
			pidList[$x]="$!"
			echo -n "$x " 1>&2
		done
		echo  " ... Waiting ..." 1>&2
		for x in {0..7} ; do wait ${pidList[$x]} ; done
		log "Done"

		for x in {0..7} ; do
        		xT="$[ $xoffset + $x ]"
	        	log "$cnt / 64"
		 	convert -page +$[ 256 * $x  ]+$[ 256 * $y ] "$OUTPUT_DIR/tmp/raw-${xT}-${yT}.png" -channel RGB -format PNG32 "$OUTPUT_DIR/tmp/tile-${xT}-${yT}.png"
		 	imageList[$cnt]="$OUTPUT_DIR/tmp/tile-${xT}-${yT}.png"
		 	cnt=$[ $cnt + 1 ]
		done
	done
	#convert -channel ALL -normalize -layers mosaic ${imageList[*]} "$file"
	convert -layers mosaic ${imageList[*]} "$file"
	rm -f "$OUTPUT_DIR/tmp/tile-"*
	echo -n "$file"
	

}

##############################################################################################3

pointsTextureLatLng(){
	local ZUTM="$1"
	local LEVEL="$2"
	local padfGeoTransform=( ${3} ${4} ${5} ${6} ${7} ${8} )
	local x=""
	local y=""


	for y in {0..7} ; do
		for x in {0..7} ; do
			local east="$(  echo "scale = 6; ${padfGeoTransform[0]} + (256 * $x) * ${padfGeoTransform[1]} + (256 * $y) * ${padfGeoTransform[2]}" | bc )"
			local north="$( echo "scale = 6; ${padfGeoTransform[3]} + (256 * $x) * ${padfGeoTransform[4]} + (256 * $y) * ${padfGeoTransform[5]}" | bc )"

			padfGeoTransformNew=( $east ${GeoTransform[1]} ${GeoTransform[2]} $north ${GeoTransform[4]} ${GeoTransform[5]} )

			UTMimageInfo=(  $( imageGeoInfo 	${padfGeoTransformNew[*]} )     )
			imageInfo=( 	$( imageGeoInfoToLatLng "$ZUTM" "${UTMimageInfo[*]}" ) 	)
			imageInfoTest=( ${imageInfo[*]/,/} )
			[ "${#imageInfoTest[*]}" -ne "5" ] && return 1
			echo "${imageInfo[*]}" 
		done
	done
	return 0
}

##############################################################################################3

imageGeoInfoToLatLngWithCrop(){
	local args=( $* )
	local zone="${args[0]}"

	scale="$[ $( echo "${tolerance#*.}" | wc -c ) - $( echo "${tolerance#*.}" | sed -e s/^0*//g  | wc -c ) + 2 ]"
	scale="${scale/-/}"
	[ -z "$scale" ] && scale="20"

	local cnt="1"
	while [ ! -z "${args[$cnt]}" ] ; do
		x="${args[$cnt]%,*}"
		y="${args[$cnt]#*,}"
cat << EOM | bc -l 
define i(xt) 		{  auto s ; s = scale; scale = 0; xt /= 1; scale = s; return (xt); }
scale = 20
define tan(xt) 		{ xt = s(xt) / c(xt); return (xt); }
define abs(xt) 		{ if ( xt < 0 ) xt = xt * -1.0; return (xt); }
define pow(at,bt)	{ xt = e(l(at) * bt); return (xt); }
define main(x,y, utmz){
	drad	= 4*a(1) / 180
	a	= 6378137.0;
	f	= 1 / 298.2572236;
	b 	= a*(1-f);
	k0	= 0.9996;			
	b	= a * ( 1 - f );		
	e 	= sqrt( 1 - ( b/a ) * (b/a) );	
	e0 	= e / sqrt(1 - e*e);		
	esq	= (1 - (b/a)*(b/a));	
	e0sq 	= e*e/(1-e*e);	
	zcm	= 3 + 6*(utmz-1) - 180;		
	e1 	= (1 - sqrt(1 - e*e))/(1 + sqrt(1 - e*e)); 
	m0 	= 0;
	m	= m0 + y/k0;
	mu	= m / (a*(1 - esq * ( 1/4 + esq * ( 3/64 + 5*esq/256))));
	phi1	= mu + e1*(3/2 - 27*e1*e1/32)* s(2*mu) + e1*e1*(21/16 -55*e1*e1/32) * s(4*mu);
	phi1	= phi1 + e1*e1*e1*(s(6*mu)*151/96 + e1*s(8*mu)*1097/512);
	c1 	= e0sq * c(phi1) * c(phi1);
	t1 	= tan(phi1) * tan(phi1);
	n1 	= a / sqrt(1 - ((e*s(phi1)) * e*s(phi1)) );
	r1 	= n1*(1-e*e)/ (1- (e*s(phi1) * e*s(phi1)) );
	d 	= (x-500000)/(n1*k0);
	phi	= (d*d)*(1/2 - d*d*(5 + 3*t1 + 10*c1 - 4*c1*c1 - 9*e0sq)/24);
	phi	= phi + d*d*d*d*d*d*(61 + 90*t1 + 298*c1 + 45*t1*t1 -252*e0sq - 3*c1*c1)/720;
	phi	= phi1 - (n1*tan(phi1)/r1)*phi;
	lat 	= ( 1000000 * phi / drad)/1000000;
	lng	= d*(1 + d*d*((-1 -2*t1 -c1)/6 + d*d*(5 - 2*c1 + 28*t1 - 3*c1*c1 +8*e0sq + 24*t1*t1)/120))/c(phi1);
	lngd	= zcm+lng/drad;
	lon	= (1000000*lngd)/1000000;


	e = lon;
	a = lat;
	if ( lat < ( $UpperLeftLat - 1 ) )	{ a = $UpperLeftLat - 1; }
	if ( lat > $UpperLeftLat ) 		{ a = $UpperLeftLat;	 }	
	if ( lon < $UpperLeftLon ) 		{ e = $UpperLeftLon;	 }
	if ( lon > ( $UpperLeftLon + 1 ) )	{ e = $UpperLeftLon + 1; }


	/*
	if ( e < 0.0 ){
		t = ((180.0 + e) / 6) + 1; 
	}else{
		t = (e / 6) + 31; 
	}

	t = i(t);	
	*/
	t = $zone;

	p = 6378137
	n = 0.00669438
	l = 0.9996
	u = 3.14159265
	b = u * e / 180
	f = u * a / 180
	q = ( (t-1) * 6 - 180 + 3 ) * u / 180
	h = (n) / (1-n)
	g = p /  sqrt(1-n * s(f) *s(f) )
	d = tan(f) * tan(f)
	m = h * c(f) * c(f)
	o = c(f) * (b-q)
	j = p*((1-n/4-3*n*n/64-5*n*n*n/256)*f-(3*n/8+3*n*n/32+45*n*n*n/1024) * s(2*f)+(15*n*n/256+45*n*n*n/1024) * s(4*f)-(35*n*n*n/3072)*s(6*f))
	s = (l*g*(o+(1-d+m)*o*o*o/6+(5-18*d+d*d+72*m-58*h)*o*o*o*o*o/120)+500000)
	r = (l*(j+g*tan(f)*(o*o/2+(5-d+9*m+4*m*m)*o*o*o*o/24+(61-58*d+d*d+600*m-330*h)*o*o*o*o*o*o/720)));

	s = ( s  - $x );
	r = ( $y - r  );

	if ( abs(s) 	< abs(${pixelResolution[0]}) ) 	s 	= 0.0;
	if ( abs(r) 	< abs(${pixelResolution[1]}) ) 	r	= 0.0;
	if (( e 	== $UpperLeftLon ) && (( a - $UpperLeftLat ) <= 0.1))	e 	+= 0.00001;

	s = i(s / ${pixelResolution[0]} );
	r = i(r / ${pixelResolution[1]} );

	scale 	= 7; /* $scale; */
	e	/= 1;
	a	/= 1;
	s	/= 1;
	r	/= 1;

 	print e, "," , a, "," , s, "," , r, " ";

	

	
}

r = main($x, $y, $zone);

EOM
		((cnt++))
	done
}

##############################################################################################3

pointsTextureLatLngBorder(){
	local ZUTM="$1"
	local LEVEL="$2"
	local padfGeoTransform=( ${3} ${4} ${5} ${6} ${7} ${8} )
	local x=""
	local y=""

	for y in {0..7} ; do
		for x in {0..7} ; do

			local east="$(  echo "scale = 6; ${padfGeoTransform[0]} + (256 * $x) * ${padfGeoTransform[1]} + (256 * $y) * ${padfGeoTransform[2]}" | bc )"
			local north="$( echo "scale = 6; ${padfGeoTransform[3]} + (256 * $x) * ${padfGeoTransform[4]} + (256 * $y) * ${padfGeoTransform[5]}" | bc )"

			padfGeoTransformNew=( $east ${GeoTransform[1]} ${GeoTransform[2]} $north ${GeoTransform[4]} ${GeoTransform[5]} )

			UTMimageInfo=(  $( imageGeoInfo 	${padfGeoTransformNew[*]} )     )
			imageInfo=( 	$( imageGeoInfoToLatLngWithCrop "$ZUTM" "${UTMimageInfo[*]}" ) )
			echo "${imageInfo[*]}" 
		done
	done
	return 0
}

##############################################################################################3

createTerFile(){
	local file="$1"
	[ -f "${file/.png/.ter}" ] && return
	local name="$( basename "$file")"
	log "Creating Ter file for $name ..."	

# LOAD_CENTER 42.70321 -72.34234 4000 1024
cat > "${file/.png/.ter}" << EOF
A
800
TERRAIN
BASE_TEX_NOWRAP ../images/$name
EOF

}

dsfFileWrite(){
	local args=( $* )

	local n="8"
	local xtoken=( $( seq 0 $( echo "scale = 6; 1 / $n" | bc ) 1 		 ) )
	local ytoken=( $( seq 0 $( echo "scale = 6; 1 / $n" | bc ) 1 | sort -r ) )

	local xsize="${xtoken[1]}"
	local ysize="${xtoken[1]}"
	local patchNum="$1"

	local CC=()
	local LL=()
	local LR=()
	local UR=()
	local UL=()
	local x=""
	local y=""

	local cnt="1"
	local i="0"
	last=$[ ${#args[*]} - 1 ] ; [ "${args[$last]}" -ne "0" ] && return ${args[$last]}
	unset args[$last]

	srtm="srtm_39_04.tif"
	echo "BEGIN_PATCH $patchNum   0.0 -1.0     1 7"
	echo "BEGIN_PRIMITIVE 0"
	while [ ! -z "${args[$cnt]}" ] ; do			
		x="$[ $i % $n ]"
		y="$[ $i / $n ]"
		((i++))

 		CC=( $( awk 'BEGIN { printf "%f %f", '${args[$cnt]}'	}' ) ) 
 		UL=( $( awk 'BEGIN { printf "%f %f", '${args[$cnt+1]}'	}' ) ) 
 		UR=( $( awk 'BEGIN { printf "%f %f", '${args[$cnt+2]}'	}' ) ) 
 		LR=( $( awk 'BEGIN { printf "%f %f", '${args[$cnt+3]}'	}' ) ) 
 		LL=( $( awk 'BEGIN { printf "%f %f", '${args[$cnt+4]}'	}' ) ) 
		cnt="$[ $cnt + 5 ]"	


		alt=( $( $WD/dem/getalt $WD/dem/$srtm ${CC[*]} ${UL[*]} ${UR[*]} ${LR[*]} ${LL[*]} ) )
		[ "${#alt[*]}" -ne "5" ] && continue


		CC[${#CC[*]}]="${alt[0]}"
		UL[${#UL[*]}]="${alt[1]}"
		UR[${#UR[*]}]="${alt[2]}"
		LR[${#LR[*]}]="${alt[3]}"
		LL[${#LL[*]}]="${alt[4]}"

 		local xstart="${xtoken[$x]}"
 		local ystart="${ytoken[$y]}"

                CC[${#CC[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' + '$xsize' / 2 }'	)"
		CC[${#CC[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' - '$ysize' / 2 }'	)" 
                LL[${#LL[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' }' 		)"
		LL[${#LL[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' - '$ysize'  }' 	)" 
                UL[${#UL[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' }' 		)"
		UL[${#UL[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' }' 		)" 
                UR[${#UR[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' + '$xsize' }'	)"
		UR[${#UR[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' }'		)" 
                LR[${#LR[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' + '$xsize' }'	)"
		LR[${#LR[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' - '$ysize'  }'	)" 

echo "
PATCH_VERTEX ${UL[0]} ${UL[1]} ${UL[2]} 0 0 ${UL[3]} ${UL[4]}
PATCH_VERTEX ${UR[0]} ${UR[1]} ${UR[2]} 0 0 ${UR[3]} ${UR[4]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[2]} 0 0 ${CC[3]} ${CC[4]}
PATCH_VERTEX ${UR[0]} ${UR[1]} ${UR[2]} 0 0 ${UR[3]} ${UR[4]}
PATCH_VERTEX ${LR[0]} ${LR[1]} ${LR[2]} 0 0 ${LR[3]} ${LR[4]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[2]} 0 0 ${CC[3]} ${CC[4]}
PATCH_VERTEX ${LR[0]} ${LR[1]} ${LR[2]} 0 0 ${LR[3]} ${LR[4]}
PATCH_VERTEX ${LL[0]} ${LL[1]} ${LL[2]} 0 0 ${LL[3]} ${LL[4]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[2]} 0 0 ${CC[3]} ${CC[4]}
PATCH_VERTEX ${LL[0]} ${LL[1]} ${LL[2]} 0 0 ${LL[3]} ${LL[4]}
PATCH_VERTEX ${UL[0]} ${UL[1]} ${UL[2]} 0 0 ${UL[3]} ${UL[4]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[2]} 0 0 ${CC[3]} ${CC[4]}"

	
	done
	echo "END_PRIMITIVE"
	echo "END_PATCH"
	echo
	return 0
}

##############################################################################################3

dsfFileWriteBorder(){
	local args=( $* )

	local n="8"
	local xtoken=( $( seq 0 $( echo "scale = 6; 1 / $n" | bc ) 1 		 ) )
	local ytoken=( $( seq 0 $( echo "scale = 6; 1 / $n" | bc ) 1 | sort -r ) )

	local xsize="${xtoken[1]}"
	local ysize="${xtoken[1]}"
	local patchNum="$1"

	local CC=()
	local LL=()
	local LR=()
	local UR=()
	local UL=()
	local x=""
	local y=""

	local cnt="1"
	local i="0"
	last=$[ ${#args[*]} - 1 ] ; [ "${args[$last]}" -ne "0" ] && return ${args[$last]}
	unset args[$last]
	writeHeader="false"
	headerWrote="false"

	srtm="srtm_39_04.tif"
	while [ ! -z "${args[$cnt]}" ] ; do			
		x="$[ $i % $n ]"
		y="$[ $i / $n ]"
		((i++))

 		CC=( $( awk 'BEGIN { printf "%f %f %f %f", '${args[$cnt]}'	}' ) ) 
 		UL=( $( awk 'BEGIN { printf "%f %f %f %f", '${args[$cnt+1]}'	}' ) ) 
 		UR=( $( awk 'BEGIN { printf "%f %f %f %f", '${args[$cnt+2]}'	}' ) ) 
 		LR=( $( awk 'BEGIN { printf "%f %f %f %f", '${args[$cnt+3]}'	}' ) ) 
 		LL=( $( awk 'BEGIN { printf "%f %f %f %f", '${args[$cnt+4]}'	}' ) ) 
		cnt="$[ $cnt + 5 ]"	


		alt=( $( $WD/dem/getalt $WD/dem/$srtm ${CC[0]} ${CC[1]} ${UL[0]} ${UL[1]} ${UR[0]} ${UR[1]} ${LR[0]} ${LR[1]} ${LL[0]} ${LL[1]} ) )
		[ "${#alt[*]}" -ne "5" ] && continue

		CC[${#CC[*]}]="${alt[0]}"
		UL[${#UL[*]}]="${alt[1]}"
		UR[${#UR[*]}]="${alt[2]}"
		LR[${#LR[*]}]="${alt[3]}"
		LL[${#LL[*]}]="${alt[4]}"

 		local xstart="${xtoken[$x]}"
 		local ystart="${ytoken[$y]}"

		CC[${#CC[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' + '$xsize' / 2 }'	)"
		CC[${#CC[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' - '$ysize' / 2 }'	)" 
		LL[${#LL[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' }' 		)"
		LL[${#LL[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' - '$ysize' }' 	)" 
		UL[${#UL[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' }' 		)"
		UL[${#UL[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' }' 		)" 
		UR[${#UR[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' + '$xsize' }'	)"
		UR[${#UR[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' }'		)" 
		LR[${#LR[*]}]="$( awk 'BEGIN { printf "%f", '$xstart' + '$xsize' }'	)"
		LR[${#LR[*]}]="$( awk 'BEGIN { printf "%f", '$ystart' - '$ysize' }'	)" 



		if [ "$( 	echo 	"( ${CC[2]} == 0.0 ) && ( ${CC[3]} == 0.0 ) && ( ${LL[2]} == 0.0 ) && ( ${LL[3]} == 0.0 ) && ( ${UL[2]} == 0.0 ) && ( ${UL[3]} == 0.0 ) &&  \
					 ( ${UR[2]} == 0.0 ) && ( ${UR[3]} == 0.0 ) && ( ${LR[2]} == 0.0 ) && ( ${LR[3]} == 0.0 )" | bc )" = "0" ] ; then


			[ "$( echo  -ne "${LL[0]}\n${UL[0]}\n${UR[0]}\n${LR[0]}\n" | sort -n -u | wc -l )" -le "1" ] && continue
			[ "$( echo  -ne "${LL[1]}\n${UL[1]}\n${UR[1]}\n${LR[1]}\n" | sort -n -u | wc -l )" -le "1" ] && continue


			
			writeHeader="true"

			if [ "$writeHeader" = "true" ] && [ "$headerWrote" = "false" ] ; then
				echo "BEGIN_PATCH $patchNum   0.0 -1.0     1 7"
				echo "BEGIN_PRIMITIVE 0"
				headerWrote="true"
			fi

  			LL[5]="$( awk 'BEGIN { printf "%f", '${LL[5]}' + '${LL[2]}' * 1 / 2048 }')"
  			LL[6]="$( awk 'BEGIN { printf "%f", '${LL[6]}' + '${LL[3]}' * 1 / 2048 }')"
  
  			UL[5]="$( awk 'BEGIN { printf "%f", '${UL[5]}' + '${UL[2]}' * 1 / 2048 }')"
  			UL[6]="$( awk 'BEGIN { printf "%f", '${UL[6]}' + '${UL[3]}' * 1 / 2048 }')"
  
  			UR[5]="$( awk 'BEGIN { printf "%f", '${UR[5]}' + '${UR[2]}' * 1 / 2048 }')"
  			UR[6]="$( awk 'BEGIN { printf "%f", '${UR[6]}' + '${UR[3]}' * 1 / 2048 }')"

  			LR[5]="$( awk 'BEGIN { printf "%f", '${LR[5]}' + '${LR[2]}' * 1 / 2048 }')"
  			LR[6]="$( awk 'BEGIN { printf "%f", '${LR[6]}' + '${LR[3]}' * 1 / 2048 }')"


  			LL[5]="$( awk 'BEGIN { printf "%f", ('${LL[5]}' < 0) ? 0 : '${LL[5]}' }')"
  			LR[5]="$( awk 'BEGIN { printf "%f", ('${LR[5]}' < 0) ? 0 : '${LR[5]}' }')"
  			UL[5]="$( awk 'BEGIN { printf "%f", ('${UL[5]}' < 0) ? 0 : '${UL[5]}' }')"
  			UR[5]="$( awk 'BEGIN { printf "%f", ('${UR[5]}' < 0) ? 0 : '${UR[5]}' }')"
                                                                                        
   			LL[5]="$( awk 'BEGIN { printf "%f", ('${LL[5]}' > 1) ? 1 : '${LL[5]}' }')"
   			LR[5]="$( awk 'BEGIN { printf "%f", ('${LR[5]}' > 1) ? 1 : '${LR[5]}' }')"
   			UL[5]="$( awk 'BEGIN { printf "%f", ('${UL[5]}' > 1) ? 1 : '${UL[5]}' }')"
   			UR[5]="$( awk 'BEGIN { printf "%f", ('${UR[5]}' > 1) ? 1 : '${UR[5]}' }')"

  			LL[6]="$( awk 'BEGIN { printf "%f", ('${LL[6]}' < 0) ? 0 : '${LL[6]}' }')"
  			LR[6]="$( awk 'BEGIN { printf "%f", ('${LR[6]}' < 0) ? 0 : '${LR[6]}' }')"
  			UL[6]="$( awk 'BEGIN { printf "%f", ('${UL[6]}' < 0) ? 0 : '${UL[6]}' }')"
  			UR[6]="$( awk 'BEGIN { printf "%f", ('${UR[6]}' < 0) ? 0 : '${UR[6]}' }')"
                                                                                        
   			LL[6]="$( awk 'BEGIN { printf "%f", ('${LL[6]}' > 1) ? 1 : '${LL[6]}' }')"
   			LR[6]="$( awk 'BEGIN { printf "%f", ('${LR[6]}' > 1) ? 1 : '${LR[6]}' }')"
   			UL[6]="$( awk 'BEGIN { printf "%f", ('${UL[6]}' > 1) ? 1 : '${UL[6]}' }')"
   			UR[6]="$( awk 'BEGIN { printf "%f", ('${UR[6]}' > 1) ? 1 : '${UR[6]}' }')"
                                                                           
echo "
PATCH_VERTEX ${LL[0]} ${LL[1]} ${LL[4]} 0 0 ${LL[5]} ${LL[6]}
PATCH_VERTEX ${UL[0]} ${UL[1]} ${UL[4]} 0 0 ${UL[5]} ${UL[6]}
PATCH_VERTEX ${UR[0]} ${UR[1]} ${UR[4]} 0 0 ${UR[5]} ${UR[6]}
PATCH_VERTEX ${LL[0]} ${LL[1]} ${LL[4]} 0 0 ${LL[5]} ${LL[6]}
PATCH_VERTEX ${UR[0]} ${UR[1]} ${UR[4]} 0 0 ${UR[5]} ${UR[6]}
PATCH_VERTEX ${LR[0]} ${LR[1]} ${LR[4]} 0 0 ${LR[5]} ${LR[6]}"

			continue
		fi


		writeHeader="true"

		if [ "$writeHeader" = "true" ] && [ "$headerWrote" = "false" ] ; then
			echo "BEGIN_PATCH $patchNum   0.0 -1.0     1 7"
			echo "BEGIN_PRIMITIVE 0"
			headerWrote="true"
		fi


echo "
PATCH_VERTEX ${UL[0]} ${UL[1]} ${UL[4]} 0 0 ${UL[5]} ${UL[6]}
PATCH_VERTEX ${UR[0]} ${UR[1]} ${UR[4]} 0 0 ${UR[5]} ${UR[6]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[4]} 0 0 ${CC[5]} ${CC[6]}
PATCH_VERTEX ${UR[0]} ${UR[1]} ${UR[4]} 0 0 ${UR[5]} ${UR[6]}
PATCH_VERTEX ${LR[0]} ${LR[1]} ${LR[4]} 0 0 ${LR[5]} ${LR[6]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[4]} 0 0 ${CC[5]} ${CC[6]}
PATCH_VERTEX ${LR[0]} ${LR[1]} ${LR[4]} 0 0 ${LR[5]} ${LR[6]}
PATCH_VERTEX ${LL[0]} ${LL[1]} ${LL[4]} 0 0 ${LL[5]} ${LL[6]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[4]} 0 0 ${CC[5]} ${CC[6]}
PATCH_VERTEX ${LL[0]} ${LL[1]} ${LL[4]} 0 0 ${LL[5]} ${LL[6]}
PATCH_VERTEX ${UL[0]} ${UL[1]} ${UL[4]} 0 0 ${UL[5]} ${UL[6]}
PATCH_VERTEX ${CC[0]} ${CC[1]} ${CC[4]} 0 0 ${CC[5]} ${CC[6]}"

	
	done
	if [ "$writeHeader" = "true" ] && [ "$headerWrote" = "true" ] ; then
		echo "END_PRIMITIVE"
		echo "END_PATCH"
		echo
	else
		return 1

	fi
	return 0
}

##############################################################################################3

dsfFileOpen(){
	local args=( $* )

cat << EOF
A
800
DSF2TEXT

PROPERTY sim/creation_agent $( basename $0 )
PROPERTY sim/west ${args[1]}
PROPERTY sim/east ${args[3]}
PROPERTY sim/north ${args[0]}
PROPERTY sim/south ${args[2]}
PROPERTY sim/planet earth

EOF

#TERRAIN_DEF terrain_Water
#EOF



}

##############################################################################################3

dsfFileClose(){
	local args=( $* )
	file="$OUTPUT_DIR/Earth nav data/$( getDirName ${args[2]} ${args[1]} )/$( getDSFName ${args[2]} ${args[1]} )"
	log "Merging $( getDirName ${args[2]} ${args[1]} )/$( getDSFName ${args[2]} ${args[1]} ) ..."
	# 45 11 44 12

	cat "${file}_header.txt" 
# cat << EOF
# 
# BEGIN_PATCH 0   0.0 -1.0    1   5
# BEGIN_PRIMITIVE 0
# PATCH_VERTEX ${args[1]} ${args[2]} 0   0 0
# PATCH_VERTEX ${args[3]} ${args[0]} 0   0 0
# PATCH_VERTEX ${args[3]} ${args[2]} 0   0 0
# PATCH_VERTEX ${args[1]} ${args[2]} 0   0 0
# PATCH_VERTEX ${args[1]} ${args[0]} 0   0 0
# PATCH_VERTEX ${args[3]} ${args[0]} 0   0 0
# END_PRIMITIVE
# END_PATCH
# 
# EOF

	cat "${file}_body.txt"

}

##############################################################################################3

distEarth(){
	decLonA="$1"
	decLatA="$2"
	
	decLonB="$3"
	decLatB="$4"

	pi="$( echo "scale=10; 4*a(1)" | bc -l )"

	radLatA="$(     echo "scale = 16; $pi * $decLatA / 180" | bc -l )"
	radLonA="$(     echo "scale = 16; $pi * $decLonA / 180" | bc -l )"
	radLatB="$(     echo "scale = 16; $pi * $decLatB / 180" | bc -l )"
	radLonB="$(     echo "scale = 16; $pi * $decLonB / 180" | bc -l )"
	phi="$(         echo "scale = 16;  $radLonA - $radLonB"                                                 | bc -l | tr -d "-" )"
	P="$(           echo "scale = 16; (s($radLatA) * s($radLatB)) +  (c($radLatA) * c($radLatB) * c($phi))" | bc -l )"
	P="$(           echo "scale = 16; a(-1 * $P / sqrt(-1 * $P * $P + 1)) + 2 * a(1)"                       | bc -l )"

	echo "scale=16; $P * 6372.795477598 * 1000" | bc -l | awk '{ printf "%.4f", $1 }'

}

##############################################################################################3

# 4267.50432795132138078208 5285.90057636915915661312 263553.97392506660609542713 4987329.49919556277015735568
# 4274.60224298288348454912 5479.13148088584732516352 259473.67898415547566969349 4876249.12156055389327270092


getMQ(){

cat << EOM | bc 
	scale = 6;
	x1 = $1;
	y1 = $2;
	x2 = $3;
	y2 = $4;


	m = ( y2 - y1 ) - (x2 - x1 );
	q = y1 - m * x1;
	print m, " ", q;

EOM


}



##############################################################################################3

ZUTM="$( getLongZone "${UL[1]}" )"

ULxy=( $( getXY ${UL[*]} 	  $LEVEL ) )
LRxy=( $( getXY ${LR[*]} 	  $LEVEL ) )
LLxy=( $( getXY ${LR[0]} ${UL[1]} $LEVEL ) )
URxy=( $( getXY ${UL[0]} ${LR[1]} $LEVEL ) )


zoneXsize="$( distEarth ${UL[1]} ${UL[0]} ${LR[1]} ${UL[0]} )"
zoneYsize="$( distEarth ${UL[1]} ${UL[0]} ${UL[1]} ${LR[0]} )"


xstart="${ULxy[0]%.*}"
ystart="${ULxy[1]%.*}"

dsfPath="$OUTPUT_DIR/Earth nav data/$( getDirName ${LR[0]} ${UL[1]} )" ; [ ! -d "$dsfPath" ] && mkdir -p "$dsfPath"
dsfName="$( getDSFName ${LR[0]} ${UL[1]} )"


dsfFileOpen ${UL[*]} ${LR[*]} 	> "$dsfPath/${dsfName}_header.txt"
echo -n				> "$dsfPath/${dsfName}_body.txt" 

geoStart=( 	 	$( getXY 			${UL[*]}		$LEVEL ) )
geoStartUTM=( 	 	$( geoRef			${geoStart[*]}		$LEVEL ) )
geoStartLatLong=( 	$( imageGeoInfoToLatLng 	$ZUTM			"${geoStartUTM[0]/,/},${geoStartUTM[3]/,/}" | tr "," " " ) )

GeoTransform=( 	$( geoRef 	${geoStart[*]} 		$LEVEL ) 	)
GeoTransform=( ${GeoTransform[*]/,/} )
pixelResolution=( ${GeoTransform[1]} ${GeoTransform[5]} )
log "Upper left coordinates: ${GeoTransform[0]}E ${GeoTransform[3]}N, Zone: $ZUTM, Resolution: ${GeoTransform[1]} / ${GeoTransform[5]} ..."
log "Zone dimentions $zoneXsize x $zoneYsize meters ..."
tolerance="$( echo "scale = 20; ( ${GeoTransform[1]/-/} + ${GeoTransform[5]/-/} ) / 2 / 1000" | bc )"

p="0"
X="0"
Y="0"
ySize="0"
while : ; do 
	xSize="0"
	X="0"
	yoffset="$[ $ystart + $Y ]"


	log "Forwaring in longitude ..."
	while : ; do
		xoffset="$[ $xstart + $X ]"
                north="$( echo "scale = 6; ${GeoTransform[3]} + (256 * $X) * ${GeoTransform[4]} + (256 * $Y) * ${GeoTransform[5]}" | bc )"
                east="$(  echo "scale = 6; ${GeoTransform[0]} + (256 * $X) * ${GeoTransform[1]} + (256 * $Y) * ${GeoTransform[2]}" | bc )"
		[ -z "$( imageGeoInfoToLatLng $ZUTM "$east,$north" | tr -d ", " )" ] && break
		X="$[ $X - 8 ]"
	done
	
	while : ; do 

		# 2048x2048
		log "$X, $Y ..."
		xoffset="$[ $xstart + $X ]"

		north="$( echo "scale = 6; ${GeoTransform[3]} + (256 * $X) * ${GeoTransform[4]} + (256 * $Y) * ${GeoTransform[5]}" | bc )"
		east="$(  echo "scale = 6; ${GeoTransform[0]} + (256 * $X) * ${GeoTransform[1]} + (256 * $Y) * ${GeoTransform[2]}" | bc )"

		point=( $xoffset $yoffset )
	
		GeoTransformNew=( $east ${GeoTransform[1]} ${GeoTransform[2]} $north ${GeoTransform[4]} ${GeoTransform[5]} )

		log "Generating texture vertex coordintaes for $xoffset, $yoffset ..."

		dsfFileWrite "$p" $( pointsTextureLatLng $ZUTM $LEVEL ${GeoTransformNew[*]}; echo -n " $?" )	>> "$dsfPath/${dsfName}_body.txt"
		if [ "$?" -eq "0" ] ; then
			echo "TERRAIN_DEF ter/texture-$xoffset-$yoffset.ter"		>> "$dsfPath/${dsfName}_header.txt"
			[ ! -f "$OUTPUT_DIR/images/texture-$xoffset-$yoffset.png" ] 	&& downloadTexture	$xoffset $yoffset $LEVEL 	"$OUTPUT_DIR/images/texture-$xoffset-$yoffset.png" 	> /dev/null
			[ ! -f "$OUTPUT_DIR/ter/texture-$xoffset-$yoffset.ter" ] 	&& createTerFile 					"$OUTPUT_DIR/ter/texture-$xoffset-$yoffset.png"		> /dev/null

			((p++))

		else
			log "Texture out of tile ... Try to crop ..."

			dsfFileWriteBorder $p $( pointsTextureLatLngBorder $ZUTM $LEVEL ${GeoTransformNew[*]};  echo -n " $?" ) >> "$dsfPath/${dsfName}_body.txt"
			if [ "$?" -eq "0" ] ; then
				echo "TERRAIN_DEF ter/texture-$xoffset-$yoffset.ter"		>> "$dsfPath/${dsfName}_header.txt"
				[ ! -f "$OUTPUT_DIR/images/texture-$xoffset-$yoffset.png" ] 	&& downloadTexture	$xoffset $yoffset $LEVEL 	"$OUTPUT_DIR/images/texture-$xoffset-$yoffset.png" 	> /dev/null
				[ ! -f "$OUTPUT_DIR/ter/texture-$xoffset-$yoffset.ter" ] 	&& createTerFile 					"$OUTPUT_DIR/ter/texture-$xoffset-$yoffset.png"		> /dev/null
	
				((p++))
			else
				log "Failed, completly out..."
			fi
		fi


		xSize="$( echo "scale = 6; $xSize + 2048 * ${GeoTransform[1]}" | bc )"
		[ "$( echo "$xSize > $zoneXsize" | bc )" = "1" ]  && break

		X="$[ $X + 8 ]"

	done
	ySize="$( echo "scale = 6; $ySize + 2048 * ${GeoTransform[1]}" | bc )"
	[ "$( echo "$ySize > $zoneYsize" | bc )" = "1" ]  && break
	Y="$[ $Y + 8 ]"

done


dsfFileClose ${UL[*]} ${LR[*]} > "$dsfPath/${dsfName}.txt"


exit 0


 



