#!/bin/bash

coords=( $( cat tile-trtqtstqqrtsrtrttt.crd ) )


padfTransform=( -180 0.012 0 90 0.012 0 )

input=( 12.24014282 44.66962909 12.23876953 44.67060598 12.24151611 44.66865277 )


#    lat = (90 -     ( (float)((atoi(zoneId)-1) / 1440 ) ) * 0.25            );
#    lon = (         ( (float)((atoi(zoneId)-1) % 1440 ) ) * 0.25 - 180      );


Xul="$( echo "scale = 6; ( 		${input[2]} +  180 	) / 0.012" | bc | awk -F. {'print $1'} )"
Yul="$( echo "scale = 6; ( 90 - 	${input[5]} 		) / 0.012" | bc | awk -F. {'print $1'} )"

Xlr="$( echo "scale = 6; ( 		${input[4]} +  180 	) / 0.012" | bc | awk -F. {'print $1'} )"
Ylr="$( echo "scale = 6; ( 90 - 	${input[3]} 		) / 0.012" | bc | awk -F. {'print $1'} )"


Xsize="$[ $Xlr - $Xul ]"
Ysize="$[ $Ylr - $Yul ]"

[ "$Xsize" -le "0" ] && Xsize="1"
[ "$Ysize" -le "0" ] && Ysize="1"

echo "$Xul $Yul $Xlr $Ylr"

echo convert nightearth.gif -crop ${Xsize}x${Ysize}+$Xul+$Yul night.png
