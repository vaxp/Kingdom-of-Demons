#include "venom_network.h"
#include "wifi.h"
#include "bluetooth.h"
#include "ethernet.h"
#include "dbus_service.h"
#include <stdio.h>
#include <signal.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🌐 Venom Network Daemon
// ═══════════════════════════════════════════════════════════════════════════

NetworkState network_state = {0};
GMainLoop *main_loop = NULL;

static void cleanup(void) {
    printf("\n🛑 Shutting down...\n");
    wifi_cleanup();
    bluetooth_cleanup();
    ethernet_cleanup();
    
    if (network_state.session_conn) {
        g_object_unref(network_state.session_conn);
    }
    if (network_state.system_conn) {
        g_object_unref(network_state.system_conn);
    }
    if (main_loop) {
        g_main_loop_quit(main_loop);
    }
}

static void signal_handler(int sig) {
    (void)sig;
    cleanup();
}

static void on_bus_acquired(GDBusConnection *conn, const gchar *name, gpointer data) {
    (void)name; (void)data;
    GError *error = NULL;
    
    printf("📡 D-Bus connection acquired\n");
    
    // تسجيل واجهة WiFi
    GDBusNodeInfo *wifi_info = g_dbus_node_info_new_for_xml(dbus_get_wifi_xml(), NULL);
    if (wifi_info) {
        g_dbus_connection_register_object(conn, DBUS_OBJECT_PATH_WIFI,
            wifi_info->interfaces[0], &wifi_vtable, NULL, NULL, &error);
        if (error) { fprintf(stderr, "WiFi register error: %s\n", error->message); g_clear_error(&error); }
        else printf("📶 WiFi interface registered\n");
    }
    
    // تسجيل واجهة Bluetooth
    GDBusNodeInfo *bt_info = g_dbus_node_info_new_for_xml(dbus_get_bluetooth_xml(), NULL);
    if (bt_info) {
        g_dbus_connection_register_object(conn, DBUS_OBJECT_PATH_BLUETOOTH,
            bt_info->interfaces[0], &bluetooth_vtable, NULL, NULL, &error);
        if (error) { fprintf(stderr, "Bluetooth register error: %s\n", error->message); g_clear_error(&error); }
        else printf("📱 Bluetooth interface registered\n");
    }
    
    // تسجيل واجهة Ethernet
    GDBusNodeInfo *eth_info = g_dbus_node_info_new_for_xml(dbus_get_ethernet_xml(), NULL);
    if (eth_info) {
        g_dbus_connection_register_object(conn, DBUS_OBJECT_PATH_ETHERNET,
            eth_info->interfaces[0], &ethernet_vtable, NULL, NULL, &error);
        if (error) { fprintf(stderr, "Ethernet register error: %s\n", error->message); g_clear_error(&error); }
        else printf("🔌 Ethernet interface registered\n");
    }
}

static void on_name_acquired(GDBusConnection *conn, const gchar *name, gpointer data) {
    (void)conn; (void)data;
    printf("✅ D-Bus name acquired: %s\n", name);
}

static void on_name_lost(GDBusConnection *conn, const gchar *name, gpointer data) {
    (void)conn; (void)data;
    fprintf(stderr, "❌ D-Bus name lost: %s\n", name);
    cleanup();
}

int main(void) {
    printf("🌐 ═══════════════════════════════════════════════════════════════\n");
    printf("🌐 Venom Network Daemon v1.0\n");
    printf("🌐 ═══════════════════════════════════════════════════════════════\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    GError *error = NULL;
    
    // الاتصال بـ session bus
    network_state.session_conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!network_state.session_conn) {
        fprintf(stderr, "❌ Cannot connect to session bus: %s\n", error->message);
        return 1;
    }
    
    // الاتصال بـ system bus (للتحكم بـ NetworkManager و BlueZ)
    network_state.system_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!network_state.system_conn) {
        fprintf(stderr, "❌ Cannot connect to system bus: %s\n", error->message);
        return 1;
    }
    
    // تهيئة المكونات
    if (!wifi_init()) {
        fprintf(stderr, "⚠️ WiFi init failed (NetworkManager might not be running)\n");
    }
    if (!bluetooth_init()) {
        fprintf(stderr, "⚠️ Bluetooth init failed (BlueZ might not be running)\n");
    }
    if (!ethernet_init()) {
        fprintf(stderr, "⚠️ Ethernet init failed\n");
    }
    
    // تسجيل خدمة D-Bus
    g_bus_own_name(G_BUS_TYPE_SESSION, DBUS_SERVICE_NAME,
        G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired,
        on_name_lost, NULL, NULL);
    
    printf("🚀 Starting main loop...\n");
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);
    
    cleanup();
    return 0;
}
