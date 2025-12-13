#!/bin/bash

# ==============================================
# Venom OSD Daemon - Installation Script
# ==============================================

set -e

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# المسارات
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="venom_osd"
SERVICE_NAME="venom-osd.service"

# تحديد المستخدم الحقيقي
REAL_USER=$SUDO_USER
if [ -z "$REAL_USER" ]; then
    echo -e "${RED}❌ Error: Could not detect the real user. Are you running with sudo?${NC}"
    exit 1
fi
USER_ID=$(id -u "$REAL_USER")

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   🐍 Venom OSD Daemon Installer            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""

# التحقق من صلاحيات الروت
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}❌ Error: This script must be run as root${NC}"
    exit 1
fi

# التحقق من وجود الملف التنفيذي
if [[ ! -f "${SCRIPT_DIR}/${BINARY_NAME}" ]]; then
    echo -e "${RED}❌ Error: Binary '${BINARY_NAME}' not found!${NC}"
    echo "   Compile first: gcc venom_osd.c -o venom_osd $(pkg-config --cflags --libs x11 xfixes cairo gio-2.0) -lpthread -lm"
    exit 1
fi

# 🛑 إيقاف أي نسخة تعمل حالياً
echo -e "${BLUE}🛑 Stopping running instances...${NC}"
pkill -9 ${BINARY_NAME} || true

# 1. نسخ البرنامج
echo -e "${BLUE}📦 Installing binary to /usr/bin/...${NC}"
cp "${SCRIPT_DIR}/${BINARY_NAME}" /usr/bin/
chmod +x /usr/bin/${BINARY_NAME}

# 2. نسخ ملف الخدمة
echo -e "${BLUE}📄 Installing service file (User Mode)...${NC}"
mkdir -p /usr/lib/systemd/user/
if [[ -f "${SCRIPT_DIR}/${SERVICE_NAME}" ]]; then
    cp "${SCRIPT_DIR}/${SERVICE_NAME}" /usr/lib/systemd/user/
else
    # إنشاء ملف الخدمة تلقائياً إذا لم يكن موجوداً
    echo -e "${BLUE}⚠️ Service file not found. Generating on the fly...${NC}"
    cat <<EOF > /usr/lib/systemd/user/${SERVICE_NAME}
[Unit]
Description=Venom OSD - On Screen Display Daemon
PartOf=graphical-session.target
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/${BINARY_NAME}
Restart=on-failure
RestartSec=3

[Install]
WantedBy=graphical-session.target
EOF
fi

# 3. تفعيل الخدمة للمستخدم
echo -e "${BLUE}🔄 Enabling service for user: ${REAL_USER}...${NC}"

export XDG_RUNTIME_DIR="/run/user/$USER_ID"

su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable --now ${SERVICE_NAME}"
# إعادة تشغيل للتأكد من تحميل النسخة الجديدة
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user restart ${SERVICE_NAME}"

echo ""
echo -e "${GREEN}✅ Venom OSD Daemon installed successfully!${NC}"
echo ""
echo "   Status Check:  systemctl --user status ${SERVICE_NAME}"