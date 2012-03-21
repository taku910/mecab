CXX = c++
CXXFLAGS = -O3 -Wall
LDFLAGS = -lcrfpp -lmecab
OBJ = tagger.o decomposer.o

all: pointwise_main

pointwise_main: ${OBJ} pointwise_main.o
	${CXX} ${CFLAGS} -o pointwise_main ${OBJ} pointwise_main.o ${LDFLAGS}

clean:
	rm -f *.o
