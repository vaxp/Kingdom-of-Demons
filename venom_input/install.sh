#!/bin/bash

# ==============================================
# Venom Input Daemon - Installation Script (User Mode)
# ==============================================

set -e

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# المسارات
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="venom_input"
SERVICE_NAME="venom-input.service"

# تحديد المستخدم الحقيقي الذي شغّل الأمر sudo
REAL_USER=$SUDO_USER
if [ -z "$REAL_USER" ]; then
    echo -e "${RED}❌ Error: Could not detect the real user. Are you running with sudo?${NC}"
    exit 1
fi
USER_ID=$(id -u "$REAL_USER")

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   🐍 Venom Input Daemon Installer          ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""

# التحقق من صلاحيات الروت
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}❌ Error: This script must be run as root${NC}"
    echo "   sudo ./install.sh"
    exit 1
fi

# التحقق من وجود الملفات
if [[ ! -f "${SCRIPT_DIR}/${BINARY_NAME}" ]]; then
    echo -e "${RED}❌ Error: Binary '${BINARY_NAME}' not found!${NC}"
    echo "   Compile first: gcc -o venom_input venom_input.c \$(pkg-config --cflags --libs gio-2.0)"
    exit 1
fi

# 1. نسخ البرنامج (يحتاج روت)
echo -e "${BLUE}📦 Installing binary to /usr/bin/...${NC}"
cp "${SCRIPT_DIR}/${BINARY_NAME}" /usr/bin/
chmod +x /usr/bin/${BINARY_NAME}

# 2. نسخ ملف الخدمة إلى مسار خدمات المستخدم
echo -e "${BLUE}📄 Installing service file (User Mode)...${NC}"
# التأكد من وجود المجلد
mkdir -p /usr/lib/systemd/user/
cp "${SCRIPT_DIR}/${SERVICE_NAME}" /usr/lib/systemd/user/

# 3. إعداد مجلد الإعدادات وصلاحياته
echo -e "${BLUE}📁 Creating config directory & Fixing permissions...${NC}"
mkdir -p /etc/venom
touch /etc/venom/input.json

# بما أن العفريت سيعمل كمستخدم، يجب فتح الصلاحيات للكتابة
chmod 777 /etc/venom
chmod 666 /etc/venom/input.json
# تغيير المالك ليكون المستخدم الحالي (اختياري لكن أفضل)
chown -R "$REAL_USER:$REAL_USER" /etc/venom

# 4. تفعيل الخدمة للمستخدم
echo -e "${BLUE}🔄 Enabling service for user: ${REAL_USER}...${NC}"

# نقوم بتنفيذ أوامر systemctl بصلاحيات المستخدم الأصلي وليس الروت
# هذا يتطلب تحديد XDG_RUNTIME_DIR يدوياً لأن su قد لا ينقله
export XDG_RUNTIME_DIR="/run/user/$USER_ID"

su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable --now ${SERVICE_NAME}"

echo ""
echo -e "${GREEN}✅ Venom Input Daemon installed successfully!${NC}"
echo ""
echo "   Status Check:  systemctl --user status ${SERVICE_NAME}"
echo "   Logs Check:    journalctl --user -u ${SERVICE_NAME} -f"