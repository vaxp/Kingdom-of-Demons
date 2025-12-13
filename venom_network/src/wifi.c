#include "wifi.h"
#include <stdio.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// 📶 WiFi - NetworkManager D-Bus API
// ═══════════════════════════════════════════════════════════════════════════

#define NM_SERVICE "org.freedesktop.NetworkManager"
#define NM_PATH "/org/freedesktop/NetworkManager"
#define NM_INTERFACE "org.freedesktop.NetworkManager"
#define NM_DEVICE_INTERFACE "org.freedesktop.NetworkManager.Device"
#define NM_WIRELESS_INTERFACE "org.freedesktop.NetworkManager.Device.Wireless"
#define NM_AP_INTERFACE "org.freedesktop.NetworkManager.AccessPoint"
#define NM_SETTINGS "org.freedesktop.NetworkManager.Settings"
#define NM_CONNECTION "org.freedesktop.NetworkManager.Settings.Connection"
#define NM_ACTIVE "org.freedesktop.NetworkManager.Connection.Active"

static GDBusProxy *nm_proxy = NULL;
static gchar *wifi_device_path = NULL;

gboolean wifi_init(void) {
    GError *error = NULL;
    
    nm_proxy = g_dbus_proxy_new_sync(network_state.system_conn, G_DBUS_PROXY_FLAGS_NONE,
        NULL, NM_SERVICE, NM_PATH, NM_INTERFACE, NULL, &error);
    
    if (!nm_proxy) {
        fprintf(stderr, "WiFi: Cannot connect to NetworkManager: %s\n", 
                error ? error->message : "unknown");
        g_clear_error(&error);
        return FALSE;
    }
    
    // البحث عن جهاز WiFi
    GVariant *result = g_dbus_proxy_call_sync(nm_proxy, "GetDevices", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    if (result) {
        GVariantIter *iter;
        const gchar *device_path;
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "&o", &device_path)) {
            GDBusProxy *dev_proxy = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, device_path, 
                NM_DEVICE_INTERFACE, NULL, NULL);
            
            if (dev_proxy) {
                GVariant *type_var = g_dbus_proxy_get_cached_property(dev_proxy, "DeviceType");
                if (type_var) {
                    guint32 type = g_variant_get_uint32(type_var);
                    if (type == 2) { // NM_DEVICE_TYPE_WIFI
                        wifi_device_path = g_strdup(device_path);
                        printf("📶 WiFi device found: %s\n", device_path);
                    }
                    g_variant_unref(type_var);
                }
                g_object_unref(dev_proxy);
            }
            if (wifi_device_path) break;
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    return wifi_device_path != NULL;
}

void wifi_cleanup(void) {
    if (nm_proxy) { g_object_unref(nm_proxy); nm_proxy = NULL; }
    g_free(wifi_device_path); wifi_device_path = NULL;
}

void wifi_network_free(WiFiNetwork *n) {
    if (n) { g_free(n->ssid); g_free(n->bssid); g_free(n); }
}

void wifi_status_free(WiFiStatus *s) {
    if (s) { g_free(s->ssid); g_free(s->ip_address); g_free(s->gateway); g_free(s->subnet); g_free(s->dns); g_free(s); }
}

// ═══════════════════════════════════════════════════════════════════════════
// حالة WiFi
// ═══════════════════════════════════════════════════════════════════════════

gboolean wifi_is_enabled(void) {
    if (!nm_proxy) return FALSE;
    GVariant *result = g_dbus_proxy_get_cached_property(nm_proxy, "WirelessEnabled");
    if (!result) return FALSE;
    gboolean enabled = g_variant_get_boolean(result);
    g_variant_unref(result);
    return enabled;
}

