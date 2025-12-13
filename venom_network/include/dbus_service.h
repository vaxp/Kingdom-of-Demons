#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <gio/gio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 D-Bus Service
// ═══════════════════════════════════════════════════════════════════════════

#define DBUS_SERVICE_NAME "org.venom.Network"
#define DBUS_OBJECT_PATH_WIFI "/org/venom/Network/WiFi"
#define DBUS_OBJECT_PATH_BLUETOOTH "/org/venom/Network/Bluetooth"
#define DBUS_OBJECT_PATH_ETHERNET "/org/venom/Network/Ethernet"

const gchar* dbus_get_wifi_xml(void);
const gchar* dbus_get_bluetooth_xml(void);
const gchar* dbus_get_ethernet_xml(void);

void dbus_handle_wifi_method(GDBusConnection *conn, const gchar *sender,
    const gchar *path, const gchar *interface, const gchar *method,
    GVariant *params, GDBusMethodInvocation *invocation, gpointer data);

void dbus_handle_bluetooth_method(GDBusConnection *conn, const gchar *sender,
    const gchar *path, const gchar *interface, const gchar *method,
    GVariant *params, GDBusMethodInvocation *invocation, gpointer data);

void dbus_handle_ethernet_method(GDBusConnection *conn, const gchar *sender,
    const gchar *path, const gchar *interface, const gchar *method,
    GVariant *params, GDBusMethodInvocation *invocation, gpointer data);

void dbus_emit_signal(const gchar *path, const gchar *interface, 
                      const gchar *signal_name, GVariant *params);

extern const GDBusInterfaceVTable wifi_vtable;
extern const GDBusInterfaceVTable bluetooth_vtable;
extern const GDBusInterfaceVTable ethernet_vtable;

#endif
