set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error

# remove unused and clean up apt cache
print_ok "Removing unused packages..."
apt autoremove -y --purge || true
judge "Remove unused packages"

EXIT_IF_UNNECESSARY_PACKAGE_FOUND=0

print_ok "Purging unnecessary packages"
packages=(
    gnome-mahjongg
    gnome-mines
    gnome-sudoku
    aisleriot
    hitori
    gnome-initial-setup
    gnome-photos
    eog
    tilix
    gnome-contacts
    gnome-terminal
    zutty
    update-manager-core
    gnome-shell-extension-ubuntu-dock
    libreoffice-*
    yaru-theme-unity
    yaru-theme-icon
    yaru-theme-gtk
    apport
    imagemagick*
    ubuntu-pro-client
    ubuntu-advantage-desktop-daemon
    ubuntu-advantage-tools
    ubuntu-pro-client-l10n
    popularity-contest
    ubuntu-report
    apport
    whoopsie
    snapd
    snap
    snap-store
    xterm
    # Printing
    cups
    cups-daemon
    cups-client
    cups-browsed
    printer-driver-*
    hplip
    bluez-cups
    system-config-printer
    # Accessibility
    orca
    brltty
    speech-dispatcher
    espeak
    espeak-ng-data
    gnome-accessibility-themes
    mousetweaks
    at-spi2-core
    # GNOME Remnants
    gnome-shell-common
    gnome-session-bin
    gnome-settings-daemon
    mutter-common
    # Deep Clean
    software-properties-gtk
    update-manager
    update-notifier
    avahi-daemon
    modemmanager
    bolt
    crash
    gnome-software
    gnome-software-plugin-flatpak
    gnome-software-plugin-snap
)

for pkg in "${packages[@]}"; do
    if dpkg -l "$pkg" 2>/dev/null | grep -q '^ii'; then
        print_warn "Error: package '$pkg' is installed." >&2

        apt autoremove -y --purge "$pkg" || true
        judge "Purge package $pkg" || true
    fi
done
