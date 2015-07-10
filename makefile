CC=gcc
LIBS=`pkg-config --libs grilo-0.2 gtk+-3.0 grilo-net-0.2`
CFLAGS= `pkg-config --cflags grilo-0.2 gtk+-3.0 grilo-net-0.2`
CFLAGS+= -Wall -g -DLOCAL_PATH=\""$(PWD)/"\"
TARGET=bin
CCRESOURCES=glib-compile-resources

all:
	$(CCRESOURCES) totem-video-summary.gresource.xml --target=tvsresources.h --c-name _totem_video_summary --generate-header
	$(CCRESOURCES) totem-video-summary.gresource.xml --target=tvsresources.c --c-name _totem_video_summary --generate-source
	$(CC) $(CFLAGS) -c tvsresources.c $(LIBS)
	$(CC) $(CFLAGS) -c totem-video-summary.c $(LIBS)
	$(CC) $(CFLAGS) sample.c totem-video-summary.o tvsresources.o -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) totem-video-summary.o tvsresources.*
