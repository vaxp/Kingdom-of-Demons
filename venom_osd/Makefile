# ═══════════════════════════════════════════════════════════════════════════
# Venom OSD Daemon - Makefile (GTK3)
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lX11 -lm

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
