#!/bin/bash

# ==============================================
# Venom Shot Daemon - Installation Script
# ==============================================

set -e

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# المسارات
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="venom_shot"
SERVICE_NAME="venom-shot.service"

# تحديد المستخدم الحقيقي
REAL_USER=$SUDO_USER
if [ -z "$REAL_USER" ]; then
    echo -e "${RED}❌ Error: Could not detect the real user. Are you running with sudo?${NC}"
    exit 1
fi
USER_ID=$(id -u "$REAL_USER")

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   📸 Venom Shot Daemon Installer           ║${NC}"
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

# التحقق من تثبيت ffmpeg (مطلوب للتسجيل)
if ! command -v ffmpeg &> /dev/null; then
    echo -e "${BLUE}⚠️ ffmpeg not found. Installing...${NC}"
    apt-get update && apt-get install -y ffmpeg
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
cp "${SCRIPT_DIR}/${SERVICE_NAME}" /usr/lib/systemd/user/

# 3. إنشاء مجلد الحفظ الافتراضي
SAVE_DIR="/home/${REAL_USER}/Videos"
if [[ ! -d "${SAVE_DIR}" ]]; then
    echo -e "${BLUE}📁 Creating save directory: ${SAVE_DIR}${NC}"
    mkdir -p "${SAVE_DIR}"
    chown "$REAL_USER:$REAL_USER" "${SAVE_DIR}"
fi

# 4. تفعيل الخدمة للمستخدم
echo -e "${BLUE}🔄 Enabling service for user: ${REAL_USER}...${NC}"

export XDG_RUNTIME_DIR="/run/user/$USER_ID"

su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable --now ${SERVICE_NAME}"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user restart ${SERVICE_NAME}"

echo ""
echo -e "${GREEN}✅ Venom Shot Daemon installed successfully!${NC}"
echo ""
echo "   Status Check:  systemctl --user status ${SERVICE_NAME}"
echo "   Logs Check:    journalctl --user -u ${SERVICE_NAME} -f"
echo "   Save Path:     ${SAVE_DIR}"
echo ""
echo "   D-Bus Methods:"
echo "     - FullScreen         Take fullscreen screenshot"
echo "     - SelectRegion       Select region to capture"
echo "     - StartRecord        Record full screen"
echo "     - SelectRecord       Select region to record"
echo "     - StopRecord         Stop recording"
