all: HelloWorld

clean: clean_HelloWorld

##############################
#######  TARGET: HelloWorld
##############################
TOP_HelloWorld=./
WD_HelloWorld=$(shell cd ${TOP_HelloWorld};echo `pwd`)
cpp_SRC_HelloWorld+=${WD_HelloWorld}/HelloWorld.cpp

OBJS_HelloWorld+=$(cpp_SRC_HelloWorld:.cpp=.cpp.o)

CFLAGS_HelloWorld+= -iquote./\
 -iquoteSDK/CHeaders/XPLM\
 -iquoteSDK/CHeaders/Widgets\
 -iquote- -iquote/usr/include/\
  -DIBM=0 -DAPL=0 -DLIN=1

DBG=-g

CFLAGS_HelloWorld+=-O0 -x c++ -ansi

clean_HelloWorld:
	rm -f ${OBJS_HelloWorld}
	rm -f HelloWorld.cpp.d
	rm -f HelloWorld.d

HelloWorld:
	$(MAKE) -f HelloWorld.mk HelloWorld.xpl TARGET=HelloWorld.xpl\
 CC="g++"  LD="g++"  AR="ar -crs"  SIZE="size" LIBS+="-lGL -lGLU"

HelloWorld.xpl: ${OBJS_HelloWorld}
	${CC} -shared ${LDFLAGS} -o HelloWorld.xpl ${OBJS_HelloWorld} ${LIBS}


ifeq (${TARGET}, HelloWorld.xpl)

%.cpp.o: %.cpp
	gcc -c -fPIC ${CFLAGS_HelloWorld} $< -o $@ -MMD
include $(cpp_SRC_HelloWorld:.cpp=.d)

%.d: %.cpp
	set -e; $(CC) -M $(CFLAGS_HelloWorld) $< \
 | sed 's!\($(*F)\)\.o[ :]*!$(*D)/\1.o $@ : !g' > $@; \
 [ -s $@ ] || rm -f $@

endif
# end Makefile
