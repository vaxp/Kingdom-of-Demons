#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "venom_network.h"

// ═══════════════════════════════════════════════════════════════════════════
// 📱 Bluetooth - معلومات الجهاز
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    char *address;
    char *name;
    char *icon;
    gboolean paired;
    gboolean connected;
    gboolean trusted;
    int rssi;
} BluetoothDevice;

typedef struct {
    gboolean powered;
    gboolean discovering;
    char *name;
    char *address;
} BluetoothStatus;

// ═══════════════════════════════════════════════════════════════════════════
// الوظائف
// ═══════════════════════════════════════════════════════════════════════════

gboolean bluetooth_init(void);
void bluetooth_cleanup(void);

// الحالة
BluetoothStatus* bluetooth_get_status(void);
gboolean bluetooth_set_powered(gboolean powered);
gboolean bluetooth_set_discoverable(gboolean discoverable);

// الأجهزة
GList* bluetooth_get_devices(void);
gboolean bluetooth_start_scan(void);
gboolean bluetooth_stop_scan(void);

// الإقران والاتصال
gboolean bluetooth_pair(const gchar *address);
gboolean bluetooth_connect(const gchar *address);
gboolean bluetooth_disconnect(const gchar *address);
gboolean bluetooth_remove(const gchar *address);
gboolean bluetooth_trust(const gchar *address, gboolean trusted);

// تنظيف
void bluetooth_device_free(BluetoothDevice *device);
void bluetooth_status_free(BluetoothStatus *status);

#endif
