#ifndef WIFI_H
#define WIFI_H

#include "venom_network.h"

// ═══════════════════════════════════════════════════════════════════════════
// 📶 WiFi - معلومات الشبكة
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    char *ssid;
    char *bssid;
    int strength;
    int frequency;
    gboolean secured;
    gboolean connected;
} WiFiNetwork;

typedef struct {
    gboolean connected;
    char *ssid;
    char *ip_address;
    char *gateway;
    char *subnet;
    char *dns;
    int strength;
    int speed;
} WiFiStatus;

typedef struct {
    char *ssid;
    char *ip_address;
    char *gateway;
    char *subnet;
    char *dns;
    gboolean auto_connect;
    gboolean is_dhcp;
} ConnectionDetails;

// ═══════════════════════════════════════════════════════════════════════════
// الوظائف الأساسية
// ═══════════════════════════════════════════════════════════════════════════

gboolean wifi_init(void);
void wifi_cleanup(void);

// الحالة
WiFiStatus* wifi_get_status(void);
gboolean wifi_is_enabled(void);
gboolean wifi_set_enabled(gboolean enabled);

// الشبكات
GList* wifi_scan(void);
GList* wifi_get_saved_networks(void);
gboolean wifi_connect(const gchar *ssid, const gchar *password);
gboolean wifi_disconnect(void);
gboolean wifi_forget_network(const gchar *ssid);

// ═══════════════════════════════════════════════════════════════════════════
// الميزات المتقدمة
// ═══════════════════════════════════════════════════════════════════════════

// تفاصيل الاتصال
ConnectionDetails* wifi_get_connection_details(const gchar *ssid);
void connection_details_free(ConnectionDetails *details);

// IP ثابت
gboolean wifi_set_static_ip(const gchar *ssid, const gchar *ip, 
                            const gchar *gateway, const gchar *subnet, const gchar *dns);
gboolean wifi_set_dhcp(const gchar *ssid);

// DNS
gboolean wifi_set_dns(const gchar *ssid, const gchar *dns1, const gchar *dns2);

// الاتصال التلقائي
gboolean wifi_set_auto_connect(const gchar *ssid, gboolean auto_connect);

// تنظيف
void wifi_network_free(WiFiNetwork *network);
void wifi_status_free(WiFiStatus *status);

#endif
