LIBS=`pkg-config --libs grilo-0.2 gtk+-3.0 grilo-net-0.2`
CFLAGS= `pkg-config --cflags grilo-0.2 gtk+-3.0 grilo-net-0.2`
CFLAGS+= -Wall -g
TARGET=bin

all:
	gcc totem-shows.c $(LIBS) -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET)
