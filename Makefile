CC=clang++ -std=c++11
SRC=src/*.cpp


# this is specific to my weird setup; change it to wherever your libraries are

IDIR=/opt/homebrew/Cellar/rtaudio/6.0.1/include/rtaudio
LDIR=/opt/homebrew/lib

DEPS=src/*.h $(IDIR)/*.h
LIBS=-lrtaudio -lmonome

CFLAGS=-I$(IDIR) -L$(LDIR)

all: Molisam

debug: CFLAGS+=-ggdb3 -Wall
debug: Molisam

Molisam:$(OBJ) $(DEPS)
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LIBS)

run:
	./Molisam

clean:
	rm Molisam
