#include "ethernet.h"
#include <stdio.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 Ethernet - NetworkManager D-Bus API
// ═══════════════════════════════════════════════════════════════════════════

#define NM_SERVICE "org.freedesktop.NetworkManager"
#define NM_PATH "/org/freedesktop/NetworkManager"
#define NM_INTERFACE "org.freedesktop.NetworkManager"
#define NM_DEVICE_INTERFACE "org.freedesktop.NetworkManager.Device"
#define NM_WIRED_INTERFACE "org.freedesktop.NetworkManager.Device.Wired"

static GDBusProxy *nm_proxy = NULL;
static GList *eth_devices = NULL;

typedef struct {
    gchar *path;
    gchar *name;
} EthDevice;

gboolean ethernet_init(void) {
    GError *error = NULL;
    
    nm_proxy = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, NM_PATH, NM_INTERFACE, NULL, &error);
    
    if (!nm_proxy) {
        g_clear_error(&error);
        return FALSE;
    }
    
    // البحث عن أجهزة Ethernet
    GVariant *result = g_dbus_proxy_call_sync(nm_proxy, "GetDevices",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    if (result) {
        GVariantIter *iter;
        const gchar *path;
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "&o", &path)) {
            GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
                NM_DEVICE_INTERFACE, NULL, NULL);
            
            if (dev) {
                GVariant *type_var = g_dbus_proxy_get_cached_property(dev, "DeviceType");
                if (type_var) {
                    guint32 type = g_variant_get_uint32(type_var);
                    if (type == 1) { // NM_DEVICE_TYPE_ETHERNET
                        EthDevice *ed = g_new0(EthDevice, 1);
                        ed->path = g_strdup(path);
                        
                        GVariant *iface = g_dbus_proxy_get_cached_property(dev, "Interface");
                        if (iface) {
                            ed->name = g_variant_dup_string(iface, NULL);
                            g_variant_unref(iface);
                        }
                        
                        eth_devices = g_list_append(eth_devices, ed);
                        printf("🔌 Ethernet device found: %s\n", ed->name ? ed->name : path);
                    }
                    g_variant_unref(type_var);
                }
                g_object_unref(dev);
            }
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    return eth_devices != NULL;
}

void ethernet_cleanup(void) {
    for (GList *l = eth_devices; l; l = l->next) {
        EthDevice *ed = l->data;
        g_free(ed->path);
        g_free(ed->name);
        g_free(ed);
    }
    g_list_free(eth_devices);
    eth_devices = NULL;
    
    if (nm_proxy) { g_object_unref(nm_proxy); nm_proxy = NULL; }
}

