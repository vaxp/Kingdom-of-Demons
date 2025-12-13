#include "bluetooth.h"
#include <stdio.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// 📱 Bluetooth - BlueZ D-Bus API
// ═══════════════════════════════════════════════════════════════════════════

#define BLUEZ_SERVICE "org.bluez"
#define BLUEZ_ADAPTER_INTERFACE "org.bluez.Adapter1"
#define BLUEZ_DEVICE_INTERFACE "org.bluez.Device1"
#define DBUS_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#define DBUS_PROPERTIES "org.freedesktop.DBus.Properties"

static gchar *adapter_path = NULL;
static GDBusProxy *adapter_proxy = NULL;

gboolean bluetooth_init(void) {
    GError *error = NULL;
    
    // البحث عن adapter
    GDBusProxy *manager = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, "/",
        DBUS_OBJECT_MANAGER, NULL, &error);
    
    if (!manager) {
        fprintf(stderr, "Bluetooth: Cannot connect to BlueZ: %s\n", 
                error ? error->message : "unknown");
        g_clear_error(&error);
        return FALSE;
    }
    
    GVariant *result = g_dbus_proxy_call_sync(manager, "GetManagedObjects",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    if (result) {
        GVariantIter *iter;
        const gchar *path;
        GVariant *ifaces;
        
        g_variant_get(result, "(a{oa{sa{sv}}})", &iter);
        while (g_variant_iter_next(iter, "{&o@a{sa{sv}}}", &path, &ifaces)) {
            if (g_variant_lookup(ifaces, BLUEZ_ADAPTER_INTERFACE, NULL, NULL)) {
                adapter_path = g_strdup(path);
                printf("📱 Bluetooth adapter found: %s\n", path);
            }
            g_variant_unref(ifaces);
            if (adapter_path) break;
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    g_object_unref(manager);
    
    if (adapter_path) {
        adapter_proxy = g_dbus_proxy_new_sync(network_state.system_conn,
            G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, adapter_path,
            BLUEZ_ADAPTER_INTERFACE, NULL, NULL);
    }
    
    return adapter_path != NULL;
}

void bluetooth_cleanup(void) {
    if (adapter_proxy) { g_object_unref(adapter_proxy); adapter_proxy = NULL; }
    g_free(adapter_path); adapter_path = NULL;
}

void bluetooth_device_free(BluetoothDevice *d) {
    if (d) { g_free(d->address); g_free(d->name); g_free(d->icon); g_free(d); }
}

void bluetooth_status_free(BluetoothStatus *s) {
    if (s) { g_free(s->name); g_free(s->address); g_free(s); }
}

// ═══════════════════════════════════════════════════════════════════════════
// حالة Bluetooth
// ═══════════════════════════════════════════════════════════════════════════

BluetoothStatus* bluetooth_get_status(void) {
    BluetoothStatus *status = g_new0(BluetoothStatus, 1);
    
    if (!adapter_proxy) return status;
    
    GVariant *var = g_dbus_proxy_get_cached_property(adapter_proxy, "Powered");
    if (var) { status->powered = g_variant_get_boolean(var); g_variant_unref(var); }
    
    var = g_dbus_proxy_get_cached_property(adapter_proxy, "Discovering");
    if (var) { status->discovering = g_variant_get_boolean(var); g_variant_unref(var); }
    
    var = g_dbus_proxy_get_cached_property(adapter_proxy, "Name");
    if (var) { status->name = g_variant_dup_string(var, NULL); g_variant_unref(var); }
    
    var = g_dbus_proxy_get_cached_property(adapter_proxy, "Address");
    if (var) { status->address = g_variant_dup_string(var, NULL); g_variant_unref(var); }
    
    return status;
}

gboolean bluetooth_set_powered(gboolean powered) {
    if (!adapter_proxy) return FALSE;
    GError *error = NULL;
    
    g_dbus_connection_call_sync(network_state.system_conn, BLUEZ_SERVICE, adapter_path,
        DBUS_PROPERTIES, "Set",
        g_variant_new("(ssv)", BLUEZ_ADAPTER_INTERFACE, "Powered", g_variant_new_boolean(powered)),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (error) { fprintf(stderr, "Bluetooth power error: %s\n", error->message); g_error_free(error); return FALSE; }
    printf("📱 Bluetooth %s\n", powered ? "ON" : "OFF");
    return TRUE;
}

gboolean bluetooth_set_discoverable(gboolean discoverable) {
    if (!adapter_proxy) return FALSE;
    GError *error = NULL;
    
    g_dbus_connection_call_sync(network_state.system_conn, BLUEZ_SERVICE, adapter_path,
        DBUS_PROPERTIES, "Set",
        g_variant_new("(ssv)", BLUEZ_ADAPTER_INTERFACE, "Discoverable", g_variant_new_boolean(discoverable)),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (error) { g_error_free(error); return FALSE; }
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// البحث عن أجهزة
// ═══════════════════════════════════════════════════════════════════════════

gboolean bluetooth_start_scan(void) {
    if (!adapter_proxy) return FALSE;
    GError *error = NULL;
    
    g_dbus_proxy_call_sync(adapter_proxy, "StartDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📱 Scanning for devices...\n");
    return TRUE;
}

gboolean bluetooth_stop_scan(void) {
    if (!adapter_proxy) return FALSE;
    GError *error = NULL;
    
    g_dbus_proxy_call_sync(adapter_proxy, "StopDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (error) { g_error_free(error); return FALSE; }
    return TRUE;
}

GList* bluetooth_get_devices(void) {
    GList *devices = NULL;
    
    GDBusProxy *manager = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, "/",
        DBUS_OBJECT_MANAGER, NULL, NULL);
    
    if (!manager) return devices;
    
    GVariant *result = g_dbus_proxy_call_sync(manager, "GetManagedObjects",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    if (result) {
        GVariantIter *iter;
        const gchar *path;
        GVariant *ifaces;
        
        g_variant_get(result, "(a{oa{sa{sv}}})", &iter);
        while (g_variant_iter_next(iter, "{&o@a{sa{sv}}}", &path, &ifaces)) {
            GVariant *device_props = g_variant_lookup_value(ifaces, BLUEZ_DEVICE_INTERFACE, NULL);
            if (device_props) {
                BluetoothDevice *dev = g_new0(BluetoothDevice, 1);
                
                GVariant *var = g_variant_lookup_value(device_props, "Address", NULL);
                if (var) { dev->address = g_variant_dup_string(var, NULL); g_variant_unref(var); }
                
                var = g_variant_lookup_value(device_props, "Name", NULL);
                if (var) { dev->name = g_variant_dup_string(var, NULL); g_variant_unref(var); }
                else dev->name = g_strdup(dev->address ? dev->address : "Unknown");
                
                var = g_variant_lookup_value(device_props, "Icon", NULL);
                if (var) { dev->icon = g_variant_dup_string(var, NULL); g_variant_unref(var); }
                
                var = g_variant_lookup_value(device_props, "Paired", NULL);
                if (var) { dev->paired = g_variant_get_boolean(var); g_variant_unref(var); }
                
                var = g_variant_lookup_value(device_props, "Connected", NULL);
                if (var) { dev->connected = g_variant_get_boolean(var); g_variant_unref(var); }
                
                var = g_variant_lookup_value(device_props, "Trusted", NULL);
                if (var) { dev->trusted = g_variant_get_boolean(var); g_variant_unref(var); }
                
                var = g_variant_lookup_value(device_props, "RSSI", NULL);
                if (var) { dev->rssi = g_variant_get_int16(var); g_variant_unref(var); }
                
                devices = g_list_append(devices, dev);
                g_variant_unref(device_props);
            }
            g_variant_unref(ifaces);
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    g_object_unref(manager);
    return devices;
}

// ═══════════════════════════════════════════════════════════════════════════
// الإقران والاتصال
// ═══════════════════════════════════════════════════════════════════════════

static gchar* find_device_path(const gchar *address) {
    gchar *device_path = NULL;
    
    GDBusProxy *manager = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, "/",
        DBUS_OBJECT_MANAGER, NULL, NULL);
    
    if (!manager) return NULL;
    
    GVariant *result = g_dbus_proxy_call_sync(manager, "GetManagedObjects",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    if (result) {
        GVariantIter *iter;
        const gchar *path;
        GVariant *ifaces;
        
        g_variant_get(result, "(a{oa{sa{sv}}})", &iter);
        while (g_variant_iter_next(iter, "{&o@a{sa{sv}}}", &path, &ifaces)) {
            GVariant *dev = g_variant_lookup_value(ifaces, BLUEZ_DEVICE_INTERFACE, NULL);
            if (dev) {
                GVariant *addr_var = g_variant_lookup_value(dev, "Address", NULL);
                if (addr_var) {
                    if (g_strcmp0(g_variant_get_string(addr_var, NULL), address) == 0) {
                        device_path = g_strdup(path);
                    }
                    g_variant_unref(addr_var);
                }
                g_variant_unref(dev);
            }
            g_variant_unref(ifaces);
            if (device_path) break;
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    g_object_unref(manager);
    return device_path;
}

gboolean bluetooth_pair(const gchar *address) {
    gchar *path = find_device_path(address);
    if (!path) return FALSE;
    
    GError *error = NULL;
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        g_dbus_proxy_call_sync(dev, "Pair", NULL, G_DBUS_CALL_FLAGS_NONE, 30000, NULL, &error);
        g_object_unref(dev);
    }
    g_free(path);
    
    if (error) { fprintf(stderr, "Pair error: %s\n", error->message); g_error_free(error); return FALSE; }
    printf("📱 Paired with %s\n", address);
    return TRUE;
}

gboolean bluetooth_connect(const gchar *address) {
    gchar *path = find_device_path(address);
    if (!path) return FALSE;
    
    GError *error = NULL;
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        g_dbus_proxy_call_sync(dev, "Connect", NULL, G_DBUS_CALL_FLAGS_NONE, 30000, NULL, &error);
        g_object_unref(dev);
    }
    g_free(path);
    
    if (error) { fprintf(stderr, "Connect error: %s\n", error->message); g_error_free(error); return FALSE; }
    printf("📱 Connected to %s\n", address);
    return TRUE;
}

gboolean bluetooth_disconnect(const gchar *address) {
    gchar *path = find_device_path(address);
    if (!path) return FALSE;
    
    GError *error = NULL;
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        g_dbus_proxy_call_sync(dev, "Disconnect", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        g_object_unref(dev);
    }
    g_free(path);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📱 Disconnected from %s\n", address);
    return TRUE;
}

gboolean bluetooth_remove(const gchar *address) {
    gchar *path = find_device_path(address);
    if (!path) return FALSE;
    
    GError *error = NULL;
    if (adapter_proxy) {
        g_dbus_proxy_call_sync(adapter_proxy, "RemoveDevice",
            g_variant_new("(o)", path), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    }
    g_free(path);
    
    if (error) { fprintf(stderr, "Remove error: %s\n", error->message); g_error_free(error); return FALSE; }
    printf("📱 Removed %s\n", address);
    return TRUE;
}

gboolean bluetooth_trust(const gchar *address, gboolean trusted) {
    gchar *path = find_device_path(address);
    if (!path) return FALSE;
    
    GError *error = NULL;
    g_dbus_connection_call_sync(network_state.system_conn, BLUEZ_SERVICE, path,
        DBUS_PROPERTIES, "Set",
        g_variant_new("(ssv)", BLUEZ_DEVICE_INTERFACE, "Trusted", g_variant_new_boolean(trusted)),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    g_free(path);
    if (error) { g_error_free(error); return FALSE; }
    return TRUE;
}