gboolean wifi_set_enabled(gboolean enabled) {
    if (!nm_proxy) return FALSE;
    GError *error = NULL;
    g_dbus_proxy_call_sync(nm_proxy, "org.freedesktop.DBus.Properties.Set",
        g_variant_new("(ssv)", NM_INTERFACE, "WirelessEnabled", g_variant_new_boolean(enabled)),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (error) { g_error_free(error); return FALSE; }
    printf("📶 WiFi %s\n", enabled ? "enabled" : "disabled");
    return TRUE;
}

WiFiStatus* wifi_get_status(void) {
    WiFiStatus *status = g_new0(WiFiStatus, 1);
    status->connected = FALSE;
    
    if (!wifi_device_path) return status;
    
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, wifi_device_path,
        NM_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        GVariant *state_var = g_dbus_proxy_get_cached_property(dev, "State");
        if (state_var) {
            guint32 state = g_variant_get_uint32(state_var);
            status->connected = (state == 100); // NM_DEVICE_STATE_ACTIVATED
            g_variant_unref(state_var);
        }
        
        if (status->connected) {
            // الحصول على SSID الحالي
            GDBusProxy *wifi_dev = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, wifi_device_path,
                NM_WIRELESS_INTERFACE, NULL, NULL);
            
            if (wifi_dev) {
                GVariant *ap_path = g_dbus_proxy_get_cached_property(wifi_dev, "ActiveAccessPoint");
                if (ap_path) {
                    const gchar *path = g_variant_get_string(ap_path, NULL);
                    if (path && strlen(path) > 1) {
                        GDBusProxy *ap = g_dbus_proxy_new_sync(network_state.system_conn,
                            G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
                            NM_AP_INTERFACE, NULL, NULL);
                        if (ap) {
                            GVariant *ssid_var = g_dbus_proxy_get_cached_property(ap, "Ssid");
                            if (ssid_var) {
                                gsize len;
                                const guchar *data = g_variant_get_fixed_array(ssid_var, &len, 1);
                                status->ssid = g_strndup((gchar*)data, len);
                                g_variant_unref(ssid_var);
                            }
                            GVariant *str_var = g_dbus_proxy_get_cached_property(ap, "Strength");
                            if (str_var) {
                                status->strength = g_variant_get_byte(str_var);
                                g_variant_unref(str_var);
                            }
                            g_object_unref(ap);
                        }
                    }
                    g_variant_unref(ap_path);
                }
                g_object_unref(wifi_dev);
            }
            
            // IP4 Config
            GVariant *ip4_path = g_dbus_proxy_get_cached_property(dev, "Ip4Config");
            if (ip4_path) {
                const gchar *path = g_variant_get_string(ip4_path, NULL);
                if (path && strlen(path) > 1) {
                    GDBusProxy *ip4 = g_dbus_proxy_new_sync(network_state.system_conn,
                        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path,
                        "org.freedesktop.NetworkManager.IP4Config", NULL, NULL);
                    if (ip4) {
                        // Address + Subnet
                        GVariant *addrs = g_dbus_proxy_get_cached_property(ip4, "AddressData");
                        if (addrs) {
                            GVariantIter iter;
                            GVariant *addr_dict;
                            g_variant_iter_init(&iter, addrs);
                            if (g_variant_iter_next(&iter, "@a{sv}", &addr_dict)) {
                                GVariant *addr_var = g_variant_lookup_value(addr_dict, "address", G_VARIANT_TYPE_STRING);
                                if (addr_var) {
                                    status->ip_address = g_variant_dup_string(addr_var, NULL);
                                    g_variant_unref(addr_var);
                                }
                                GVariant *prefix_var = g_variant_lookup_value(addr_dict, "prefix", G_VARIANT_TYPE_UINT32);
                                if (prefix_var) {
                                    guint32 prefix = g_variant_get_uint32(prefix_var);
                                    status->subnet = g_strdup_printf("%u", prefix);
                                    g_variant_unref(prefix_var);
                                }
                                g_variant_unref(addr_dict);
                            }
                            g_variant_unref(addrs);
                        }
                        // Gateway
                        GVariant *gw_var = g_dbus_proxy_get_cached_property(ip4, "Gateway");
                        if (gw_var) {
                            status->gateway = g_variant_dup_string(gw_var, NULL);
                            g_variant_unref(gw_var);
                        }
                        // DNS
                        GVariant *dns_arr = g_dbus_proxy_get_cached_property(ip4, "Nameservers");
                        if (dns_arr) {
                            GVariantIter dns_iter;
                            guint32 dns_addr;
                            GString *dns_str = g_string_new("");
                            g_variant_iter_init(&dns_iter, dns_arr);
                            while (g_variant_iter_next(&dns_iter, "u", &dns_addr)) {
                                if (dns_str->len > 0) g_string_append(dns_str, ", ");
                                g_string_append_printf(dns_str, "%u.%u.%u.%u",
                                    dns_addr & 0xFF, (dns_addr >> 8) & 0xFF, (dns_addr >> 16) & 0xFF, (dns_addr >> 24) & 0xFF);
                            }
                            status->dns = g_string_free(dns_str, FALSE);
                            g_variant_unref(dns_arr);
                        }
                        g_object_unref(ip4);
                    }
                }
                g_variant_unref(ip4_path);
            }
        }
        g_object_unref(dev);
    }
    
    return status;
}

// ═══════════════════════════════════════════════════════════════════════════
// فحص الشبكات
// ═══════════════════════════════════════════════════════════════════════════

