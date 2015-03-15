all: TSlink

clean: clean_TSlink

##############################
#######  TARGET: TSlink
##############################
TOP_TSlink=./
WD_TSlink=$(shell cd ${TOP_TSlink};echo `pwd`)
cpp_SRC_TSlink+=${WD_TSlink}/TSlink.cpp

OBJS_TSlink+=$(cpp_SRC_TSlink:.cpp=.cpp.o)

CFLAGS_TSlink+= -iquote./\
 -iquoteSDK/CHeaders/XPLM\
 -iquoteSDK/CHeaders/Widgets\
 -iquote- -iquote/usr/include/\
 -DIBM=0 -DAPL=0 -DLIN=1\
 -DXPLM200

DBG=-g

CFLAGS_TSlink+=-O0 -x c++ -ansi

clean_TSlink:
	rm -fv *.o *.d TSlink.xpl

TSlink:
	$(MAKE) TSlink.xpl TARGET=TSlink.xpl\
 CC="g++"  LD="g++"  AR="ar -crs"  SIZE="size" LIBS+="-lcurl -lm -lGL -lGLU"

TSlink.xpl: ${OBJS_TSlink}
	${CC} -shared ${LDFLAGS} -o TSlink.xpl ${OBJS_TSlink} ${LIBS}


ifeq (${TARGET}, TSlink.xpl)

%.cpp.o: %.cpp
	gcc -Wall -c -fPIC ${CFLAGS_TSlink} $< -o $@ -MMD
include $(cpp_SRC_TSlink:.cpp=.d)

%.d: %.cpp
	set -e; $(CC) -M $(CFLAGS_TSlink) $< \
 | sed 's!\($(*F)\)\.o[ :]*!$(*D)/\1.o $@ : !g' > $@; \
 [ -s $@ ] || rm -f $@

endif
# end Makefile
