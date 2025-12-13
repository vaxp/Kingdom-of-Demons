#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════
# 🔨 AVAXP SAM - Build Script
# ═══════════════════════════════════════════════════════════════════════════
# ترجمة جميع العفاريت (C)
# ═══════════════════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ألوان
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   🔨 AVAXP SAM - Build All C Daemons                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# ═══════════════════════════════════════════════════════════════════════════
# قائمة العفاريت
# ═══════════════════════════════════════════════════════════════════════════

DAEMONS=(
    "venom_input"
    "venom_osd"
    "venom_clipboard"
    "venom-auth"
    "venom_power"
    "venom_notify"
    "venom_audio"
    "venom_display"
    "venom_network"
    "venom_shortcuts"
    "venom_shot"
    "venom_sni"
)

# ═══════════════════════════════════════════════════════════════════════════
# دالة البناء
# ═══════════════════════════════════════════════════════════════════════════

build_daemon() {
    local name="$1"
    local dir="${SCRIPT_DIR}/${name}"
    
    if [ ! -d "$dir" ]; then
        echo -e "${YELLOW}⚠ Not found: ${name}${NC}"
        return 1
    fi
    
    echo -e "${CYAN}🔧 Building: ${name}${NC}"
    
    cd "$dir"
    
    if [ -f "Makefile" ]; then
        make clean 2>/dev/null || true
        if make; then
            echo -e "${GREEN}✓ Built: ${name}${NC}"
            return 0
        fi
    else
        echo -e "${YELLOW}  No Makefile found${NC}"
    fi
    
    echo -e "${RED}✗ Failed: ${name}${NC}"
    return 1
}

# ═══════════════════════════════════════════════════════════════════════════
# التنفيذ
# ═══════════════════════════════════════════════════════════════════════════

SUCCESS=0
FAILED=0

for daemon in "${DAEMONS[@]}"; do
    if build_daemon "$daemon"; then
        ((SUCCESS++))
    else
        ((FAILED++))
    fi
done

# ═══════════════════════════════════════════════════════════════════════════
# النتيجة
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Built: ${SUCCESS} daemons${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}❌ Failed: ${FAILED} daemons${NC}"
fi
echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${YELLOW}Next step: ./install.sh DESTDIR=/path/to/chroot${NC}"