GList* wifi_scan(void) {
    GList *networks = NULL;
    if (!wifi_device_path) return networks;
    
    GDBusProxy *wifi_dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, wifi_device_path,
        NM_WIRELESS_INTERFACE, NULL, NULL);
    
    if (!wifi_dev) return networks;
    
    // طلب فحص جديد
    g_dbus_proxy_call_sync(wifi_dev, "RequestScan",
        g_variant_new("(a{sv})", NULL), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    // انتظار قليل
    g_usleep(1000000); // 1 ثانية
    
    // الحصول على الـ AccessPoint المتصل
    gchar *active_ap_path = NULL;
    GVariant *active_ap = g_dbus_proxy_get_cached_property(wifi_dev, "ActiveAccessPoint");
    if (active_ap) {
        const gchar *path = g_variant_get_string(active_ap, NULL);
        if (path && strlen(path) > 1) {
            active_ap_path = g_strdup(path);
        }
        g_variant_unref(active_ap);
    }
    
    // الحصول على نقاط الوصول
    GVariant *aps = g_dbus_proxy_get_cached_property(wifi_dev, "AccessPoints");
    if (aps) {
        GVariantIter iter;
        const gchar *ap_path;
        g_variant_iter_init(&iter, aps);
        
        while (g_variant_iter_next(&iter, "&o", &ap_path)) {
            GDBusProxy *ap = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, ap_path,
                NM_AP_INTERFACE, NULL, NULL);
            
            if (ap) {
                WiFiNetwork *net = g_new0(WiFiNetwork, 1);
                
                // التحقق إذا كانت هذه الشبكة المتصلة
                net->connected = (active_ap_path && g_strcmp0(ap_path, active_ap_path) == 0);
                
                // SSID
                GVariant *ssid_var = g_dbus_proxy_get_cached_property(ap, "Ssid");
                if (ssid_var) {
                    gsize len;
                    const guchar *data = g_variant_get_fixed_array(ssid_var, &len, 1);
                    if (len > 0) net->ssid = g_strndup((gchar*)data, len);
                    g_variant_unref(ssid_var);
                }
                
                // BSSID
                GVariant *hw_var = g_dbus_proxy_get_cached_property(ap, "HwAddress");
                if (hw_var) {
                    net->bssid = g_variant_dup_string(hw_var, NULL);
                    g_variant_unref(hw_var);
                }
                
                // Strength
                GVariant *str_var = g_dbus_proxy_get_cached_property(ap, "Strength");
                if (str_var) {
                    net->strength = g_variant_get_byte(str_var);
                    g_variant_unref(str_var);
                }
                
                // Frequency
                GVariant *freq_var = g_dbus_proxy_get_cached_property(ap, "Frequency");
                if (freq_var) {
                    net->frequency = g_variant_get_uint32(freq_var);
                    g_variant_unref(freq_var);
                }
                
                // Security (WpaFlags or RsnFlags != 0)
                GVariant *wpa_var = g_dbus_proxy_get_cached_property(ap, "WpaFlags");
                GVariant *rsn_var = g_dbus_proxy_get_cached_property(ap, "RsnFlags");
                if (wpa_var && rsn_var) {
                    net->secured = (g_variant_get_uint32(wpa_var) != 0) || 
                                   (g_variant_get_uint32(rsn_var) != 0);
                    g_variant_unref(wpa_var);
                    g_variant_unref(rsn_var);
                }
                
                if (net->ssid && strlen(net->ssid) > 0) {
                    // وضع الشبكة المتصلة في الأول
                    if (net->connected) {
                        networks = g_list_prepend(networks, net);
                    } else {
                        networks = g_list_append(networks, net);
                    }
                } else {
                    wifi_network_free(net);
                }
                
                g_object_unref(ap);
            }
        }
        g_variant_unref(aps);
    }
    
    g_free(active_ap_path);
    g_object_unref(wifi_dev);
    return networks;
}

// ═══════════════════════════════════════════════════════════════════════════
// الاتصال / قطع الاتصال
// ═══════════════════════════════════════════════════════════════════════════

