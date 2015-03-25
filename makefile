LIBS=`pkg-config --libs grilo-0.2 gtk+-3.0 grilo-net-0.2`
CFLAGS= `pkg-config --cflags grilo-0.2 gtk+-3.0 grilo-net-0.2`
CFLAGS+= -Wall -g
TARGET=bin
CCRESOURCES=glib-compile-resources

all:
	$(CCRESOURCES) totem-video-summary.gresource.xml --target=tvsresources.h --c-name _totem_video_summary --generate-header
	$(CCRESOURCES) totem-video-summary.gresource.xml --target=tvsresources.c --c-name _totem_video_summary --generate-source
	$(CC) $(CFLAGS) -c tvsresources.c $(LIBS)
	$(CC) $(CFLAGS) -c totem-video-summary.c tvsresources.o $(LIBS)
	$(CC) $(CFLAGS) sample.c -o $(TARGET) totem-video-summary.o tvsresources.o $(LIBS)

clean:
	rm -f $(TARGET) totem-video-summary.o tvsresources.*
