#!/bin/bash
set -e                  # exit on error
set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error

SOF_BIN_LINK="https://github.com/thesofproject/sof-bin/releases/download/v2025.05/sof-bin-2025.05.tar.gz"
ALSA_UCM_CONF_LINK="https://git.aiursoft.cn/PublicVault/alsa-ucm-conf/archive/master.zip"

# helper to check URL reachability
url_exists() {
    local url="$1"
    # use wget --spider to test, short timeout and few tries
    wget --spider --timeout=10 --tries=2 "$url" >/dev/null 2>&1
}

(
    print_ok "Installing Intel SOF Mod"
    tempdir=$(mktemp -d)
    # ensure cleanup of tempdir even on early exit in this subshell
    trap 'rm -rf "$tempdir" >/dev/null 2>&1 || true' EXIT

    print_ok "Preparing installation directory $tempdir"
    cd "$tempdir" || exit 1

    # Flags to decide whether we proceed with each bundle
    DO_SOF=true
    DO_ALSA=true

    # -----------------------
    # SOF binaries
    # -----------------------
    print_ok "Checking SOF binaries link availability..."
    if url_exists "$SOF_BIN_LINK"; then
        print_ok "SOF link reachable. Downloading SOF binaries..."
        if wget "$SOF_BIN_LINK" -O sof-bin.tar.gz; then
            judge "Downloaded SOF binaries"

            print_ok "Extracting SOF binaries"
            tar -xzf sof-bin.tar.gz
            judge "Extracted SOF binaries"

            print_ok "Removing old SOF binaries"
            rm -rf /lib/firmware/intel/sof* || true
            rm -rf /usr/local/bin/sof-* || true
            judge "Removed old SOF binaries"

            if [ -d "./sof-bin-2025.05" ]; then
                print_ok "Installing SOF binaries"
                cd ./sof-bin-2025.05
                # run install script but don't let it kill the whole run if it fails:
                if ./install.sh; then
                    judge "Installed SOF binaries"
                else
                    print_warn "SOF install script failed — continuing (install.sh returned non-zero)."
                fi
                cd ..
            else
                print_warn "Expected directory sof-bin-2025.05 not found after extraction; skipping SOF install."
            fi
        else
            print_warn "Failed to download SOF binaries file even though link seemed reachable. Skipping SOF steps."
        fi
    else
        print_warn "SOF download link is not reachable. Skipping SOF download and install."
        DO_SOF=false
    fi

    # -----------------------
    # ALSA UCM conf
    # -----------------------
    print_ok "Checking alsa-ucm-conf link availability..."
    if url_exists "$ALSA_UCM_CONF_LINK"; then
        print_ok "alsa-ucm-conf link reachable. Downloading..."
        if wget "$ALSA_UCM_CONF_LINK" -O ./alsa-ucm-conf.zip; then
            judge "Download alsa-ucm-conf"

            print_ok "Unzipping alsa-ucm-conf"
            mkdir -p ./alsa-ucm/
            if unzip -q -O UTF-8 ./alsa-ucm-conf.zip -d ./alsa-ucm/; then
                judge "Unzip alsa-ucm-conf"

                # locate the unpacked ucm2 folder robustly (in case archive root name changes)
                UCM_SRC_DIR=$(find ./alsa-ucm -maxdepth 2 -type d -name ucm2 | head -n 1 || true)
                if [ -n "$UCM_SRC_DIR" ]; then
                    print_ok "Copying alsa-ucm-conf to /usr/share/alsa/ucm2/"
                    rsync -Aax --update --delete "$UCM_SRC_DIR"/ /usr/share/alsa/ucm2/
                    judge "Copy alsa-ucm-conf to /usr/share/alsa/ucm2/"
                else
                    print_warn "Could not find ucm2 directory inside the archive; skipping copy."
                fi
            else
                print_warn "Failed to unzip alsa-ucm-conf archive; skipping copy."
            fi
        else
            print_warn "Failed to download alsa-ucm-conf even though link seemed reachable. Skipping."
        fi
    else
        print_warn "alsa-ucm-conf download link is not reachable. Skipping alsa-ucm steps."
        DO_ALSA=false
    fi

    # -----------------------
    # Cleanup
    # -----------------------
    print_ok "Cleaning up temporary files"
    # remove only the files we created in tempdir (safe)
    rm -rf "$tempdir"/*
    judge "Clean up alsa-ucm-conf"

    # trap will remove tempdir itself on subshell exit
)
