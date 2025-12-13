# ═══════════════════════════════════════════════════════════════════════════
# 🔍 Venom Basilisk - Makefile
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0 gio-2.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0 gio-2.0)

TARGET = venom_basilisk

SRCS = main.c window.c search.c commands.c ai_chat.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install

all: $(TARGET)
	@echo "✅ Build complete: $(TARGET)"

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
main.o: main.c basilisk.h window.h search.h
window.o: window.c window.h basilisk.h search.h commands.h
search.o: search.c search.h basilisk.h window.h
commands.o: commands.c commands.h basilisk.h window.h ai_chat.h
ai_chat.o: ai_chat.c ai_chat.h basilisk.h

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "🧹 Cleaned."

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/bin/$(TARGET)
	@echo "✅ Installed to /usr/bin/$(TARGET)"
