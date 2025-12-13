#include "bluetooth.h"
#include <stdio.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// 📱 Bluetooth - BlueZ D-Bus API
// ═══════════════════════════════════════════════════════════════════════════

#define BLUEZ_SERVICE "org.bluez"
#define BLUEZ_ADAPTER_INTERFACE "org.bluez.Adapter1"
#define BLUEZ_DEVICE_INTERFACE "org.bluez.Device1"
#define BLUEZ_BATTERY_INTERFACE "org.bluez.Battery1"
#define BLUEZ_AGENT_INTERFACE "org.bluez.Agent1"
#define BLUEZ_AGENT_MANAGER "org.bluez.AgentManager1"
#define DBUS_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#define DBUS_PROPERTIES "org.freedesktop.DBus.Properties"

#define VENOM_AGENT_PATH "/org/venom/network/agent"

static gchar *adapter_path = NULL;
static GDBusProxy *adapter_proxy = NULL;
static guint obj_manager_signal_id = 0;
static guint properties_signal_id = 0;
static guint agent_registration_id = 0;
static BluetoothAgentCallbacks *agent_callbacks = NULL;

// Callback storage
static BluetoothDeviceCallback device_change_callback = NULL;
static BluetoothConnectionCallback connection_callback = NULL;
static BluetoothPropertyCallback property_callback = NULL;

// ═══════════════════════════════════════════════════════════════════════════
// التحقق من التوفر
// ═══════════════════════════════════════════════════════════════════════════

