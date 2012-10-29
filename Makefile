CFLAGS := -g -Wall
CXXFLAGS := $(CFLAGS)
LDFLAGS := -lGL -lGLEW -lSDL -lpng -ljpeg
proj := imager

objs :=  \
	src/gre.o \
	src/gui.o \
	src/imageloader.o \
	src/imagemanager.o \
	src/memorymapper_posix.o \
	src/thread.o \
	src/jpeg.o \
	src/main.o \
	src/mime.o \
	src/png.o \
	src/tga.o

imager: $(objs)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(proj) $(objs)
