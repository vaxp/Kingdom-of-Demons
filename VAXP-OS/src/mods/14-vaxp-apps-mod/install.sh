set -e                  # exit on error
set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error

print_ok "Installing venom-shell and other venom applications"
wait_network

print_ok "Installing basic CLI tools..."
apt install $INTERACTIVE \
    apt-transport-https \
    cifs-utils \
    cloud-init \
    coreutils \
    gnupg \
    gpg \
    gvfs-fuse \
    gvfs-backends \
    wsdd \
    libsass1 \
    lsb-release \
    systemd-timesyncd \
    gdb \
    sassc \
    software-properties-common \
    mesa-vulkan-drivers \
    squashfs-tools \
    sysstat \
    wget \
    whiptail \
    gdisk \
    eatmydata \
    patch \
    less \
    gnupg-l10n \
    gpg-wks-client \
    upower \
    mdadm \
    build-essential \
    pkg-config \
    cmake \
    libgtk-3-dev \
    libgtk-layer-shell-dev \
    libc6 \
    libgcc-s1 \
    libstdc++6 \
    libx11-6 \
    libxcomposite1 \
    libxdamage1 \
    libxext6 \
    libglib2.0-dev \
    libpolkit-agent-1-dev \
    libjson-glib-dev \
    libpulse-dev \
    libxkbregistry-dev \
    libdbusmenu-glib-dev \
    libgtk-3-dev \
    libgtk-layer-shell-dev \
    appstream \
    packagekit-tools \
    python3-babel \
    exfatprogs \
    iw \
    xxd \
    xdg-utils \
    zenity \
    power-profiles-daemon \
    --no-install-recommends
judge "Install basic CLI tools"


print_ok "Installing LightDM and Xorg..."
apt install $INTERACTIVE \
    lightdm \
    lightdm-gtk-greeter \
    lightdm-gtk-greeter-settings \
    xserver-xorg \
    xserver-xorg-core \
    xserver-xorg-input-all \
    xserver-xorg-video-all \
    --no-install-recommends
judge "Install LightDM and Xorg"

print_ok "Installing default cli applications..."
apt install $INTERACTIVE \
    wget \
    $DEFAULT_CLI_TOOLS \
    --no-install-recommends
judge "Install default cli applications"


print_ok "Installing ubuntu drivers support..."
apt install $INTERACTIVE \
    ubuntu-drivers-common \
    alsa-utils \
    alsa-base \
    alsa-topology-conf \
    alsa-ucm-conf \
    fprintd --no-install-recommends
judge "Install ubuntu drivers support"

print_ok "Installing python3..."
apt install $INTERACTIVE \
    python3 \
    python3-pip \
    python-is-python3 \
    pipx \
    --no-install-recommends
judge "Install python3"



print_ok "Remove the default htop.desktop file"
rm /usr/share/applications/htop.desktop || true
judge "Remove the default htop.desktop file"

print_ok "Remove the default vim.desktop file"
rm /usr/share/applications/vim.desktop || true
judge "Remove the default vim.desktop file"

print_ok "Installing $LANGUAGE_PACKS language packs"
apt install $INTERACTIVE $LANGUAGE_PACKS --no-install-recommends
judge "Install language packs"