gboolean bluetooth_is_available(void) {
    GError *error = NULL;
    GDBusProxy *proxy = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
        NULL, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", NULL, &error);
    
    if (!proxy) {
        g_clear_error(&error);
        return FALSE;
    }
    
    GVariant *result = g_dbus_proxy_call_sync(proxy, "NameHasOwner",
        g_variant_new("(s)", BLUEZ_SERVICE),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    gboolean available = FALSE;
    if (result) {
        g_variant_get(result, "(b)", &available);
        g_variant_unref(result);
    }
    
    g_object_unref(proxy);
    return available;
}

gboolean bluetooth_has_adapter(void) {
    return adapter_path != NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// Error Handling
// ═══════════════════════════════════════════════════════════════════════════

const gchar* bluetooth_error_to_string(BluetoothError error) {
    switch (error) {
        case BT_ERROR_NONE: return "Success";
        case BT_ERROR_NOT_AVAILABLE: return "BlueZ service not available";
        case BT_ERROR_NO_ADAPTER: return "No Bluetooth adapter found";
        case BT_ERROR_DEVICE_NOT_FOUND: return "Device not found";
        case BT_ERROR_ALREADY_CONNECTED: return "Already connected";
        case BT_ERROR_NOT_CONNECTED: return "Not connected";
        case BT_ERROR_AUTH_FAILED: return "Authentication failed";
        case BT_ERROR_AUTH_CANCELED: return "Authentication canceled";
        case BT_ERROR_AUTH_REJECTED: return "Authentication rejected by device";
        case BT_ERROR_AUTH_TIMEOUT: return "Authentication timeout";
        case BT_ERROR_CONNECTION_FAILED: return "Connection failed";
        default: return "Unknown error";
    }
}

static BluetoothError parse_bluez_error(GError *error) {
    if (!error) return BT_ERROR_NONE;
    
    const gchar *msg = error->message;
    if (g_strstr_len(msg, -1, "AuthenticationFailed")) return BT_ERROR_AUTH_FAILED;
    if (g_strstr_len(msg, -1, "AuthenticationCanceled")) return BT_ERROR_AUTH_CANCELED;
    if (g_strstr_len(msg, -1, "AuthenticationRejected")) return BT_ERROR_AUTH_REJECTED;
    if (g_strstr_len(msg, -1, "AuthenticationTimeout")) return BT_ERROR_AUTH_TIMEOUT;
    if (g_strstr_len(msg, -1, "ConnectionAttemptFailed")) return BT_ERROR_CONNECTION_FAILED;
    if (g_strstr_len(msg, -1, "AlreadyConnected")) return BT_ERROR_ALREADY_CONNECTED;
    if (g_strstr_len(msg, -1, "NotConnected")) return BT_ERROR_NOT_CONNECTED;
    if (g_strstr_len(msg, -1, "DoesNotExist")) return BT_ERROR_DEVICE_NOT_FOUND;
    
    return BT_ERROR_UNKNOWN;
}

void bluetooth_result_free(BluetoothResult *result) {
    // BluetoothResult is stack-allocated in most cases
    // This function is for consistency
    (void)result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Signal Handlers
// ═══════════════════════════════════════════════════════════════════════════

static void on_interfaces_added(GDBusConnection *conn, const gchar *sender,
    const gchar *object_path, const gchar *interface, const gchar *signal_name,
    GVariant *parameters, gpointer user_data) {
    (void)conn; (void)sender; (void)object_path; (void)interface; (void)signal_name; (void)user_data;
    
    const gchar *path;
    GVariant *ifaces;
    g_variant_get(parameters, "(&o@a{sa{sv}})", &path, &ifaces);
    
    // Check if it's a device
    if (g_variant_lookup(ifaces, BLUEZ_DEVICE_INTERFACE, NULL, NULL)) {
        // Get address from device
        GVariant *dev_props = g_variant_lookup_value(ifaces, BLUEZ_DEVICE_INTERFACE, NULL);
        if (dev_props) {
            GVariant *addr_var = g_variant_lookup_value(dev_props, "Address", NULL);
            if (addr_var) {
                const gchar *address = g_variant_get_string(addr_var, NULL);
                printf("📱 Device added: %s\n", address);
                
                if (device_change_callback) {
                    device_change_callback(address, TRUE);
                }
                
                // Emit D-Bus signal
                if (network_state.session_conn) {
                    g_dbus_connection_emit_signal(network_state.session_conn, NULL,
                        "/org/venom/Network/Bluetooth", "org.venom.Network.Bluetooth",
                        "DevicesChanged", NULL, NULL);
                }
                g_variant_unref(addr_var);
            }
            g_variant_unref(dev_props);
        }
    }
    g_variant_unref(ifaces);
}

static void on_interfaces_removed(GDBusConnection *conn, const gchar *sender,
    const gchar *object_path, const gchar *interface, const gchar *signal_name,
    GVariant *parameters, gpointer user_data) {
    (void)conn; (void)sender; (void)object_path; (void)interface; (void)signal_name; (void)user_data;
    
    const gchar *path;
    GVariant *ifaces_array;
    g_variant_get(parameters, "(&o@as)", &path, &ifaces_array);
    
    GVariantIter iter;
    const gchar *iface_name;
    g_variant_iter_init(&iter, ifaces_array);
    while (g_variant_iter_next(&iter, "&s", &iface_name)) {
        if (g_strcmp0(iface_name, BLUEZ_DEVICE_INTERFACE) == 0) {
            printf("📱 Device removed: %s\n", path);
            
            if (device_change_callback) {
                // Extract address from path (e.g., /org/bluez/hci0/dev_XX_XX_XX_XX_XX_XX)
                const gchar *addr_start = g_strrstr(path, "dev_");
                if (addr_start) {
                    gchar *address = g_strdup(addr_start + 4);
                    // Replace underscores with colons
                    for (gchar *p = address; *p; p++) {
                        if (*p == '_') *p = ':';
                    }
                    device_change_callback(address, FALSE);
                    g_free(address);
                }
            }
            
            if (network_state.session_conn) {
                g_dbus_connection_emit_signal(network_state.session_conn, NULL,
                    "/org/venom/Network/Bluetooth", "org.venom.Network.Bluetooth",
                    "DevicesChanged", NULL, NULL);
            }
            break;
        }
    }
    g_variant_unref(ifaces_array);
}

static void on_properties_changed(GDBusConnection *conn, const gchar *sender,
    const gchar *object_path, const gchar *interface, const gchar *signal_name,
    GVariant *parameters, gpointer user_data) {
    (void)conn; (void)sender; (void)signal_name; (void)user_data;
    
    const gchar *iface_name;
    GVariant *changed_props;
    GVariant *invalidated;
    
    g_variant_get(parameters, "(&s@a{sv}@as)", &iface_name, &changed_props, &invalidated);
    
    if (g_strcmp0(iface_name, BLUEZ_DEVICE_INTERFACE) == 0) {
        // Extract address from path
        const gchar *addr_start = g_strrstr(object_path, "dev_");
        gchar *address = NULL;
        if (addr_start) {
            address = g_strdup(addr_start + 4);
            for (gchar *p = address; *p; p++) {
                if (*p == '_') *p = ':';
            }
        }
        
        // Check for Connected property change
        GVariant *connected_var = g_variant_lookup_value(changed_props, "Connected", NULL);
        if (connected_var && address) {
            gboolean connected = g_variant_get_boolean(connected_var);
            printf("📱 Device %s: %s\n", connected ? "connected" : "disconnected", address);
            
            if (connection_callback) {
                connection_callback(address, connected);
            }
            
            if (network_state.session_conn) {
                g_dbus_connection_emit_signal(network_state.session_conn, NULL,
                    "/org/venom/Network/Bluetooth", "org.venom.Network.Bluetooth",
                    connected ? "DeviceConnected" : "DeviceDisconnected",
                    g_variant_new("(s)", address), NULL);
            }
            g_variant_unref(connected_var);
        }
        
        // Notify property callback for any change
        if (property_callback && address) {
            GVariantIter iter;
            const gchar *key;
            GVariant *value;
            g_variant_iter_init(&iter, changed_props);
            while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
                property_callback(address, key, value);
                g_variant_unref(value);
            }
        }
        
        g_free(address);
    }
    
    g_variant_unref(changed_props);
    g_variant_unref(invalidated);
}

// ═══════════════════════════════════════════════════════════════════════════
// Subscribe Functions
// ═══════════════════════════════════════════════════════════════════════════

void bluetooth_subscribe_device_changes(BluetoothDeviceCallback callback) {
    device_change_callback = callback;
}

void bluetooth_subscribe_connection_changes(BluetoothConnectionCallback callback) {
    connection_callback = callback;
}

void bluetooth_subscribe_property_changes(BluetoothPropertyCallback callback) {
    property_callback = callback;
}

// ═══════════════════════════════════════════════════════════════════════════
// Initialization
// ═══════════════════════════════════════════════════════════════════════════

gboolean bluetooth_init(void) {
    GError *error = NULL;
    
    if (!bluetooth_is_available()) {
        fprintf(stderr, "Bluetooth: BlueZ service not available\n");
        return FALSE;
    }
    
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
        
        // Subscribe to ObjectManager signals for device add/remove
        obj_manager_signal_id = g_dbus_connection_signal_subscribe(
            network_state.system_conn, BLUEZ_SERVICE, DBUS_OBJECT_MANAGER,
            NULL, "/", NULL, G_DBUS_SIGNAL_FLAGS_NONE,
            (GDBusSignalCallback)on_interfaces_added, NULL, NULL);
        
        g_dbus_connection_signal_subscribe(network_state.system_conn, BLUEZ_SERVICE,
            DBUS_OBJECT_MANAGER, "InterfacesRemoved", "/", NULL,
            G_DBUS_SIGNAL_FLAGS_NONE, on_interfaces_removed, NULL, NULL);
        
        // Subscribe to property changes on all devices
        properties_signal_id = g_dbus_connection_signal_subscribe(
            network_state.system_conn, BLUEZ_SERVICE, DBUS_PROPERTIES,
            "PropertiesChanged", NULL, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
            on_properties_changed, NULL, NULL);
    }
    
    return adapter_path != NULL;
}

void bluetooth_cleanup(void) {
    bluetooth_unregister_agent();
    
    if (obj_manager_signal_id) {
        g_dbus_connection_signal_unsubscribe(network_state.system_conn, obj_manager_signal_id);
        obj_manager_signal_id = 0;
    }
    if (properties_signal_id) {
        g_dbus_connection_signal_unsubscribe(network_state.system_conn, properties_signal_id);
        properties_signal_id = 0;
    }
    if (adapter_proxy) { g_object_unref(adapter_proxy); adapter_proxy = NULL; }
    g_free(adapter_path); adapter_path = NULL;
    
    device_change_callback = NULL;
    connection_callback = NULL;
    property_callback = NULL;
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
    
    var = g_dbus_proxy_get_cached_property(adapter_proxy, "Pairable");
    if (var) { status->pairable = g_variant_get_boolean(var); g_variant_unref(var); }
    
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
    printf("📱 Bluetooth discoverable: %s\n", discoverable ? "ON" : "OFF");
    return TRUE;
}

gboolean bluetooth_set_pairable(gboolean pairable) {
    if (!adapter_proxy) return FALSE;
    GError *error = NULL;
    
    g_dbus_connection_call_sync(network_state.system_conn, BLUEZ_SERVICE, adapter_path,
        DBUS_PROPERTIES, "Set",
        g_variant_new("(ssv)", BLUEZ_ADAPTER_INTERFACE, "Pairable", g_variant_new_boolean(pairable)),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📱 Bluetooth pairable: %s\n", pairable ? "ON" : "OFF");
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
    
    if (error) { 
        fprintf(stderr, "Bluetooth scan error: %s\n", error->message);
        g_error_free(error); 
        return FALSE; 
    }
    printf("📱 Scanning for devices...\n");
    return TRUE;
}

gboolean bluetooth_stop_scan(void) {
    if (!adapter_proxy) return FALSE;
    GError *error = NULL;
    
    g_dbus_proxy_call_sync(adapter_proxy, "StopDiscovery", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📱 Scan stopped\n");
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
// Helper: Find device path
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
                    if (g_ascii_strcasecmp(g_variant_get_string(addr_var, NULL), address) == 0) {
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

// ═══════════════════════════════════════════════════════════════════════════
// الإقران والاتصال
// ═══════════════════════════════════════════════════════════════════════════

BluetoothResult bluetooth_pair(const gchar *address) {
    BluetoothResult result = {BT_ERROR_NONE, NULL};
    
    gchar *path = find_device_path(address);
    if (!path) {
        result.code = BT_ERROR_DEVICE_NOT_FOUND;
        result.message = g_strdup(bluetooth_error_to_string(result.code));
        return result;
    }
    
    GError *error = NULL;
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        g_dbus_proxy_call_sync(dev, "Pair", NULL, G_DBUS_CALL_FLAGS_NONE, 60000, NULL, &error);
        g_object_unref(dev);
    }
    g_free(path);
    
    if (error) {
        result.code = parse_bluez_error(error);
        result.message = g_strdup(error->message);
        fprintf(stderr, "Pair error: %s\n", error->message);
        g_error_free(error);
    } else {
        printf("📱 Paired with %s\n", address);
    }
    
    return result;
}

BluetoothResult bluetooth_connect(const gchar *address) {
    BluetoothResult result = {BT_ERROR_NONE, NULL};
    
    gchar *path = find_device_path(address);
    if (!path) {
        result.code = BT_ERROR_DEVICE_NOT_FOUND;
        result.message = g_strdup(bluetooth_error_to_string(result.code));
        return result;
    }
    
    GError *error = NULL;
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        g_dbus_proxy_call_sync(dev, "Connect", NULL, G_DBUS_CALL_FLAGS_NONE, 30000, NULL, &error);
        g_object_unref(dev);
    }
    g_free(path);
    
    if (error) {
        result.code = parse_bluez_error(error);
        result.message = g_strdup(error->message);
        fprintf(stderr, "Connect error: %s\n", error->message);
        g_error_free(error);
    } else {
        printf("📱 Connected to %s\n", address);
    }
    
    return result;
}

BluetoothResult bluetooth_disconnect(const gchar *address) {
    BluetoothResult result = {BT_ERROR_NONE, NULL};
    
    gchar *path = find_device_path(address);
    if (!path) {
        result.code = BT_ERROR_DEVICE_NOT_FOUND;
        result.message = g_strdup(bluetooth_error_to_string(result.code));
        return result;
    }
    
    GError *error = NULL;
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        g_dbus_proxy_call_sync(dev, "Disconnect", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        g_object_unref(dev);
    }
    g_free(path);
    
    if (error) {
        result.code = parse_bluez_error(error);
        result.message = g_strdup(error->message);
        g_error_free(error);
    } else {
        printf("📱 Disconnected from %s\n", address);
    }
    
    return result;
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
    printf("📱 %s trust: %s\n", address, trusted ? "ON" : "OFF");
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔋 Battery Level
// ═══════════════════════════════════════════════════════════════════════════

BluetoothBattery* bluetooth_get_battery(const gchar *address) {
    BluetoothBattery *battery = g_new0(BluetoothBattery, 1);
    battery->percentage = -1;
    battery->available = FALSE;
    
    gchar *path = find_device_path(address);
    if (!path) return battery;
    
    GDBusProxy *proxy = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_BATTERY_INTERFACE, NULL, NULL);
    
    if (proxy) {
        GVariant *var = g_dbus_proxy_get_cached_property(proxy, "Percentage");
        if (var) {
            battery->percentage = g_variant_get_byte(var);
            battery->available = TRUE;
            g_variant_unref(var);
        }
        g_object_unref(proxy);
    }
    
    g_free(path);
    return battery;
}

void bluetooth_battery_free(BluetoothBattery *battery) {
    g_free(battery);
}

// ═══════════════════════════════════════════════════════════════════════════
// 🎵 Profiles
// ═══════════════════════════════════════════════════════════════════════════

BluetoothProfiles* bluetooth_get_profiles(const gchar *address) {
    BluetoothProfiles *profiles = g_new0(BluetoothProfiles, 1);
    
    gchar *path = find_device_path(address);
    if (!path) return profiles;
    
    GDBusProxy *proxy = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, path,
        BLUEZ_DEVICE_INTERFACE, NULL, NULL);
    
    if (proxy) {
        GVariant *uuids_var = g_dbus_proxy_get_cached_property(proxy, "UUIDs");
        if (uuids_var) {
            GVariantIter iter;
            const gchar *uuid;
            g_variant_iter_init(&iter, uuids_var);
            
            while (g_variant_iter_next(&iter, "&s", &uuid)) {
                // A2DP Sink (Audio Sink - speakers/headphones)
                if (g_str_has_prefix(uuid, "0000110b")) profiles->a2dp_sink = TRUE;
                // A2DP Source (Audio Source)
                if (g_str_has_prefix(uuid, "0000110a")) profiles->a2dp_source = TRUE;
                // HFP Handsfree
                if (g_str_has_prefix(uuid, "0000111e")) profiles->hfp_hf = TRUE;
                // HFP Audio Gateway
                if (g_str_has_prefix(uuid, "0000111f")) profiles->hfp_ag = TRUE;
                // HID (Human Interface Device)
                if (g_str_has_prefix(uuid, "00001124")) profiles->hid = TRUE;
            }
            g_variant_unref(uuids_var);
        }
        g_object_unref(proxy);
    }
    
    g_free(path);
    return profiles;
}

void bluetooth_profiles_free(BluetoothProfiles *profiles) {
    g_free(profiles);
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔐 Pairing Agent
// ═══════════════════════════════════════════════════════════════════════════

static void agent_method_call(GDBusConnection *conn, const gchar *sender,
    const gchar *object_path, const gchar *interface_name, const gchar *method_name,
    GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data) {
    (void)conn; (void)sender; (void)object_path; (void)interface_name; (void)user_data;
    
    printf("📱 Agent method: %s\n", method_name);
    
    if (g_strcmp0(method_name, "Release") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "RequestConfirmation") == 0) {
        const gchar *device_path;
        guint32 passkey;
        g_variant_get(parameters, "(&ou)", &device_path, &passkey);
        
        gboolean confirmed = TRUE;
        if (agent_callbacks && agent_callbacks->confirm_passkey) {
            confirmed = agent_callbacks->confirm_passkey(device_path, passkey);
        }
        
        if (confirmed) {
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else {
            g_dbus_method_invocation_return_dbus_error(invocation,
                "org.bluez.Error.Rejected", "Passkey rejected by user");
        }
    }
    else if (g_strcmp0(method_name, "DisplayPasskey") == 0) {
        const gchar *device_path;
        guint32 passkey;
        guint16 entered;
        g_variant_get(parameters, "(&ouq)", &device_path, &passkey, &entered);
        
        if (agent_callbacks && agent_callbacks->display_passkey) {
            agent_callbacks->display_passkey(device_path, passkey, entered);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "RequestPinCode") == 0) {
        const gchar *device_path;
        g_variant_get(parameters, "(&o)", &device_path);
        
        gchar *pin = NULL;
        if (agent_callbacks && agent_callbacks->request_pincode) {
            pin = agent_callbacks->request_pincode(device_path);
        }
        
        if (pin) {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", pin));
            g_free(pin);
        } else {
            g_dbus_method_invocation_return_dbus_error(invocation,
                "org.bluez.Error.Canceled", "PIN entry canceled");
        }
    }
    else if (g_strcmp0(method_name, "Cancel") == 0) {
        if (agent_callbacks && agent_callbacks->pairing_complete) {
            agent_callbacks->pairing_complete(NULL, FALSE);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else {
        g_dbus_method_invocation_return_dbus_error(invocation,
            "org.bluez.Error.Rejected", "Method not implemented");
    }
}

static const gchar *agent_introspection_xml =
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' "
    "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    "  <interface name='org.bluez.Agent1'>"
    "    <method name='Release'/>"
    "    <method name='RequestPinCode'>"
    "      <arg type='o' direction='in'/>"
    "      <arg type='s' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPinCode'>"
    "      <arg type='o' direction='in'/>"
    "      <arg type='s' direction='in'/>"
    "    </method>"
    "    <method name='RequestPasskey'>"
    "      <arg type='o' direction='in'/>"
    "      <arg type='u' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPasskey'>"
    "      <arg type='o' direction='in'/>"
    "      <arg type='u' direction='in'/>"
    "      <arg type='q' direction='in'/>"
    "    </method>"
    "    <method name='RequestConfirmation'>"
    "      <arg type='o' direction='in'/>"
    "      <arg type='u' direction='in'/>"
    "    </method>"
    "    <method name='RequestAuthorization'>"
    "      <arg type='o' direction='in'/>"
    "    </method>"
    "    <method name='AuthorizeService'>"
    "      <arg type='o' direction='in'/>"
    "      <arg type='s' direction='in'/>"
    "    </method>"
    "    <method name='Cancel'/>"
    "  </interface>"
    "</node>";

static const GDBusInterfaceVTable agent_vtable = {
    agent_method_call,
    NULL,
    NULL
};

gboolean bluetooth_register_agent(BluetoothAgentCallbacks *callbacks) {
    if (!network_state.system_conn) return FALSE;
    
    agent_callbacks = callbacks;
    
    GError *error = NULL;
    GDBusNodeInfo *node_info = g_dbus_node_info_new_for_xml(agent_introspection_xml, &error);
    if (!node_info) {
        fprintf(stderr, "Agent XML error: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    agent_registration_id = g_dbus_connection_register_object(
        network_state.system_conn, VENOM_AGENT_PATH,
        node_info->interfaces[0], &agent_vtable, NULL, NULL, &error);
    
    g_dbus_node_info_unref(node_info);
    
    if (!agent_registration_id) {
        fprintf(stderr, "Agent register error: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    // Register with BlueZ
    GDBusProxy *agent_manager = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, "/org/bluez",
        BLUEZ_AGENT_MANAGER, NULL, NULL);
    
    if (agent_manager) {
        g_dbus_proxy_call_sync(agent_manager, "RegisterAgent",
            g_variant_new("(os)", VENOM_AGENT_PATH, "KeyboardDisplay"),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        
        if (error) {
            fprintf(stderr, "RegisterAgent error: %s\n", error->message);
            g_error_free(error);
            error = NULL;
        } else {
            printf("📱 Pairing agent registered\n");
            
            // Request to be default agent
            g_dbus_proxy_call_sync(agent_manager, "RequestDefaultAgent",
                g_variant_new("(o)", VENOM_AGENT_PATH),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
        }
        
        g_object_unref(agent_manager);
    }
    
    return agent_registration_id != 0;
}

void bluetooth_unregister_agent(void) {
    if (agent_registration_id && network_state.system_conn) {
        GDBusProxy *agent_manager = g_dbus_proxy_new_sync(network_state.system_conn,
            G_DBUS_PROXY_FLAGS_NONE, NULL, BLUEZ_SERVICE, "/org/bluez",
            BLUEZ_AGENT_MANAGER, NULL, NULL);
        
        if (agent_manager) {
            g_dbus_proxy_call_sync(agent_manager, "UnregisterAgent",
                g_variant_new("(o)", VENOM_AGENT_PATH),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
            g_object_unref(agent_manager);
        }
        
        g_dbus_connection_unregister_object(network_state.system_conn, agent_registration_id);
        agent_registration_id = 0;
        printf("📱 Pairing agent unregistered\n");
    }
    agent_callbacks = NULL;
}
