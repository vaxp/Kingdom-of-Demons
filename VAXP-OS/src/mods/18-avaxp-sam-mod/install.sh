#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════
# 🐍 AVAXP SAM (System Abstraction Module) - Build Install Script
# ═══════════════════════════════════════════════════════════════════════════
# هذا السكربت للتثبيت في بيئة chroot/build فقط
# لا يستخدم systemctl أو أي أوامر تتطلب نظام يعمل
# ═══════════════════════════════════════════════════════════════════════════

set -e

# المسار الهدف (للتثبيت في chroot)
DESTDIR="${DESTDIR:-}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ألوان
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   🐍 AVAXP SAM - System Abstraction Module Installer       ║"
echo "║   Build Environment Mode (chroot)                          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

if [ -n "$DESTDIR" ]; then
    echo -e "${YELLOW}📁 Target: ${DESTDIR}${NC}"
else
    echo -e "${YELLOW}📁 Target: / (live system)${NC}"
fi

# ═══════════════════════════════════════════════════════════════════════════
# قائمة العفاريت
# ═══════════════════════════════════════════════════════════════════════════

DAEMONS=(
    "venom_input"       # لغات الكيبورد (أساسي)
    "venom_osd"         # OSD للغة
    "venom_clipboard"   # مدير الحافظة
    "venom-auth"        # المصادقة
    "venom_power"       # إدارة الطاقة
    "venom_notify"      # الإشعارات
    "venom_audio"       # الصوت
    "venom_display"     # الشاشة
    "venom_network"     # الشبكة
    "venom_shortcuts"   # اختصارات لوحة المفاتيح
    "venom_shot"        # لقطات الشاشة
    "venom_sni"         # أيقونات صينية النظام
)

# ═══════════════════════════════════════════════════════════════════════════
# دوال التثبيت
# ═══════════════════════════════════════════════════════════════════════════

install_binary() {
    local src="$1"
    local name="$2"
    
    if [ -f "$src" ]; then
        install -Dm755 "$src" "${DESTDIR}/usr/bin/${name}"
        echo -e "  ${GREEN}✓${NC} Binary: /usr/bin/${name}"
        return 0
    fi
    return 1
}

install_service() {
    local src="$1"
    local dest_name="$2"
    local service_type="$3"  # user أو system
    
    if [ -f "$src" ]; then
        if [ "$service_type" = "user" ]; then
            install -Dm644 "$src" "${DESTDIR}/usr/lib/systemd/user/${dest_name}"
            echo -e "  ${GREEN}✓${NC} Service (user): /usr/lib/systemd/user/${dest_name}"
        else
            install -Dm644 "$src" "${DESTDIR}/usr/lib/systemd/system/${dest_name}"
            echo -e "  ${GREEN}✓${NC} Service (system): /usr/lib/systemd/system/${dest_name}"
        fi
        return 0
    fi
    return 1
}

install_config() {
    local src="$1"
    local dest="$2"
    
    if [ -f "$src" ]; then
        install -Dm644 "$src" "${DESTDIR}${dest}"
        echo -e "  ${GREEN}✓${NC} Config: ${dest}"
        return 0
    fi
    return 1
}

# ═══════════════════════════════════════════════════════════════════════════
# تثبيت كل عفريت
# ═══════════════════════════════════════════════════════════════════════════

install_daemon() {
    local daemon_name="$1"
    local daemon_dir="${SCRIPT_DIR}/${daemon_name}"
    
    if [ ! -d "$daemon_dir" ]; then
        echo -e "${RED}✗ Not found: ${daemon_name}${NC}"
        return 1
    fi
    
    echo -e "\n${CYAN}📦 Installing: ${daemon_name}${NC}"
    
    # البحث عن الملف التنفيذي
    local binary=""
    for bin in "$daemon_dir/$daemon_name" "$daemon_dir/target/release/$daemon_name" "$daemon_dir/build/$daemon_name"; do
        if [ -f "$bin" ] && [ -x "$bin" ]; then
            binary="$bin"
            break
        fi
    done
    
    if [ -n "$binary" ]; then
        install_binary "$binary" "$daemon_name"
    else
        echo -e "  ${YELLOW}⚠${NC} No binary found (may need compilation)"
    fi
    
    # البحث عن ملفات الخدمة
    for service_file in "$daemon_dir"/*.service; do
        if [ -f "$service_file" ]; then
            local service_name=$(basename "$service_file")
            # تحديد نوع الخدمة من المحتوى
            if grep -q "WantedBy=default.target" "$service_file" 2>/dev/null; then
                install_service "$service_file" "$service_name" "user"
            else
                install_service "$service_file" "$service_name" "system"
            fi
        fi
    done
    
    # البحث عن ملفات الإعداد
    if [ -d "$daemon_dir/config" ]; then
        for config_file in "$daemon_dir/config"/*; do
            if [ -f "$config_file" ]; then
                local config_name=$(basename "$config_file")
                install_config "$config_file" "/etc/venom/${config_name}"
            fi
        done
    fi
    
    return 0
}

# ═══════════════════════════════════════════════════════════════════════════
# التنفيذ
# ═══════════════════════════════════════════════════════════════════════════

# إنشاء المجلدات المطلوبة
mkdir -p "${DESTDIR}/usr/bin"
mkdir -p "${DESTDIR}/usr/lib/systemd/user"
mkdir -p "${DESTDIR}/usr/lib/systemd/system"
mkdir -p "${DESTDIR}/etc/venom"

# تثبيت العفاريت
SUCCESS=0
FAILED=0

for daemon in "${DAEMONS[@]}"; do
    if install_daemon "$daemon"; then
        ((SUCCESS++))
    else
        ((FAILED++))
    fi
done

# ═══════════════════════════════════════════════════════════════════════════
# تثبيت ملف Preset (للتفعيل التلقائي)
# ═══════════════════════════════════════════════════════════════════════════

echo -e "\n${CYAN}📋 Installing preset file...${NC}"
mkdir -p "${DESTDIR}/usr/lib/systemd/user-preset"
if [ -f "${SCRIPT_DIR}/50-venom.preset" ]; then
    install -Dm644 "${SCRIPT_DIR}/50-venom.preset" "${DESTDIR}/usr/lib/systemd/user-preset/50-venom.preset"
    echo -e "  ${GREEN}✓${NC} Preset: /usr/lib/systemd/user-preset/50-venom.preset"
    echo -e "  ${GREEN}ℹ${NC} Services will be auto-enabled on first login"
else
    echo -e "  ${YELLOW}⚠${NC} Preset file not found"
fi

# ═══════════════════════════════════════════════════════════════════════════
# النتيجة
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Installed: ${SUCCESS} daemons${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}❌ Failed: ${FAILED} daemons${NC}"
fi
echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"

if [ -z "$DESTDIR" ]; then
    echo ""
    echo -e "${YELLOW}ℹ️  To enable services after boot:${NC}"
    echo "   systemctl --user enable --now venom-*.service"
fi
