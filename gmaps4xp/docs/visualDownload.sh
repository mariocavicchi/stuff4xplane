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





downloadTile(){
	xcoord="$1"
	ycoord="$2"
	zcoord="$3"
	file="$4"
	server="$[ ( $xcoord % 4 )  + 1 ]"
	# wget -q "http://visualimages${server}.paginegialle.it/xml.php/europa-orto.imgi?cmd=tile&format=png&x=${xcoord}&y=${ycoord}&z=${zcoord}&extra=2&ts=256&q=100&rdr=0&sito=visual"		-O "$file"
	wget -q "http://visualimages${server}.paginegialle.it/xmlvisual.php/europa.imgi?cmd=tile&format=png&x=${xcoord}&y=${ycoord}&z=${zcoord}&extra=2&ts=256&q=100&rdr=0&sito=visual&v=1" 	-O "$file"
}

# http://visualimages3.paginegialle.it/xml.php/europa-orto.imgi?cmd=tile&format=jpeg&x=1&y=1&z=32768&extra=2&ts=256&q=65&rdr=0&sito=visual
# http://visualimages3.paginegialle.it/xmlvisual.php/europa.imgi?cmd=tile&format=jpeg&x=8422&y=10628&z=8&extra=2&ts=256&q=60&rdr=0&sito=visual&v=1
# http://visualimages1.paginegialle.it/xml.php/europa-orto.imgi?cmd=tile&format=jpeg&x=8432&y=10648&z=8&extra=2&ts=256&q=65&rdr=0&sito=visual
 
getXY(){
	lat="$1"
	lon="$2"
	zoom="$3"
	zone="$( echo "scale = 6; ( $lon   + 180   ) / 6 + 1" | bc )"
	zone="${zone%.*}"

 

cat << EOM | bc -l 
mapwidthlevel1pixel 	= 33554432
mapwidthmeters		= 4709238.7
mapcentreutmeasting	= 637855.35
mapcentreutmnorthing	= 5671353.65
define tan(x) { x = s(x) / c(x); return (x); }
define latlongxy(e,a,t){
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

	x = s
	y = r
	n = mapwidthmeters
	f = mapcentreutmeasting
	g = mapcentreutmnorthing
	j = t
	e = x+((n/2)-f)
	b = g + (n/2) - y
	l = e / n
	h = b / n
	l * ${TILE_LEVEL[$zoom]}
	h * ${TILE_LEVEL[$zoom]}
	s
	r

}

a = latlongxy($lon, $lat, $zone)

EOM

}


geoRef(){
	x="$1"
	y="$2"
	E="$3"
	N="$4"

	zoom="$5"

	pixelRes="$( echo "scale = 6; 4709238.7 / ${TILE_LEVEL[$zoom]}" | bc )"
	

	x="$( echo "$x" | awk -F. {'print "0."$2'} )"
	y="$( echo "$y" | awk -F. {'print "0."$2'} )"

	ULx="$( echo "scale = 6; $E - ( $pixelRes * $x )" | bc )"
	ULy="$( echo "scale = 6; $N + ( $pixelRes * $y )" | bc )"
	pixelRes="$( echo "scale = 6; 4709238.7 / ${TILE_LEVEL[$zoom]} / 256" | bc )"

	echo -n "$ULx, $pixelRes, 0.0, $ULy, 0.0, $pixelRes"

}


UL=( 44.854227 11.597803 )
LR=( 44.824209 11.636779 )

#UL=( 44.875188 11.575790 )
#LR=( 44.849834 11.607900 )
#UL=( 44.906861 11.609939 )
#LR=( 44.671381 11.808416 )
#UL=( 44.906861 11.609939 )
#LR=( 44.671381 11.808416 )
#UL=(  44.875582  11.576781 )
#LR=(  44.844107  11.603004 )





LEVEL="8"

ULxy=( $( getXY ${UL[*]} $LEVEL ) )
LRxy=( $( getXY ${LR[*]} $LEVEL ) )

echo "${ULxy[*]}"
echo "${LRxy[*]}"

xsize="$[ ${LRxy[0]%.*} - ${ULxy[0]%.*} ]"
ysize="$[ ${LRxy[1]%.*} - ${ULxy[1]%.*} ]"
xoffset="${ULxy[0]%.*}"
yoffset="${ULxy[1]%.*}"

 cnt="0"
 for y in $( seq 0  $ysize ) ; do
 	yT="$[ $yoffset + $y ]"
 	for x in $( seq 0  $xsize ) ; do
 		xT="$[ $xoffset + $x ]"
 
 		echo "${xT} ${yT}"
 		downloadTile "${xT}" "${yT}" "$LEVEL" "tmp-${xT}-${yT}.png"
 		convert -page +$[ 256 * $x  ]+$[ 256 * $y ] "tmp-${xT}-${yT}.png" -format PNG32 "tile-${xT}-${yT}.png"
 		imageList[$cnt]="tile-${xT}-${yT}.png"
 		rm -f "tmp-${xT}-${yT}.png"
 		cnt=$[ $cnt + 1 ]
 	done
 done
 
 convert  -layers mosaic ${imageList[*]} map.png
 
 

GeoTransform="$( geoRef ${ULxy[*]} $LEVEL )"

zone="$( echo "scale = 6; ( ${UL[1]}   + 180   ) / 6 + 1" | bc )"
zone="${zone%.*}"

cat << EOF > map.png.aux.xml
<PAMDataset>
  <SRS>PROJCS[&quot;UTM Zone $zone, Northern Hemisphere&quot;,GEOGCS[&quot;WGS 84&quot;,DATUM[&quot;WGS_1984&quot;,SPHEROID[&quot;WGS 84&quot;,6378137,298.257223563,AUTHORITY[&quot;EPSG&quot;,&quot;7030&quot;]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[&quot;EPSG&quot;,&quot;6326&quot;]],PRIMEM[&quot;Greenwich&quot;,0,AUTHORITY[&quot;EPSG&quot;,&quot;8901&quot;]],UNIT[&quot;degree&quot;,0.0174532925199433,AUTHORITY[&quot;EPSG&quot;,&quot;9108&quot;]],AUTHORITY[&quot;EPSG&quot;,&quot;4326&quot;]],PROJECTION[&quot;Transverse_Mercator&quot;],PARAMETER[&quot;latitude_of_origin&quot;,0],PARAMETER[&quot;central_meridian&quot;,9],PARAMETER[&quot;scale_factor&quot;,0.9996],PARAMETER[&quot;false_easting&quot;,500000],PARAMETER[&quot;false_northing&quot;,0],UNIT[&quot;Meter&quot;,1]]</SRS>
  <GeoTransform>  $GeoTransform </GeoTransform>
  <Metadata domain="IMAGE_STRUCTURE">
    <MDI key="INTERLEAVE">PIXEL</MDI>
  </Metadata>
</PAMDataset>

EOF

gdal_translate -of GTiff map.png map.tif
rm -f map.png.aux.xml
