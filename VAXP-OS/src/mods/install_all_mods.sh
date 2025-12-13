#!/bin/bash

#==========================
# Set up the environment
#==========================
set -e                  # exit on error
set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error
source /root/mods/shared.sh
source /root/mods/args.sh

#==========================
# Variables for mods
#==========================
print_ok "Building variables for mods:"

echo "TARGET_UBUNTU_VERSION=$TARGET_UBUNTU_VERSION"
echo "BUILD_UBUNTU_MIRROR=$BUILD_UBUNTU_MIRROR"
echo "TARGET_NAME=$TARGET_NAME"
echo "TARGET_BUSINESS_NAME=$TARGET_BUSINESS_NAME"
echo "TARGET_BUILD_VERSION=$TARGET_BUILD_VERSION"

#==========================
# Execute mods
#==========================
STATE_DIR="/root/mods/.state"
mkdir -p "$STATE_DIR"

for mod in "$SCRIPT_DIR"/*; do
    if [[ -d "$mod" && -f "$mod/install.sh" ]]; then
        mod_name=$(basename "$mod")
        state_file="$STATE_DIR/${mod_name}.done"

        if [[ -f "$state_file" ]]; then
            print_ok "Skipping mod: $mod_name (already done)"
            continue
        fi

        print_info "Processing mod: $mod"
        (
            cd "$mod" && \
            chmod +x install.sh && \
            bash "$mod/install.sh"
        )
        
        # If execution reaches here, the mod succeeded (because of set -e)
        touch "$state_file"
    fi
done
