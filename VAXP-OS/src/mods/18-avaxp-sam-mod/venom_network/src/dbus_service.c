#include "dbus_service.h"
#include "wifi.h"
#include "bluetooth.h"
#include "ethernet.h"
#include "venom_network.h"
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 📄 D-Bus XML - WiFi
// ═══════════════════════════════════════════════════════════════════════════

const gchar* dbus_get_wifi_xml(void) {
    return
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' "
    "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    "  <interface name='org.venom.Network.WiFi'>"
    "    <method name='GetStatus'><arg type='(bsssssis)' direction='out'/></method>"
    "    <method name='GetNetworks'><arg type='a(ssiibb)' direction='out'/></method>"
    "    <method name='GetSavedNetworks'><arg type='as' direction='out'/></method>"
    "    <method name='IsEnabled'><arg type='b' direction='out'/></method>"
    "    <method name='SetEnabled'><arg type='b' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Connect'><arg type='s' name='ssid' direction='in'/><arg type='s' name='password' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Disconnect'><arg type='b' direction='out'/></method>"
    "    <method name='ForgetNetwork'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='GetConnectionDetails'><arg type='s' direction='in'/><arg type='(sssssbb)' direction='out'/></method>"
    "    <method name='SetStaticIP'><arg type='s' direction='in'/><arg type='s' direction='in'/><arg type='s' direction='in'/><arg type='s' direction='in'/><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='SetDHCP'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='SetDNS'><arg type='s' direction='in'/><arg type='s' direction='in'/><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='SetAutoConnect'><arg type='s' direction='in'/><arg type='b' direction='in'/><arg type='b' direction='out'/></method>"
    "    <signal name='NetworksChanged'/>"
    "    <signal name='StatusChanged'><arg type='b'/><arg type='s'/></signal>"
    "  </interface>"
    "</node>";
}

// ═══════════════════════════════════════════════════════════════════════════
// 📄 D-Bus XML - Bluetooth
// ═══════════════════════════════════════════════════════════════════════════

const gchar* dbus_get_bluetooth_xml(void) {
    return
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' "
    "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    "  <interface name='org.venom.Network.Bluetooth'>"
    "    <method name='GetStatus'><arg type='(bbss)' direction='out'/></method>"
    "    <method name='GetDevices'><arg type='a(sssbbbs)' direction='out'/></method>"
    "    <method name='SetPowered'><arg type='b' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='StartScan'><arg type='b' direction='out'/></method>"
    "    <method name='StopScan'><arg type='b' direction='out'/></method>"
    "    <method name='Pair'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Connect'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Disconnect'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Remove'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Trust'><arg type='s' direction='in'/><arg type='b' direction='in'/><arg type='b' direction='out'/></method>"
    "    <signal name='DevicesChanged'/>"
    "    <signal name='DeviceConnected'><arg type='s'/></signal>"
    "    <signal name='DeviceDisconnected'><arg type='s'/></signal>"
    "  </interface>"
    "</node>";
}

// ═══════════════════════════════════════════════════════════════════════════
// 📄 D-Bus XML - Ethernet
// ═══════════════════════════════════════════════════════════════════════════

