# ═══════════════════════════════════════════════════════════════════════════
# Venom OSD Daemon - Makefile (GTK3)
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0 libpulse libpulse-mainloop-glib x11 xrandr)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0 libpulse libpulse-mainloop-glib x11 xrandr) -lm

TARGET = venom_osd
SRC = venom_osd.c

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "🧹 Cleaned."

install: $(TARGET)
	@echo "🔧 Run: sudo ./install.sh"
