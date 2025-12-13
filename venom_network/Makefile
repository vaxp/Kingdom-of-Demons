# ═══════════════════════════════════════════════════════════════════════════
# 🌐 Venom Network Daemon - Makefile
# ═══════════════════════════════════════════════════════════════════════════

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include $(shell pkg-config --cflags glib-2.0 gio-2.0)
LDFLAGS = $(shell pkg-config --libs glib-2.0 gio-2.0)

TARGET = venom_network
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
	@echo "✅ Installed"

# ═══════════════════════════════════════════════════════════════════════════
# 🧪 Tests
# ═══════════════════════════════════════════════════════════════════════════

test-wifi:
	@dbus-send --session --print-reply --dest=org.venom.Network \
		/org/venom/Network/WiFi org.venom.Network.WiFi.GetNetworks

test-bt:
	@dbus-send --session --print-reply --dest=org.venom.Network \
		/org/venom/Network/Bluetooth org.venom.Network.Bluetooth.GetDevices

test-eth:
	@dbus-send --session --print-reply --dest=org.venom.Network \
		/org/venom/Network/Ethernet org.venom.Network.Ethernet.GetInterfaces
