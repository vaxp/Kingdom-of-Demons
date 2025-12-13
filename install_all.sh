#!/bin/bash

# ==============================================
# Venom Desktop Environment - Master Installer
# ==============================================
# يقوم بتثبيت جميع العفاريت واحداً تلو الآخر

# لا نستخدم set -e هنا لأننا نريد الاستمرار حتى لو فشل عفريت واحد

# الألوان
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# المسار الرئيسي
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# قائمة العفاريت بالترتيب المطلوب للتثبيت
# (بعض العفاريت يجب أن تُثبّت قبل غيرها)
DAEMONS=(
    "venom_input"       # لغات الكيبورد (أساسي)
    "venom_osd"         # OSD للغة (يعتمد على input)
    "venom-auth"        # المصادقة (حيوي)
    "venom_power"       # إدارة الطاقة (حيوي)
    "venom_notify"      # الإشعارات (يعتمد عليه الآخرون)
    "venom_audio"       # الصوت
    "venom_display"     # الشاشات
    "venom_network"     # الشبكات
    "venom_shortcuts"   # اختصارات لوحة المفاتيح
    "venom_shot"        # لقطات الشاشة
    "venom_sni"         # System Tray
    "venom_clipboard"   # الحافظة
)

# متغيرات الإحصائيات
INSTALLED=0
FAILED=0
SKIPPED=0

echo -e "${CYAN}"
echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║                                                                  ║"
echo "║   🐍 Venom Desktop Environment - Master Installer                ║"
echo "║                                                                  ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""

# التحقق من صلاحيات الروت
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}❌ Error: This script must be run as root${NC}"
    echo "   sudo ./install_all.sh"
    exit 1
fi

# التحقق من وجود SUDO_USER
if [ -z "$SUDO_USER" ]; then
    echo -e "${RED}❌ Error: Could not detect the real user.${NC}"
    echo "   Run with: sudo ./install_all.sh"
    exit 1
fi

echo -e "${BLUE}👤 Installing for user: ${SUDO_USER}${NC}"
echo -e "${BLUE}📂 Project directory: ${SCRIPT_DIR}${NC}"
echo ""

# إيقاف جميع الخدمات والعمليات أولاً
echo -e "${YELLOW}🛑 Stopping all Venom services and processes...${NC}"
echo ""

# الحصول على USER_ID
USER_ID=$(id -u "$SUDO_USER")

# إيقاف جميع خدمات systemd
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-audio.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-display.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-network.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-notify.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-power.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-shot.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-sni.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-input.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-shortcuts.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-osd.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-auth.service 2>/dev/null" || true
su - "$SUDO_USER" -c "export XDG_RUNTIME_DIR=/run/user/$USER_ID; systemctl --user stop venom-clipboard.service 2>/dev/null" || true

# إيقاف أي عمليات متبقية
pkill -9 venom_audio 2>/dev/null || true
pkill -9 venom_display 2>/dev/null || true
pkill -9 venom_network 2>/dev/null || true
pkill -9 venom_notify 2>/dev/null || true
pkill -9 venom_power 2>/dev/null || true
pkill -9 venom_shot 2>/dev/null || true
pkill -9 venom_sni 2>/dev/null || true
pkill -9 venom_input 2>/dev/null || true
pkill -9 venom_shortcuts 2>/dev/null || true
pkill -9 venom_osd 2>/dev/null || true
pkill -9 venom_auth 2>/dev/null || true
pkill -9 venom_clipboard 2>/dev/null || true

# انتظار قليل للتأكد من إيقاف كل شيء
sleep 2

echo -e "${GREEN}✅ All services stopped${NC}"
echo ""
echo -e "${YELLOW}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# دالة تثبيت عفريت واحد
install_daemon() {
    local daemon_name=$1
    local daemon_path="${SCRIPT_DIR}/${daemon_name}"
    local install_script="${daemon_path}/install.sh"
    
    echo -e "${CYAN}┌──────────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│  Installing: ${daemon_name}${NC}"
    echo -e "${CYAN}└──────────────────────────────────────────────────────────────┘${NC}"
    
    # التحقق من وجود المجلد
    if [[ ! -d "${daemon_path}" ]]; then
        echo -e "${YELLOW}⚠️  Directory not found: ${daemon_path}${NC}"
        echo -e "${YELLOW}   Skipping...${NC}"
        ((SKIPPED++))
        echo ""
        return
    fi
    
    # التحقق من وجود سكربت التثبيت
    if [[ ! -f "${install_script}" ]]; then
        echo -e "${YELLOW}⚠️  Install script not found: ${install_script}${NC}"
        echo -e "${YELLOW}   Skipping...${NC}"
        ((SKIPPED++))
        echo ""
        return
    fi
    
    # ترجمة الكود إذا وجد Makefile
    if [[ -f "${daemon_path}/Makefile" ]]; then
        echo -e "${BLUE}🔧 Compiling ${daemon_name}...${NC}"
        cd "${daemon_path}"
        make clean 2>/dev/null || true
        if ! make -B 2>&1; then
            echo -e "${RED}❌ Compilation failed for ${daemon_name}${NC}"
            ((FAILED++))
            echo ""
            cd "${SCRIPT_DIR}"
            return
        fi
        echo -e "${GREEN}✅ Compiled successfully${NC}"
        cd "${SCRIPT_DIR}"
    fi
    
    # التحقق من وجود الملف التنفيذي
    local binary_name=$(echo "$daemon_name" | sed 's/-/_/g')
    if [[ ! -f "${daemon_path}/${binary_name}" ]]; then
        echo -e "${YELLOW}⚠️  Binary not found: ${daemon_path}/${binary_name}${NC}"
        echo -e "${YELLOW}   Skipping...${NC}"
        ((SKIPPED++))
        echo ""
        return
    fi
    
    # تشغيل سكربت التثبيت
    echo -e "${BLUE}🚀 Running install script...${NC}"
    
    cd "${daemon_path}"
    if bash "${install_script}"; then
        echo -e "${GREEN}✅ ${daemon_name} installed successfully!${NC}"
        ((INSTALLED++))
    else
        echo -e "${RED}❌ Failed to install ${daemon_name}${NC}"
        ((FAILED++))
    fi
    
    echo ""
    cd "${SCRIPT_DIR}"
}

# تثبيت جميع العفاريت
for daemon in "${DAEMONS[@]}"; do
    install_daemon "$daemon"
done

# ملخص التثبيت
echo -e "${CYAN}"
echo "╔══════════════════════════════════════════════════════════════════╗"
echo "║                    📊 Installation Summary                       ║"
echo "╚══════════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""
echo -e "   ${GREEN}✅ Installed successfully: ${INSTALLED}${NC}"
echo -e "   ${YELLOW}⚠️  Skipped:               ${SKIPPED}${NC}"
echo -e "   ${RED}❌ Failed:                 ${FAILED}${NC}"
echo ""

if [[ $FAILED -eq 0 && $SKIPPED -eq 0 ]]; then
    echo -e "${GREEN}🎉 All daemons installed successfully!${NC}"
elif [[ $FAILED -eq 0 ]]; then
    echo -e "${YELLOW}⚠️  Some daemons were skipped. Check if binaries are compiled.${NC}"
else
    echo -e "${RED}❌ Some installations failed. Check the logs above.${NC}"
fi

echo ""
echo -e "${BLUE}📋 To check status of all services:${NC}"
echo "   systemctl --user list-units 'venom-*'"
echo ""
echo -e "${BLUE}📋 To view logs:${NC}"
echo "   journalctl --user -u venom-[name].service -f"
echo ""
