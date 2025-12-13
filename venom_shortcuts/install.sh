#!/bin/bash

# ==============================================
# Venom Shortcuts Daemon - Installation Script
# ==============================================

set -e

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# المسارات والأسماء
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="venom_shortcuts"
SERVICE_NAME="venom-shortcuts.service"
CSV_SOURCE="venom_shortcuts.csv"
CSV_TARGET="/etc/venom/shortcuts.csv"

# تحديد المستخدم الحقيقي الذي شغّل الأمر sudo
REAL_USER=$SUDO_USER
if [ -z "$REAL_USER" ]; then
    echo -e "${RED}❌ Error: Could not detect the real user. Are you running with sudo?${NC}"
    exit 1
fi
USER_ID=$(id -u "$REAL_USER")

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   🐍 Venom Shortcuts Daemon Installer      ║${NC}"
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
    echo "   Compile first!"
    exit 1
fi

# 🛑 إيقاف أي نسخة تعمل حالياً لتحرير الملفات
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
    echo -e "${RED}⚠️ Warning: Service file '${SERVICE_NAME}' not found in current directory.${NC}"
fi

# 3. إعداد مجلد الإعدادات ونسخ ملف الاختصارات CSV
echo -e "${BLUE}📁 Setting up config directory (/etc/venom)...${NC}"
mkdir -p /etc/venom

if [[ -f "${SCRIPT_DIR}/${CSV_SOURCE}" ]]; then
    echo -e "${BLUE}📝 Copying shortcuts CSV to ${CSV_TARGET}...${NC}"
    cp "${SCRIPT_DIR}/${CSV_SOURCE}" "${CSV_TARGET}"
else
    echo -e "${BLUE}📝 Creating new empty shortcuts file at ${CSV_TARGET}...${NC}"
    touch "${CSV_TARGET}"
fi

# 4. ضبط الصلاحيات (مهم جداً)
echo -e "${BLUE}🔐 Fixing permissions...${NC}"
# السماح للجميع بالكتابة في المجلد (لأن العفريت يعمل كمستخدم)
chmod 777 /etc/venom
# السماح بقراءة وكتابة ملف CSV
chmod 666 "${CSV_TARGET}"
# تعيين المالك للمستخدم الحالي
chown -R "$REAL_USER:$REAL_USER" /etc/venom

# 5. تفعيل الخدمة للمستخدم
echo -e "${BLUE}🔄 Enabling service for user: ${REAL_USER}...${NC}"

# ضبط متغيرات البيئة لتشغيل systemctl كمستخدم
export XDG_RUNTIME_DIR="/run/user/$USER_ID"

su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable --now ${SERVICE_NAME}"
# إعادة تشغيل إجبارية للتأكد من تحميل الملف الجديد
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user restart ${SERVICE_NAME}"

echo ""
echo -e "${GREEN}✅ Venom Shortcuts Daemon installed successfully!${NC}"
echo ""
echo "   Status Check:  systemctl --user status ${SERVICE_NAME}"
echo "   Config File:   ${CSV_TARGET}"