gboolean wifi_connect(const gchar *ssid, const gchar *password) {
    if (!nm_proxy || !wifi_device_path || !ssid) return FALSE;
    
    GError *error = NULL;
    GVariantBuilder conn_builder, wireless_builder, security_builder, ipv4_builder;
    
    // بناء إعدادات الاتصال
    g_variant_builder_init(&conn_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&conn_builder, "{sv}", "type", g_variant_new_string("802-11-wireless"));
    g_variant_builder_add(&conn_builder, "{sv}", "id", g_variant_new_string(ssid));
    
    // إعدادات WiFi
    g_variant_builder_init(&wireless_builder, G_VARIANT_TYPE("a{sv}"));
    GBytes *ssid_bytes = g_bytes_new(ssid, strlen(ssid));
    g_variant_builder_add(&wireless_builder, "{sv}", "ssid", 
        g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, g_bytes_get_data(ssid_bytes, NULL), 
                                   g_bytes_get_size(ssid_bytes), 1));
    g_bytes_unref(ssid_bytes);
    
    // إعدادات الأمان
    if (password && strlen(password) > 0) {
        g_variant_builder_add(&wireless_builder, "{sv}", "security", 
            g_variant_new_string("802-11-wireless-security"));
        
        g_variant_builder_init(&security_builder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&security_builder, "{sv}", "key-mgmt", 
            g_variant_new_string("wpa-psk"));
        g_variant_builder_add(&security_builder, "{sv}", "psk", 
            g_variant_new_string(password));
    }
    
    // IPv4 auto
    g_variant_builder_init(&ipv4_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&ipv4_builder, "{sv}", "method", g_variant_new_string("auto"));
    
    // بناء التكوين الكامل
    GVariantBuilder settings_builder;
    g_variant_builder_init(&settings_builder, G_VARIANT_TYPE("a{sa{sv}}"));
    g_variant_builder_add(&settings_builder, "{sa{sv}}", "connection", &conn_builder);
    g_variant_builder_add(&settings_builder, "{sa{sv}}", "802-11-wireless", &wireless_builder);
    if (password && strlen(password) > 0) {
        g_variant_builder_add(&settings_builder, "{sa{sv}}", "802-11-wireless-security", &security_builder);
    }
    g_variant_builder_add(&settings_builder, "{sa{sv}}", "ipv4", &ipv4_builder);
    
    // الاتصال
    GVariant *result = g_dbus_proxy_call_sync(nm_proxy, "AddAndActivateConnection",
        g_variant_new("(a{sa{sv}}oo)", &settings_builder, wifi_device_path, "/"),
        G_DBUS_CALL_FLAGS_NONE, 30000, NULL, &error);
    
    if (error) {
        fprintf(stderr, "WiFi connect error: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    if (result) g_variant_unref(result);
    printf("📶 Connected to %s\n", ssid);
    return TRUE;
}

gboolean wifi_disconnect(void) {
    if (!nm_proxy || !wifi_device_path) return FALSE;
    
    GDBusProxy *dev = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, wifi_device_path,
        NM_DEVICE_INTERFACE, NULL, NULL);
    
    if (dev) {
        GError *error = NULL;
        g_dbus_proxy_call_sync(dev, "Disconnect", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        g_object_unref(dev);
        if (error) { g_error_free(error); return FALSE; }
        printf("📶 Disconnected\n");
        return TRUE;
    }
    return FALSE;
}

// ═══════════════════════════════════════════════════════════════════════════
// الشبكات المحفوظة
// ═══════════════════════════════════════════════════════════════════════════

GList* wifi_get_saved_networks(void) {
    GList *saved = NULL;
    GError *error = NULL;
    
    GDBusProxy *settings = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
        NM_SETTINGS, NULL, &error);
    
    if (!settings) return saved;
    
    GVariant *result = g_dbus_proxy_call_sync(settings, "ListConnections",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    if (result) {
        GVariantIter *iter;
        const gchar *conn_path;
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "&o", &conn_path)) {
            GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, conn_path,
                NM_CONNECTION, NULL, NULL);
            
            if (conn) {
                GVariant *settings_var = g_dbus_proxy_call_sync(conn, "GetSettings",
                    NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
                
                if (settings_var) {
                    GVariant *conn_dict = g_variant_lookup_value(settings_var, "connection", NULL);
                    if (conn_dict) {
                        GVariant *type_var = g_variant_lookup_value(conn_dict, "type", NULL);
                        if (type_var) {
                            const gchar *type = g_variant_get_string(type_var, NULL);
                            if (g_strcmp0(type, "802-11-wireless") == 0) {
                                GVariant *id_var = g_variant_lookup_value(conn_dict, "id", NULL);
                                if (id_var) {
                                    WiFiNetwork *net = g_new0(WiFiNetwork, 1);
                                    net->ssid = g_variant_dup_string(id_var, NULL);
                                    saved = g_list_append(saved, net);
                                    g_variant_unref(id_var);
                                }
                            }
                            g_variant_unref(type_var);
                        }
                        g_variant_unref(conn_dict);
                    }
                    g_variant_unref(settings_var);
                }
                g_object_unref(conn);
            }
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    g_object_unref(settings);
    return saved;
}

gboolean wifi_forget_network(const gchar *ssid) {
    GError *error = NULL;
    
    GDBusProxy *settings = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
        NM_SETTINGS, NULL, NULL);
    
    if (!settings) return FALSE;
    
    GVariant *result = g_dbus_proxy_call_sync(settings, "ListConnections",
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    gboolean found = FALSE;
    if (result) {
        GVariantIter *iter;
        const gchar *conn_path;
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "&o", &conn_path)) {
            GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, conn_path,
                NM_CONNECTION, NULL, NULL);
            
            if (conn) {
                GVariant *settings_var = g_dbus_proxy_call_sync(conn, "GetSettings",
                    NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
                
                if (settings_var) {
                    // GetSettings returns (a{sa{sv}}) - extract the dictionary
                    GVariant *settings_dict = g_variant_get_child_value(settings_var, 0);
                    if (settings_dict) {
                        GVariant *conn_dict = g_variant_lookup_value(settings_dict, "connection", NULL);
                        if (conn_dict) {
                            GVariant *id_var = g_variant_lookup_value(conn_dict, "id", NULL);
                            if (id_var) {
                                if (g_strcmp0(g_variant_get_string(id_var, NULL), ssid) == 0) {
                                    // قطع الاتصال أولاً إذا كان متصلاً
                                    wifi_disconnect();
                                    
                                    g_dbus_proxy_call_sync(conn, "Delete", NULL,
                                        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
                                    if (!error) {
                                        printf("📶 Forgot network: %s\n", ssid);
                                        found = TRUE;
                                    }
                                }
                                g_variant_unref(id_var);
                            }
                            g_variant_unref(conn_dict);
                        }
                        g_variant_unref(settings_dict);
                    }
                    g_variant_unref(settings_var);
                }
                g_object_unref(conn);
            }
            if (found) break;
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    
    g_object_unref(settings);
    return found;
}

// ═══════════════════════════════════════════════════════════════════════════
// الميزات المتقدمة
// ═══════════════════════════════════════════════════════════════════════════

void connection_details_free(ConnectionDetails *d) {
    if (d) {
        g_free(d->ssid);
        g_free(d->ip_address);
        g_free(d->gateway);
        g_free(d->subnet);
        g_free(d->dns);
        g_free(d);
    }
}

static gchar* find_connection_path(const gchar *ssid) {
    GDBusProxy *settings = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, "/org/freedesktop/NetworkManager/Settings",
        NM_SETTINGS, NULL, NULL);
    if (!settings) return NULL;
    
    gchar *found_path = NULL;
    GVariant *result = g_dbus_proxy_call_sync(settings, "ListConnections", NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    
    if (result) {
        GVariantIter *iter;
        const gchar *conn_path;
        g_variant_get(result, "(ao)", &iter);
        
        while (g_variant_iter_next(iter, "&o", &conn_path)) {
            GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
                G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, conn_path, NM_CONNECTION, NULL, NULL);
            if (conn) {
                GVariant *s = g_dbus_proxy_call_sync(conn, "GetSettings", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
                if (s) {
                    // GetSettings returns (a{sa{sv}}) - extract the dictionary
                    GVariant *settings_dict = g_variant_get_child_value(s, 0);
                    if (settings_dict) {
                        GVariant *cd = g_variant_lookup_value(settings_dict, "connection", NULL);
                        if (cd) {
                            GVariant *id = g_variant_lookup_value(cd, "id", NULL);
                            if (id && g_strcmp0(g_variant_get_string(id, NULL), ssid) == 0) {
                                found_path = g_strdup(conn_path);
                            }
                            if (id) g_variant_unref(id);
                            g_variant_unref(cd);
                        }
                        g_variant_unref(settings_dict);
                    }
                    g_variant_unref(s);
                }
                g_object_unref(conn);
            }
            if (found_path) break;
        }
        g_variant_iter_free(iter);
        g_variant_unref(result);
    }
    g_object_unref(settings);
    return found_path;
}

ConnectionDetails* wifi_get_connection_details(const gchar *ssid) {
    ConnectionDetails *d = g_new0(ConnectionDetails, 1);
    d->ssid = g_strdup(ssid);
    d->is_dhcp = TRUE;
    d->auto_connect = TRUE;
    
    gchar *path = find_connection_path(ssid);
    if (!path) return d;
    
    GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path, NM_CONNECTION, NULL, NULL);
    g_free(path);
    if (!conn) return d;
    
    GVariant *s = g_dbus_proxy_call_sync(conn, "GetSettings", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if (s) {
        // GetSettings returns (a{sa{sv}}) - extract the dictionary
        GVariant *settings_dict = g_variant_get_child_value(s, 0);
        if (settings_dict) {
            // Connection settings
            GVariant *cd = g_variant_lookup_value(settings_dict, "connection", NULL);
            if (cd) {
                GVariant *ac = g_variant_lookup_value(cd, "autoconnect", NULL);
                if (ac) { d->auto_connect = g_variant_get_boolean(ac); g_variant_unref(ac); }
                g_variant_unref(cd);
            }
            
            // IPv4 settings
            GVariant *ipv4 = g_variant_lookup_value(settings_dict, "ipv4", NULL);
            if (ipv4) {
                GVariant *method = g_variant_lookup_value(ipv4, "method", NULL);
                if (method) {
                    d->is_dhcp = (g_strcmp0(g_variant_get_string(method, NULL), "auto") == 0);
                    g_variant_unref(method);
                }
                
                GVariant *addrs = g_variant_lookup_value(ipv4, "address-data", NULL);
                if (addrs) {
                    GVariantIter iter;
                    GVariant *dict;
                    g_variant_iter_init(&iter, addrs);
                    if (g_variant_iter_next(&iter, "@a{sv}", &dict)) {
                        GVariant *a = g_variant_lookup_value(dict, "address", NULL);
                        if (a) { d->ip_address = g_variant_dup_string(a, NULL); g_variant_unref(a); }
                        GVariant *p = g_variant_lookup_value(dict, "prefix", NULL);
                        if (p) {
                            guint32 prefix = g_variant_get_uint32(p);
                            d->subnet = g_strdup_printf("%u", prefix);
                            g_variant_unref(p);
                        }
                        g_variant_unref(dict);
                    }
                    g_variant_unref(addrs);
                }
                
                GVariant *gw = g_variant_lookup_value(ipv4, "gateway", NULL);
                if (gw) { d->gateway = g_variant_dup_string(gw, NULL); g_variant_unref(gw); }
                
                GVariant *dns = g_variant_lookup_value(ipv4, "dns", NULL);
                if (dns) {
                    GVariantIter iter;
                    guint32 addr;
                    GString *dns_str = g_string_new("");
                    g_variant_iter_init(&iter, dns);
                    while (g_variant_iter_next(&iter, "u", &addr)) {
                        if (dns_str->len > 0) g_string_append(dns_str, ", ");
                        g_string_append_printf(dns_str, "%u.%u.%u.%u",
                            addr & 0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF, (addr >> 24) & 0xFF);
                    }
                    d->dns = g_string_free(dns_str, FALSE);
                    g_variant_unref(dns);
                }
                g_variant_unref(ipv4);
            }
            g_variant_unref(settings_dict);
        }
        g_variant_unref(s);
    }
    g_object_unref(conn);
    return d;
}

gboolean wifi_set_static_ip(const gchar *ssid, const gchar *ip, 
                            const gchar *gateway, const gchar *subnet, const gchar *dns) {
    gchar *path = find_connection_path(ssid);
    if (!path) return FALSE;
    
    GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path, NM_CONNECTION, NULL, NULL);
    g_free(path);
    if (!conn) return FALSE;
    
    // الحصول على الإعدادات الحالية
    GVariant *current = g_dbus_proxy_call_sync(conn, "GetSettings", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if (!current) { g_object_unref(conn); return FALSE; }
    
    // بناء إعدادات IPv4 الجديدة
    GVariantBuilder ipv4_builder, addr_builder, addr_dict_builder, dns_builder;
    
    g_variant_builder_init(&ipv4_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&ipv4_builder, "{sv}", "method", g_variant_new_string("manual"));
    
    // Address data
    g_variant_builder_init(&addr_builder, G_VARIANT_TYPE("aa{sv}"));
    g_variant_builder_init(&addr_dict_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&addr_dict_builder, "{sv}", "address", g_variant_new_string(ip));
    guint32 prefix = subnet ? atoi(subnet) : 24;
    g_variant_builder_add(&addr_dict_builder, "{sv}", "prefix", g_variant_new_uint32(prefix));
    g_variant_builder_add_value(&addr_builder, g_variant_builder_end(&addr_dict_builder));
    g_variant_builder_add(&ipv4_builder, "{sv}", "address-data", g_variant_builder_end(&addr_builder));
    
    // Gateway
    g_variant_builder_add(&ipv4_builder, "{sv}", "gateway", g_variant_new_string(gateway));
    
    // DNS
    if (dns && strlen(dns) > 0) {
        g_variant_builder_init(&dns_builder, G_VARIANT_TYPE("au"));
        gchar **parts = g_strsplit(dns, ".", 4);
        if (parts[0] && parts[1] && parts[2] && parts[3]) {
            guint32 dns_addr = atoi(parts[0]) | (atoi(parts[1]) << 8) | 
                               (atoi(parts[2]) << 16) | (atoi(parts[3]) << 24);
            g_variant_builder_add(&dns_builder, "u", dns_addr);
        }
        g_strfreev(parts);
        g_variant_builder_add(&ipv4_builder, "{sv}", "dns", g_variant_builder_end(&dns_builder));
    }
    
    // بناء الإعدادات الكاملة
    GVariantBuilder settings_builder;
    g_variant_builder_init(&settings_builder, G_VARIANT_TYPE("a{sa{sv}}"));
    
    GVariantIter iter;
    const gchar *section_name;
    GVariant *section_dict;
    g_variant_iter_init(&iter, g_variant_get_child_value(current, 0));
    
    while (g_variant_iter_next(&iter, "{&s@a{sv}}", &section_name, &section_dict)) {
        if (g_strcmp0(section_name, "ipv4") == 0) {
            g_variant_builder_add(&settings_builder, "{sa{sv}}", "ipv4", &ipv4_builder);
        } else {
            GVariantBuilder section_builder;
            g_variant_builder_init(&section_builder, G_VARIANT_TYPE("a{sv}"));
            
            GVariantIter props_iter;
            const gchar *prop_name;
            GVariant *prop_val;
            g_variant_iter_init(&props_iter, section_dict);
            while (g_variant_iter_next(&props_iter, "{&sv}", &prop_name, &prop_val)) {
                g_variant_builder_add(&section_builder, "{sv}", prop_name, prop_val);
            }
            g_variant_builder_add(&settings_builder, "{sa{sv}}", section_name, &section_builder);
        }
        g_variant_unref(section_dict);
    }
    
    GError *error = NULL;
    g_dbus_proxy_call_sync(conn, "Update", g_variant_new("(a{sa{sv}})", &settings_builder),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    g_variant_unref(current);
    g_object_unref(conn);
    
    if (error) { fprintf(stderr, "Set static IP error: %s\n", error->message); g_error_free(error); return FALSE; }
    printf("📶 Set static IP for %s: %s\n", ssid, ip);
    return TRUE;
}

gboolean wifi_set_dhcp(const gchar *ssid) {
    gchar *path = find_connection_path(ssid);
    if (!path) return FALSE;
    
    GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path, NM_CONNECTION, NULL, NULL);
    g_free(path);
    if (!conn) return FALSE;
    
    GVariant *current = g_dbus_proxy_call_sync(conn, "GetSettings", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if (!current) { g_object_unref(conn); return FALSE; }
    
    GVariantBuilder ipv4_builder;
    g_variant_builder_init(&ipv4_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&ipv4_builder, "{sv}", "method", g_variant_new_string("auto"));
    
    GVariantBuilder settings_builder;
    g_variant_builder_init(&settings_builder, G_VARIANT_TYPE("a{sa{sv}}"));
    
    GVariantIter iter;
    const gchar *section_name;
    GVariant *section_dict;
    g_variant_iter_init(&iter, g_variant_get_child_value(current, 0));
    
    while (g_variant_iter_next(&iter, "{&s@a{sv}}", &section_name, &section_dict)) {
        if (g_strcmp0(section_name, "ipv4") == 0) {
            g_variant_builder_add(&settings_builder, "{sa{sv}}", "ipv4", &ipv4_builder);
        } else {
            GVariantBuilder section_builder;
            g_variant_builder_init(&section_builder, G_VARIANT_TYPE("a{sv}"));
            
            GVariantIter props_iter;
            const gchar *prop_name;
            GVariant *prop_val;
            g_variant_iter_init(&props_iter, section_dict);
            while (g_variant_iter_next(&props_iter, "{&sv}", &prop_name, &prop_val)) {
                g_variant_builder_add(&section_builder, "{sv}", prop_name, prop_val);
            }
            g_variant_builder_add(&settings_builder, "{sa{sv}}", section_name, &section_builder);
        }
        g_variant_unref(section_dict);
    }
    
    GError *error = NULL;
    g_dbus_proxy_call_sync(conn, "Update", g_variant_new("(a{sa{sv}})", &settings_builder),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    g_variant_unref(current);
    g_object_unref(conn);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📶 Set DHCP for %s\n", ssid);
    return TRUE;
}

gboolean wifi_set_dns(const gchar *ssid, const gchar *dns1, const gchar *dns2) {
    gchar *path = find_connection_path(ssid);
    if (!path) return FALSE;
    
    GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path, NM_CONNECTION, NULL, NULL);
    g_free(path);
    if (!conn) return FALSE;
    
    GVariant *current = g_dbus_proxy_call_sync(conn, "GetSettings", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if (!current) { g_object_unref(conn); return FALSE; }
    
    GVariantBuilder settings_builder;
    g_variant_builder_init(&settings_builder, G_VARIANT_TYPE("a{sa{sv}}"));
    
    GVariantIter iter;
    const gchar *section_name;
    GVariant *section_dict;
    g_variant_iter_init(&iter, g_variant_get_child_value(current, 0));
    
    while (g_variant_iter_next(&iter, "{&s@a{sv}}", &section_name, &section_dict)) {
        GVariantBuilder section_builder;
        g_variant_builder_init(&section_builder, G_VARIANT_TYPE("a{sv}"));
        
        GVariantIter props_iter;
        const gchar *prop_name;
        GVariant *prop_val;
        g_variant_iter_init(&props_iter, section_dict);
        
        while (g_variant_iter_next(&props_iter, "{&sv}", &prop_name, &prop_val)) {
            if (g_strcmp0(section_name, "ipv4") == 0 && g_strcmp0(prop_name, "dns") == 0) {
                // تجاهل DNS القديم
            } else {
                g_variant_builder_add(&section_builder, "{sv}", prop_name, prop_val);
            }
        }
        
        if (g_strcmp0(section_name, "ipv4") == 0) {
            GVariantBuilder dns_builder;
            g_variant_builder_init(&dns_builder, G_VARIANT_TYPE("au"));
            
            if (dns1 && strlen(dns1) > 0) {
                gchar **p = g_strsplit(dns1, ".", 4);
                if (p[0] && p[1] && p[2] && p[3]) {
                    guint32 a = atoi(p[0]) | (atoi(p[1]) << 8) | (atoi(p[2]) << 16) | (atoi(p[3]) << 24);
                    g_variant_builder_add(&dns_builder, "u", a);
                }
                g_strfreev(p);
            }
            if (dns2 && strlen(dns2) > 0) {
                gchar **p = g_strsplit(dns2, ".", 4);
                if (p[0] && p[1] && p[2] && p[3]) {
                    guint32 a = atoi(p[0]) | (atoi(p[1]) << 8) | (atoi(p[2]) << 16) | (atoi(p[3]) << 24);
                    g_variant_builder_add(&dns_builder, "u", a);
                }
                g_strfreev(p);
            }
            g_variant_builder_add(&section_builder, "{sv}", "dns", g_variant_builder_end(&dns_builder));
        }
        
        g_variant_builder_add(&settings_builder, "{sa{sv}}", section_name, &section_builder);
        g_variant_unref(section_dict);
    }
    
    GError *error = NULL;
    g_dbus_proxy_call_sync(conn, "Update", g_variant_new("(a{sa{sv}})", &settings_builder),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    g_variant_unref(current);
    g_object_unref(conn);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📶 Set DNS for %s: %s, %s\n", ssid, dns1 ? dns1 : "", dns2 ? dns2 : "");
    return TRUE;
}

gboolean wifi_set_auto_connect(const gchar *ssid, gboolean auto_connect) {
    gchar *path = find_connection_path(ssid);
    if (!path) return FALSE;
    
    GDBusProxy *conn = g_dbus_proxy_new_sync(network_state.system_conn,
        G_DBUS_PROXY_FLAGS_NONE, NULL, NM_SERVICE, path, NM_CONNECTION, NULL, NULL);
    g_free(path);
    if (!conn) return FALSE;
    
    GVariant *current = g_dbus_proxy_call_sync(conn, "GetSettings", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    if (!current) { g_object_unref(conn); return FALSE; }
    
    GVariantBuilder settings_builder;
    g_variant_builder_init(&settings_builder, G_VARIANT_TYPE("a{sa{sv}}"));
    
    GVariantIter iter;
    const gchar *section_name;
    GVariant *section_dict;
    g_variant_iter_init(&iter, g_variant_get_child_value(current, 0));
    
    while (g_variant_iter_next(&iter, "{&s@a{sv}}", &section_name, &section_dict)) {
        GVariantBuilder section_builder;
        g_variant_builder_init(&section_builder, G_VARIANT_TYPE("a{sv}"));
        
        GVariantIter props_iter;
        const gchar *prop_name;
        GVariant *prop_val;
        g_variant_iter_init(&props_iter, section_dict);
        
        while (g_variant_iter_next(&props_iter, "{&sv}", &prop_name, &prop_val)) {
            if (g_strcmp0(section_name, "connection") == 0 && g_strcmp0(prop_name, "autoconnect") == 0) {
                // تجاهل القيمة القديمة
            } else {
                g_variant_builder_add(&section_builder, "{sv}", prop_name, prop_val);
            }
        }
        
        if (g_strcmp0(section_name, "connection") == 0) {
            g_variant_builder_add(&section_builder, "{sv}", "autoconnect", g_variant_new_boolean(auto_connect));
        }
        
        g_variant_builder_add(&settings_builder, "{sa{sv}}", section_name, &section_builder);
        g_variant_unref(section_dict);
    }
    
    GError *error = NULL;
    g_dbus_proxy_call_sync(conn, "Update", g_variant_new("(a{sa{sv}})", &settings_builder),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    
    g_variant_unref(current);
    g_object_unref(conn);
    
    if (error) { g_error_free(error); return FALSE; }
    printf("📶 Set auto-connect for %s: %s\n", ssid, auto_connect ? "yes" : "no");
    return TRUE;
}
