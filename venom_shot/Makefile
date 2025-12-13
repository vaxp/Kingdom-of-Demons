CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags glib-2.0 gio-2.0 gio-unix-2.0 cairo x11 xi xtst)
LIBS = $(shell pkg-config --libs glib-2.0 gio-2.0 gio-unix-2.0 cairo x11 xi xtst)

TARGET = venom_shot
SRCS = main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
