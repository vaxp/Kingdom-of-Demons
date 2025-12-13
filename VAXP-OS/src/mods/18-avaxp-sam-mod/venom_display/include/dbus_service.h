#ifndef DBUS_SERVICE_H
#define DBUS_SERVICE_H

#include <gio/gio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 D-Bus Service
// ═══════════════════════════════════════════════════════════════════════════

#define DBUS_SERVICE_NAME "org.venom.Display"
#define DBUS_OBJECT_PATH "/org/venom/Display"
#define DBUS_INTERFACE_NAME "org.venom.Display"

const gchar* dbus_get_introspection_xml(void);

void dbus_handle_method_call(GDBusConnection *connection,
                             const gchar *sender,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *method_name,
                             GVariant *parameters,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data);

void dbus_emit_signal(const gchar *signal_name, GVariant *parameters);

extern const GDBusInterfaceVTable dbus_interface_vtable;

#endif
