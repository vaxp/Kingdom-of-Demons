# ═══════════════════════════════════════════════════════════════════════════
# 🖥️ Venom Display Daemon - Makefile
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include $(shell pkg-config --cflags glib-2.0 gio-2.0 x11 xrandr)
LDFLAGS = $(shell pkg-config --libs glib-2.0 gio-2.0 x11 xrandr) -lm

TARGET = venom_display
BUILD_DIR = build
SRC_DIR = src

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean install

all: $(TARGET)
	@echo "✅ Build complete: $(TARGET)"

$(TARGET): $(OBJECTS)
	@echo "🔗 Linking $(TARGET)..."
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "📦 Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@echo "🧹 Cleaning..."
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "✅ Clean complete"

install: $(TARGET)
	@echo "📦 Installing..."
	@install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)
	@echo "✅ Installed to /usr/local/bin/$(TARGET)"

# ═══════════════════════════════════════════════════════════════════════════
# 🧪 اختبارات
# ═══════════════════════════════════════════════════════════════════════════

test-displays:
	@echo "🖥️ Testing GetDisplays..."
	@dbus-send --session --print-reply --dest=org.venom.Display \
		/org/venom/Display org.venom.Display.GetDisplays

test-info:
	@echo "📺 Testing display info..."
	@dbus-send --session --print-reply --dest=org.venom.Display \
		/org/venom/Display org.venom.Display.GetPrimaryDisplay
