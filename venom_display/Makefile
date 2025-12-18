# ═══════════════════════════════════════════════════════════════════════════
# 🖥️ Venom Display Daemon - Makefile
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include $(shell pkg-config --cflags glib-2.0 gio-2.0 x11 xrandr)
LDFLAGS = $(shell pkg-config --libs glib-2.0 gio-2.0 x11 xrandr) -lXext -lm

TARGET = venom_display
BUILD_DIR = build
SRC_DIR = src

SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/dbus_service.c \
          $(SRC_DIR)/display_core.c \
          $(SRC_DIR)/display_mode.c \
          $(SRC_DIR)/display_layout.c \
          $(SRC_DIR)/display_night.c \
          $(SRC_DIR)/display_profile.c \
          $(SRC_DIR)/display_dpms.c \
          $(SRC_DIR)/display_backlight.c \
          $(SRC_DIR)/display_edid.c \
          $(SRC_DIR)/display_ddc.c \
          $(SRC_DIR)/display_vrr.c \
          $(SRC_DIR)/display_auto.c \
          $(SRC_DIR)/display_arrange.c \
          $(SRC_DIR)/display_persist.c

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

test-dpms:
	@echo "🔌 Testing DPMS..."
	@dbus-send --session --print-reply --dest=org.venom.Display \
		/org/venom/Display org.venom.Display.GetDPMSMode

test-backlight:
	@echo "💡 Testing Backlight..."
	@dbus-send --session --print-reply --dest=org.venom.Display \
		/org/venom/Display org.venom.Display.GetBacklight

test-edid:
	@echo "📋 Testing EDID..."
	@dbus-send --session --print-reply --dest=org.venom.Display \
		/org/venom/Display org.venom.Display.GetEDID string:"eDP-1"
