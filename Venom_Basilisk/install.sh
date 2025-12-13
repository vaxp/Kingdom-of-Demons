#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════
# 🔍 Venom Basilisk - Installer
# ═══════════════════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DAEMON_NAME="venom_basilisk"
SERVICE_NAME="venom-basilisk.service"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   🔍 Venom Basilisk - Spotlight Launcher Installer         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}❌ Please run as root (sudo)${NC}"
    exit 1
fi

# Stop service
echo -e "${YELLOW}🛑 Stopping running instances...${NC}"
REAL_USER="${SUDO_USER:-$USER}"
USER_ID=$(id -u "$REAL_USER")
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop $SERVICE_NAME 2>/dev/null" || true
pkill -9 $DAEMON_NAME 2>/dev/null || true

# Install binary
echo -e "${YELLOW}📦 Installing binary...${NC}"
install -Dm755 "$SCRIPT_DIR/$DAEMON_NAME" "/usr/bin/$DAEMON_NAME"
echo -e "  ${GREEN}✓${NC} /usr/bin/$DAEMON_NAME"

# Install service
echo -e "${YELLOW}📄 Installing service file...${NC}"
install -Dm644 "$SCRIPT_DIR/$SERVICE_NAME" "/usr/lib/systemd/user/$SERVICE_NAME"
echo -e "  ${GREEN}✓${NC} /usr/lib/systemd/user/$SERVICE_NAME"

# Enable service
echo -e "${YELLOW}🔄 Enabling service for user: ${REAL_USER}${NC}"
export XDG_RUNTIME_DIR="/run/user/$USER_ID"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable $SERVICE_NAME"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user start $SERVICE_NAME"

echo ""
echo -e "${GREEN}✅ Venom Basilisk installed successfully!${NC}"
echo ""
echo -e "   ${CYAN}Status:${NC} systemctl --user status $SERVICE_NAME"
echo ""
echo -e "${CYAN}📋 Usage:${NC}"
echo "   Super+Space  - Toggle launcher (add to shortcuts)"
echo "   (text)       - Search apps"
echo "   vater:cmd    - Run terminal command"
echo "   !:expr       - Calculate math"
echo "   vafile:name  - Search files"
echo "   g:query      - Search GitHub"
echo "   s:query      - Search Google"
echo "   ai:question  - Ask Admiral AI"
echo ""
echo -e "${CYAN}📋 D-Bus Control:${NC}"
echo "   dbus-send --session --dest=org.venom.Basilisk /org/venom/Basilisk org.venom.Basilisk.Toggle"
