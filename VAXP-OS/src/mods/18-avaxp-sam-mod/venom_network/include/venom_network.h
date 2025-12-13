#ifndef VENOM_NETWORK_H
#define VENOM_NETWORK_H

#include <glib.h>
#include <gio/gio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🌐 Venom Network Daemon
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    GDBusConnection *session_conn;
    GDBusConnection *system_conn;
    GDBusProxy *nm_proxy;      // NetworkManager
    GDBusProxy *bluez_proxy;   // BlueZ
} NetworkState;

extern NetworkState network_state;
extern GMainLoop *main_loop;

#endif
