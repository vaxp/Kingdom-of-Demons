# ═══════════════════════════════════════════════════════════════════════════
# Venom Shortcuts Daemon - Makefile
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags x11 gio-2.0)
LDFLAGS = $(shell pkg-config --libs x11 gio-2.0)

TARGET = venom_shortcuts
SRC = venom_shortcuts.c

all: $(TARGET)
	@echo "✅ Build complete: $(TARGET)"

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)
	@echo "🧹 Cleaned."

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/bin/$(TARGET)
	install -Dm644 venom-shortcuts.service /usr/lib/systemd/user/
	@echo "✅ Installed to /usr/bin/"

.PHONY: all clean install
