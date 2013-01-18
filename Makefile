CFLAGS := -g -Wall -I.
CXXFLAGS := $(CFLAGS)
LDFLAGS := -lGL -lpng -ljpeg -lX11 -pthread
proj := imager

objs :=  \
	zdl/zdl_xlib.o \
	src/gre_zdl.o \
	src/gui.o \
	src/font.o \
	src/imageloader.o \
	src/imagemanager.o \
	src/memorymapper_posix.o \
	src/thread.o \
	src/simpletcp.o \
	src/proxy.o \
	src/sbuffer.o \
	src/server.o \
	src/httpstrm.o \
	src/httpproxy.o \
	src/ringbuffer.o \
	src/jpeg.o \
	src/main.o \
	src/mime.o \
	src/png.o \
	src/tga.o

imager: $(objs)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(proj) $(objs)
