all: webserver  

clean: clean_webserver

##############################
#######  TARGET: webserver
##############################
TOP_webserver=./
WD_webserver=$(shell cd ${TOP_webserver};echo `pwd`)
cpp_SRC_webserver+=${WD_webserver}/webserver.cpp
cpp_SRC_webserver+=${WD_webserver}/core.cpp

OBJS_webserver+=$(cpp_SRC_webserver:.cpp=.cpp.o)

CFLAGS_webserver+= -iquote./\
 -iquoteSDK/CHeaders/XPLM\
 -iquoteSDK/CHeaders/Widgets\
 -iquote- -iquote/usr/include/\
 -DIBM=0 -DAPL=0 -DLIN=1\
 -DXPLM200

DBG=-g

CFLAGS_webserver+=-O0 -x c++ -ansi

clean_webserver:
	rm -fv *.o *.d webserver.xpl

webserver:
	$(MAKE) webserver.xpl TARGET=webserver.xpl CC="g++"  LD="g++"  AR="ar -crs"  SIZE="size" LIBS+="-lpthread"
	#$(MAKE) webserver.exe TARGET=webserver.exe CC="g++"  LD="g++"  LIBS+="-lpthread"

webserver.xpl: ${OBJS_webserver}
	${CC} -shared 	${LDFLAGS}	-o webserver.xpl 	${OBJS_webserver} ${LIBS}

webserver.exe: ${OBJS_webserver}
	${CC} ${LDFLAGS}		-o webserver.exe 	${WD_webserver}/core.cpp.o ${LIBS}


ifeq (${TARGET}, webserver.xpl)

%.cpp.o: %.cpp
	gcc -Wall -c -fPIC ${CFLAGS_webserver} $< -o $@ -MMD
include $(cpp_SRC_webserver:.cpp=.d)

%.d: %.cpp
	set -e; $(CC) -M $(CFLAGS_webserver) $<  | sed 's!\($(*F)\)\.o[ :]*!$(*D)/\1.o $@ : !g' > $@;  [ -s $@ ] || rm -f $@

endif
# end Makefile
