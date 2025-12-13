#!/bin/bash

# ==============================================
# Venom Network Daemon - Installation Script
# ==============================================

set -e

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# المسارات
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="venom_network"
SERVICE_NAME="venom-network.service"

# تحديد المستخدم الحقيقي
REAL_USER=$SUDO_USER
if [ -z "$REAL_USER" ]; then
    echo -e "${RED}❌ Error: Could not detect the real user. Are you running with sudo?${NC}"
    exit 1
fi
USER_ID=$(id -u "$REAL_USER")

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   🌐 Venom Network Daemon Installer        ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""

# التحقق من صلاحيات الروت
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}❌ Error: This script must be run as root${NC}"
    echo "   sudo ./install.sh"
    exit 1
fi

# التحقق من وجود الملف التنفيذي
if [[ ! -f "${SCRIPT_DIR}/${BINARY_NAME}" ]]; then
    echo -e "${RED}❌ Error: Binary '${BINARY_NAME}' not found!${NC}"
    echo "   Compile first: make"
    exit 1
fi

# 0. تثبيت المتطلبات (BlueZ + NetworkManager + rfkill)
echo -e "${BLUE}📦 Checking and installing dependencies...${NC}"

# التحقق من BlueZ
if ! command -v bluetoothctl &> /dev/null; then
    echo -e "${BLUE}   Installing BlueZ...${NC}"
    apt-get update -qq
    apt-get install -y bluez bluez-tools
else
    echo -e "${GREEN}   ✓ BlueZ already installed${NC}"
fi

# التحقق من rfkill (مطلوب لإدارة حظر Bluetooth)
if ! command -v rfkill &> /dev/null; then
    echo -e "${BLUE}   Installing rfkill...${NC}"
    apt-get install -y rfkill
else
    echo -e "${GREEN}   ✓ rfkill already installed${NC}"
fi

# التحقق من NetworkManager
if ! command -v nmcli &> /dev/null; then
    echo -e "${BLUE}   Installing NetworkManager...${NC}"
    apt-get install -y network-manager
else
    echo -e "${GREEN}   ✓ NetworkManager already installed${NC}"
fi

echo ""

# ==============================================
# 🔵 إعداد وإصلاح BlueZ
# ==============================================
echo -e "${BLUE}🔵 Setting up Bluetooth...${NC}"

# التأكد من تشغيل خدمة البلوتوث
echo -e "${BLUE}   Enabling bluetooth.service...${NC}"
systemctl enable bluetooth.service 2>/dev/null || true
systemctl start bluetooth.service 2>/dev/null || true

# فتح حظر Bluetooth (rfkill unblock)
echo -e "${BLUE}   Checking rfkill status...${NC}"
if rfkill list bluetooth 2>/dev/null | grep -q "Soft blocked: yes"; then
    echo -e "${YELLOW}   ⚠️  Bluetooth is soft-blocked, unblocking...${NC}"
    rfkill unblock bluetooth
    echo -e "${GREEN}   ✓ Bluetooth unblocked${NC}"
else
    echo -e "${GREEN}   ✓ Bluetooth is not blocked${NC}"
fi

# التحقق من Hard block
if rfkill list bluetooth 2>/dev/null | grep -q "Hard blocked: yes"; then
    echo -e "${RED}   ⚠️  WARNING: Bluetooth is hardware-blocked!${NC}"
    echo -e "${RED}      Please check your laptop's physical Bluetooth switch or BIOS settings.${NC}"
fi

# انتظار قليلاً حتى يستقر BlueZ
sleep 1

# تشغيل Bluetooth adapter
echo -e "${BLUE}   Powering on Bluetooth adapter...${NC}"
if bluetoothctl power on 2>/dev/null; then
    echo -e "${GREEN}   ✓ Bluetooth powered on${NC}"
else
    echo -e "${YELLOW}   ⚠️  Could not power on Bluetooth (may need manual intervention)${NC}"
fi

# عرض حالة Bluetooth
echo -e "${BLUE}   Bluetooth Status:${NC}"
bluetoothctl show 2>/dev/null | grep -E "Name:|Address:|Powered:|Discoverable:" | sed 's/^/      /'

echo ""

# ==============================================
# 🛑 إيقاف أي نسخة تعمل حالياً
# ==============================================
echo -e "${BLUE}🛑 Stopping running instances...${NC}"
pkill -9 ${BINARY_NAME} 2>/dev/null || true

# 1. نسخ البرنامج
echo -e "${BLUE}📦 Installing binary to /usr/bin/...${NC}"
cp "${SCRIPT_DIR}/${BINARY_NAME}" /usr/bin/
chmod +x /usr/bin/${BINARY_NAME}

# 2. نسخ ملف الخدمة
echo -e "${BLUE}📄 Installing service file (User Mode)...${NC}"
mkdir -p /usr/lib/systemd/user/
cp "${SCRIPT_DIR}/${SERVICE_NAME}" /usr/lib/systemd/user/

# 3. تفعيل الخدمة للمستخدم
echo -e "${BLUE}🔄 Enabling service for user: ${REAL_USER}...${NC}"

export XDG_RUNTIME_DIR="/run/user/$USER_ID"

su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable --now ${SERVICE_NAME}"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user restart ${SERVICE_NAME}"

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   ✅ Installation Complete!                ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
echo ""
echo -e "   ${BLUE}Service Status:${NC}  systemctl --user status ${SERVICE_NAME}"
echo -e "   ${BLUE}View Logs:${NC}       journalctl --user -u ${SERVICE_NAME} -f"
echo -e "   ${BLUE}Bluetooth:${NC}       bluetoothctl show"
echo ""
echo -e "${GREEN}💡 Tip: If Bluetooth doesn't work after reboot, run:${NC}"
echo -e "   sudo rfkill unblock bluetooth && bluetoothctl power on"
echo ""
