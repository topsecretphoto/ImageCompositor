CC      = g++
C       = cpp

CFLAGS  = -g

ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS   = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

PROJECT	= final

OBJECTS = final.o matrix.o 

matrix.o: matrix.cpp matrix.h

all: ${PROJECT} 
${PROJECT}:	${OBJECTS}
	${CC} ${CFLAGS} ${LFLAGS} -o ${PROJECT} ${OBJECTS} ${LDFLAGS}
	${CC} ${CFLAGS} -c matrix.cpp

%.o: %.cpp
	${CC} -c ${CFLAGS} $<

clean:
	rm -f core.* *.o *~ ${PROJECT}