void ethernet_interface_free(EthernetInterface *iface) {
    if (iface) {
        g_free(iface->name);
        g_free(iface->mac_address);
        g_free(iface->ip_address);
        g_free(iface->gateway);
        g_free(iface);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// الحصول على الواجهات
// ═══════════════════════════════════════════════════════════════════════════

static const gchar* find_eth_path(const gchar *name) {
    for (GList *l = eth_devices; l; l = l->next) {
        EthDevice *ed = l->data;
        if (g_strcmp0(ed->name, name) == 0) return ed->path;
    }
    return NULL;
}

GList* ethernet_get_interfaces(void) {
    GList *interfaces = NULL;
    
    for (GList *l = eth_devices; l; l = l->next) {
        EthDevice *ed = l->data;
        EthernetInterface *iface = ethernet_get_status(ed->name);
        if (iface) interfaces = g_list_append(interfaces, iface);
    }
    
    return interfaces;
}

EthernetInterface* ethernet_get_status(const gchar *name) {
    const gchar *path = find_eth_path(name);
    if (!path) return NULL;
    
    EthernetInterface *iface = g_new0(EthernetInterface, 1);
    iface->name = g_strdup(name);
    
    // Device proxy
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
        NM_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        // State
        GVariant *state_var = g_dbus_proxy_get_cached_property(dev, "State");
        if (state_var) {
            guint32 state = g_variant_get_uint32(state_var);
            iface->connected = (state == 100); // ACTIVATED
            iface->enabled = (state >= 30); // DISCONNECTED or higher
            g_variant_unref(state_var);
        }
        
        // IP Address
        if (iface->connected) {
            GVariant *ip4_path = g_dbus_proxy_get_cached_property(dev, "Ip4Config");
            if (ip4_path) {
                const gchar *ip_path = g_variant_get_string(ip4_path, NULL);
                if (ip_path && strlen(ip_path) > 1) {
                    GDBusProxy *ip4 = g_dbus_proxy_new_sync(network_state.system_conn,
                        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, ip_path,
                        "org.freedesktop.NetworkManager.IP4Config", NULL, NULL);
                    
                    if (ip4) {
                        GVariant *addrs = g_dbus_proxy_get_cached_property(ip4, "AddressData");
                        if (addrs) {
                            GVariantIter iter;
                            GVariant *dict;
                            g_variant_iter_init(&iter, addrs);
                            if (g_variant_iter_next(&iter, "@a{sv}", &dict)) {
                                GVariant *addr = g_variant_lookup_value(dict, "address", NULL);
                                if (addr) {
                                    iface->ip_address = g_variant_dup_string(addr, NULL);
                                    g_variant_unref(addr);
                                }
                                g_variant_unref(dict);
                            }
                            g_variant_unref(addrs);
                        }
                        
                        GVariant *gw = g_dbus_proxy_get_cached_property(ip4, "Gateway");
                        if (gw) {
                            iface->gateway = g_variant_dup_string(gw, NULL);
                            g_variant_unref(gw);
                        }
                        
                        g_object_unref(ip4);
                    }
                }
                g_variant_unref(ip4_path);
            }
        }
        
        g_object_unref(dev);
    }
    
    // Wired interface for MAC and speed
    GDBusProxy *wired = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
        NM_WIRED_INTERFACE, NULL, NULL);
    
    if (wired) {
        GVariant *mac = g_dbus_proxy_get_cached_property(wired, "HwAddress");
        if (mac) {
            iface->mac_address = g_variant_dup_string(mac, NULL);
            g_variant_unref(mac);
        }
        
        GVariant *speed = g_dbus_proxy_get_cached_property(wired, "Speed");
        if (speed) {
            iface->speed = g_variant_get_uint32(speed);
            g_variant_unref(speed);
        }
        
        g_object_unref(wired);
    }
    
    return iface;
}

// ═══════════════════════════════════════════════════════════════════════════
// تشغيل / إيقاف
// ═══════════════════════════════════════════════════════════════════════════

gboolean ethernet_enable(const gchar *name) {
    const gchar *path = find_eth_path(name);
    if (!path || !nm_proxy) return FALSE;
    
    // الحصول على اتصال متاح للجهاز
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
        NM_DEVICE_INTERFACE, NULL, NULL);
    
    if (!dev) return FALSE;
    
    GVariant *conns = g_dbus_proxy_get_cached_property(dev, "AvailableConnections");
    gboolean success = FALSE;
    
    if (conns) {
        GVariantIter iter;
        const gchar *conn_path;
        g_variant_iter_init(&iter, conns);
        
        if (g_variant_iter_next(&iter, "&o", &conn_path)) {
            GError *error = NULL;
            g_dbus_proxy_call_sync(nm_proxy, "ActivateConnection",
                g_variant_new("(ooo)", conn_path, path, "/"),
                G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
            
            if (!error) {
                printf("🔌 %s enabled\n", name);
                success = TRUE;
            } else {
                g_error_free(error);
            }
        }
        g_variant_unref(conns);
    }
    
    g_object_unref(dev);
    return success;
}

gboolean ethernet_disable(const gchar *name) {
    const gchar *path = find_eth_path(name);
    if (!path) return FALSE;
    
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
        NM_DEVICE_INTERFACE, NULL, NULL);
    
    if (!dev) return FALSE;
    
    GError *error = NULL;
    g_dbus_proxy_call_sync(dev, "Disconnect", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    g_object_unref(dev);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("🔌 %s disabled\n", name);
    return TRUE;
}
