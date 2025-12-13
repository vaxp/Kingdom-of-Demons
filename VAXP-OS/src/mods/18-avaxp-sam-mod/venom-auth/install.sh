#!/bin/bash

# ==============================================
# Venom Auth Agent - Installer for Custom Distro
# ==============================================

set -e

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

# المسارات
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="venom_auth"
SERVICE_NAME="venom-auth.service"
PAM_CONFIG_FILE="/etc/pam.d/venom_auth"

# تحديد المستخدم (لأننا نشغل السكربت بـ sudo)
REAL_USER=$SUDO_USER
if [ -z "$REAL_USER" ]; then
    echo -e "${RED}❌ Error: Run with sudo!${NC}"
    exit 1
fi
USER_ID=$(id -u "$REAL_USER")

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   🛡️ Venom Auth Installer (Hybrid Mode)     ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""

if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}❌ Error: This script must be run as root${NC}"
    exit 1
fi

# 1. إيقاف النسخ القديمة
echo -e "${BLUE}🛑 Stopping running instances...${NC}"
pkill -9 ${BINARY_NAME} || true

# 2. تثبيت الملف التنفيذي
echo -e "${BLUE}📦 Installing binary to /usr/bin/...${NC}"
if [[ ! -f "${SCRIPT_DIR}/${BINARY_NAME}" ]]; then
    echo -e "${RED}❌ Error: Binary not found! Compile it first.${NC}"
    exit 1
fi
cp "${SCRIPT_DIR}/${BINARY_NAME}" /usr/bin/
chmod +x /usr/bin/${BINARY_NAME}

# 3. تكوين PAM (أخطر وأهم خطوة للمصادقة)
# هذا ما يسمح للعفريت بقراءة كلمات السر عبر common-auth
echo -e "${BLUE}🔑 Configuring PAM service (${PAM_CONFIG_FILE})...${NC}"
cat <<EOF > ${PAM_CONFIG_FILE}
# PAM configuration for Venom Auth
# نعتمد على إعدادات أوبنتو الأساسية (debootstrap يسحبها تلقائياً)
@include common-auth
@include common-account
@include common-password
@include common-session
EOF

# 4. تثبيت الخدمة (User Service)
echo -e "${BLUE}📄 Installing service file...${NC}"
mkdir -p /usr/lib/systemd/user/

# إنشاء ملف الخدمة مباشرة هنا لضمان صحة المحتوى
cat <<EOF > /usr/lib/systemd/user/${SERVICE_NAME}
[Unit]
Description=Venom Authentication Agent & Lock Screen
Documentation=https://github.com/vaxp/venom-auth
# يعمل مع الجلسة الرسومية
PartOf=graphical-session.target
After=graphical-session.target

[Service]
Type=simple
ExecStart=/usr/bin/${BINARY_NAME}
# إعادة تشغيل فورية لأنه حرج جداً
Restart=always
RestartSec=0
# ضبط الشاشة ضروري لنوافذ القفل
Environment=DISPLAY=:0

[Install]
WantedBy=graphical-session.target
EOF

# 5. التفعيل والتشغيل
echo -e "${BLUE}🔄 Enabling service for user: ${REAL_USER}...${NC}"

export XDG_RUNTIME_DIR="/run/user/$USER_ID"

su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user daemon-reload"
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user enable --now ${SERVICE_NAME}"
# إعادة تشغيل للتأكد من أن ملف PAM الجديد تم التعرف عليه
su - "$REAL_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user restart ${SERVICE_NAME}"

echo ""
echo -e "${GREEN}✅ Venom Auth installed successfully!${NC}"
echo "   PAM Config:    OK"
echo "   Polkit Agent:  Registered via Code"
echo "   Service:       Active"