const gchar* dbus_get_ethernet_xml(void) {
    return
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' "
    "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    "  <interface name='org.venom.Network.Ethernet'>"
    "    <method name='GetInterfaces'><arg type='a(ssssibb)' direction='out'/></method>"
    "    <method name='GetStatus'><arg type='s' direction='in'/><arg type='(ssssibb)' direction='out'/></method>"
    "    <method name='Enable'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <method name='Disable'><arg type='s' direction='in'/><arg type='b' direction='out'/></method>"
    "    <signal name='StatusChanged'><arg type='s'/><arg type='b'/></signal>"
    "  </interface>"
    "</node>";
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 WiFi Method Handler
// ═══════════════════════════════════════════════════════════════════════════

void dbus_handle_wifi_method(GDBusConnection *conn, const gchar *sender,
    const gchar *path, const gchar *interface, const gchar *method,
    GVariant *params, GDBusMethodInvocation *invoc, gpointer data) {
    (void)conn; (void)sender; (void)path; (void)interface; (void)data;
    
    printf("📶 WiFi: %s\n", method);
    
    if (g_strcmp0(method, "GetStatus") == 0) {
        WiFiStatus *s = wifi_get_status();
        g_dbus_method_invocation_return_value(invoc,
            g_variant_new("((bsssssis))", s->connected, s->ssid ? s->ssid : "",
                s->ip_address ? s->ip_address : "", s->gateway ? s->gateway : "",
                s->subnet ? s->subnet : "", s->dns ? s->dns : "", s->strength, s->speed));
        wifi_status_free(s);
    }
    else if (g_strcmp0(method, "GetNetworks") == 0) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("a(ssiibb)"));
        GList *nets = wifi_scan();
        for (GList *l = nets; l; l = l->next) {
            WiFiNetwork *n = l->data;
            g_variant_builder_add(&b, "(ssiibb)", n->ssid ? n->ssid : "",
                n->bssid ? n->bssid : "", n->strength, n->frequency, n->secured, n->connected);
            wifi_network_free(n);
        }
        g_list_free(nets);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(a(ssiibb))", &b));
    }
    else if (g_strcmp0(method, "GetSavedNetworks") == 0) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
        GList *saved = wifi_get_saved_networks();
        for (GList *l = saved; l; l = l->next) {
            WiFiNetwork *n = l->data;
            if (n->ssid) g_variant_builder_add(&b, "s", n->ssid);
            wifi_network_free(n);
        }
        g_list_free(saved);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(as)", &b));
    }
    else if (g_strcmp0(method, "IsEnabled") == 0) {
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_is_enabled()));
    }
    else if (g_strcmp0(method, "SetEnabled") == 0) {
        gboolean enabled;
        g_variant_get(params, "(b)", &enabled);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_set_enabled(enabled)));
    }
    else if (g_strcmp0(method, "Connect") == 0) {
        const gchar *ssid, *pass;
        g_variant_get(params, "(&s&s)", &ssid, &pass);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_connect(ssid, pass)));
    }
    else if (g_strcmp0(method, "Disconnect") == 0) {
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_disconnect()));
    }
    else if (g_strcmp0(method, "ForgetNetwork") == 0) {
        const gchar *ssid;
        g_variant_get(params, "(&s)", &ssid);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_forget_network(ssid)));
    }
    else if (g_strcmp0(method, "GetConnectionDetails") == 0) {
        const gchar *ssid;
        g_variant_get(params, "(&s)", &ssid);
        ConnectionDetails *d = wifi_get_connection_details(ssid);
        g_dbus_method_invocation_return_value(invoc,
            g_variant_new("((sssssbb))", d->ssid ? d->ssid : "",
                d->ip_address ? d->ip_address : "", d->gateway ? d->gateway : "",
                d->subnet ? d->subnet : "", d->dns ? d->dns : "",
                d->auto_connect, d->is_dhcp));
        connection_details_free(d);
    }
    else if (g_strcmp0(method, "SetStaticIP") == 0) {
        const gchar *ssid, *ip, *gw, *subnet, *dns;
        g_variant_get(params, "(&s&s&s&s&s)", &ssid, &ip, &gw, &subnet, &dns);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_set_static_ip(ssid, ip, gw, subnet, dns)));
    }
    else if (g_strcmp0(method, "SetDHCP") == 0) {
        const gchar *ssid;
        g_variant_get(params, "(&s)", &ssid);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_set_dhcp(ssid)));
    }
    else if (g_strcmp0(method, "SetDNS") == 0) {
        const gchar *ssid, *dns1, *dns2;
        g_variant_get(params, "(&s&s&s)", &ssid, &dns1, &dns2);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_set_dns(ssid, dns1, dns2)));
    }
    else if (g_strcmp0(method, "SetAutoConnect") == 0) {
        const gchar *ssid; gboolean ac;
        g_variant_get(params, "(&sb)", &ssid, &ac);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", wifi_set_auto_connect(ssid, ac)));
    }
    else {
        g_dbus_method_invocation_return_error(invoc, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 Bluetooth Method Handler
// ═══════════════════════════════════════════════════════════════════════════

void dbus_handle_bluetooth_method(GDBusConnection *conn, const gchar *sender,
    const gchar *path, const gchar *interface, const gchar *method,
    GVariant *params, GDBusMethodInvocation *invoc, gpointer data) {
    (void)conn; (void)sender; (void)path; (void)interface; (void)data;
    
    printf("📱 Bluetooth: %s\n", method);
    
    if (g_strcmp0(method, "GetStatus") == 0) {
        BluetoothStatus *s = bluetooth_get_status();
        g_dbus_method_invocation_return_value(invoc,
            g_variant_new("((bbss))", s->powered, s->discovering,
                s->name ? s->name : "", s->address ? s->address : ""));
        bluetooth_status_free(s);
    }
    else if (g_strcmp0(method, "GetDevices") == 0) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("a(sssbbbs)"));
        GList *devs = bluetooth_get_devices();
        for (GList *l = devs; l; l = l->next) {
            BluetoothDevice *d = l->data;
            g_variant_builder_add(&b, "(sssbbbs)", d->address ? d->address : "",
                d->name ? d->name : "", d->icon ? d->icon : "",
                d->paired, d->connected, d->trusted, "");
            bluetooth_device_free(d);
        }
        g_list_free(devs);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(a(sssbbbs))", &b));
    }
    else if (g_strcmp0(method, "SetPowered") == 0) {
        gboolean p;
        g_variant_get(params, "(b)", &p);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_set_powered(p)));
    }
    else if (g_strcmp0(method, "StartScan") == 0) {
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_start_scan()));
    }
    else if (g_strcmp0(method, "StopScan") == 0) {
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_stop_scan()));
    }
    else if (g_strcmp0(method, "Pair") == 0) {
        const gchar *addr;
        g_variant_get(params, "(&s)", &addr);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_pair(addr)));
    }
    else if (g_strcmp0(method, "Connect") == 0) {
        const gchar *addr;
        g_variant_get(params, "(&s)", &addr);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_connect(addr)));
    }
    else if (g_strcmp0(method, "Disconnect") == 0) {
        const gchar *addr;
        g_variant_get(params, "(&s)", &addr);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_disconnect(addr)));
    }
    else if (g_strcmp0(method, "Remove") == 0) {
        const gchar *addr;
        g_variant_get(params, "(&s)", &addr);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_remove(addr)));
    }
    else if (g_strcmp0(method, "Trust") == 0) {
        const gchar *addr; gboolean t;
        g_variant_get(params, "(&sb)", &addr, &t);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", bluetooth_trust(addr, t)));
    }
    else {
        g_dbus_method_invocation_return_error(invoc, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 Ethernet Method Handler
// ═══════════════════════════════════════════════════════════════════════════

void dbus_handle_ethernet_method(GDBusConnection *conn, const gchar *sender,
    const gchar *path, const gchar *interface, const gchar *method,
    GVariant *params, GDBusMethodInvocation *invoc, gpointer data) {
    (void)conn; (void)sender; (void)path; (void)interface; (void)data;
    
    printf("🔌 Ethernet: %s\n", method);
    
    if (g_strcmp0(method, "GetInterfaces") == 0) {
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("a(ssssibb)"));
        GList *ifaces = ethernet_get_interfaces();
        for (GList *l = ifaces; l; l = l->next) {
            EthernetInterface *i = l->data;
            g_variant_builder_add(&b, "(ssssibb)", i->name ? i->name : "",
                i->mac_address ? i->mac_address : "", i->ip_address ? i->ip_address : "",
                i->gateway ? i->gateway : "", i->speed, i->connected, i->enabled);
            ethernet_interface_free(i);
        }
        g_list_free(ifaces);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(a(ssssibb))", &b));
    }
    else if (g_strcmp0(method, "GetStatus") == 0) {
        const gchar *name;
        g_variant_get(params, "(&s)", &name);
        EthernetInterface *i = ethernet_get_status(name);
        if (i) {
            g_dbus_method_invocation_return_value(invoc,
                g_variant_new("((ssssibb))", i->name ? i->name : "",
                    i->mac_address ? i->mac_address : "", i->ip_address ? i->ip_address : "",
                    i->gateway ? i->gateway : "", i->speed, i->connected, i->enabled));
            ethernet_interface_free(i);
        } else {
            g_dbus_method_invocation_return_error(invoc, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Not found");
        }
    }
    else if (g_strcmp0(method, "Enable") == 0) {
        const gchar *name;
        g_variant_get(params, "(&s)", &name);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", ethernet_enable(name)));
    }
    else if (g_strcmp0(method, "Disable") == 0) {
        const gchar *name;
        g_variant_get(params, "(&s)", &name);
        g_dbus_method_invocation_return_value(invoc, g_variant_new("(b)", ethernet_disable(name)));
    }
    else {
        g_dbus_method_invocation_return_error(invoc, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// الإشارات
// ═══════════════════════════════════════════════════════════════════════════

void dbus_emit_signal(const gchar *path, const gchar *interface, 
                      const gchar *signal_name, GVariant *params) {
    if (!network_state.session_conn) return;
    g_dbus_connection_emit_signal(network_state.session_conn, NULL, path, interface, signal_name, params, NULL);
}

// ═══════════════════════════════════════════════════════════════════════════
// VTables
// ═══════════════════════════════════════════════════════════════════════════

const GDBusInterfaceVTable wifi_vtable = { dbus_handle_wifi_method, NULL, NULL };
const GDBusInterfaceVTable bluetooth_vtable = { dbus_handle_bluetooth_method, NULL, NULL };
const GDBusInterfaceVTable ethernet_vtable = { dbus_handle_ethernet_method, NULL, NULL };
