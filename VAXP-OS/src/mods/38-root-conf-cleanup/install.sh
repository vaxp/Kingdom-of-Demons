set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error

print_ok "Cleaning up /root/.config/ and root's gnome-shell extensions"
/usr/bin/pipx uninstall gnome-extensions-cli || true
[ -f /root/.config/mimeapps.list ] && rm /root/.config/mimeapps.list || true
[ -d /root/.config/dconf ] && rm /root/.config/dconf -rf || true
[ -d /root/.local/share/gnome-shell/extensions ] && rm /root/.local/share/gnome-shell/extensions -rf || true
/usr/bin/pipx uninstall-all || true
PIPX_HOME=$(pipx environment --value PIPX_HOME) || true
[ -n "${PIPX_HOME:-}" ] && [ -d "$PIPX_HOME" ] && rm "$PIPX_HOME" -rf || true
[ -d /root/.cache ] && rm /root/.cache -rf || true
judge "Clean up /root/.config/ and root's gnome-shell extensions"