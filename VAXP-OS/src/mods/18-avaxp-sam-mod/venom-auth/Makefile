CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE $(shell pkg-config --cflags polkit-agent-1 glib-2.0 x11 cairo cairo-xlib)
LDFLAGS = $(shell pkg-config --libs polkit-agent-1 glib-2.0 x11 cairo cairo-xlib) -lXext -lcrypt -lpam

TARGET = venom_auth
OBJS = venom_auth.o x11_ui.o pam_auth.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete!"

venom_auth.o: venom_auth.c
	$(CC) $(CFLAGS) -c venom_auth.c -o venom_auth.o

x11_ui.o: x11_ui.c
	$(CC) $(CFLAGS) -c x11_ui.c -o x11_ui.o

pam_auth.o: pam_auth.c
	$(CC) $(CFLAGS) -c pam_auth.c -o pam_auth.o

clean:
	@echo "Cleaning..."
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete!"

.PHONY: all clean
