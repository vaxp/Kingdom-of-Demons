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
    gboolean pairable;
    char *name;
    char *address;
} BluetoothStatus;

// ═══════════════════════════════════════════════════════════════════════════
// 🔋 Battery Support
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    gint percentage;      // 0-100, -1 if not available
    gboolean available;   // TRUE if battery info is available
} BluetoothBattery;

// ═══════════════════════════════════════════════════════════════════════════
// 🎵 Audio Profile Support
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    gboolean a2dp_sink;      // Audio playback (speaker/headphones)
    gboolean a2dp_source;    // Audio source (send audio)
    gboolean hfp_hf;         // Handsfree profile (headset)
    gboolean hfp_ag;         // Audio gateway (phone)
    gboolean hid;            // Human Interface Device (keyboard/mouse)
} BluetoothProfiles;

// ═══════════════════════════════════════════════════════════════════════════
// 🔐 Pairing Agent Callbacks
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    // Called when pairing needs passkey confirmation
    // Return TRUE to confirm, FALSE to reject
    gboolean (*confirm_passkey)(const gchar *device_path, guint32 passkey);
    
    // Called to display passkey to user (for display-only devices)
    void (*display_passkey)(const gchar *device_path, guint32 passkey, guint16 entered);
    
    // Called to request PIN code (for older devices)
    // Return PIN string (caller must free) or NULL to cancel
    gchar* (*request_pincode)(const gchar *device_path);
    
    // Called when pairing is complete
    void (*pairing_complete)(const gchar *device_path, gboolean success);
} BluetoothAgentCallbacks;

// ═══════════════════════════════════════════════════════════════════════════
// ❌ Error Handling
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    BT_ERROR_NONE = 0,
    BT_ERROR_NOT_AVAILABLE,      // BlueZ not running
    BT_ERROR_NO_ADAPTER,         // No Bluetooth adapter found
    BT_ERROR_DEVICE_NOT_FOUND,   // Device not found
    BT_ERROR_ALREADY_CONNECTED,  // Already connected
    BT_ERROR_NOT_CONNECTED,      // Not connected
    BT_ERROR_AUTH_FAILED,        // Authentication failed
    BT_ERROR_AUTH_CANCELED,      // Authentication canceled
    BT_ERROR_AUTH_REJECTED,      // Authentication rejected
    BT_ERROR_AUTH_TIMEOUT,       // Authentication timeout
    BT_ERROR_CONNECTION_FAILED,  // Connection failed
    BT_ERROR_UNKNOWN             // Unknown error
} BluetoothError;

typedef struct {
    BluetoothError code;
    gchar *message;
} BluetoothResult;

// ═══════════════════════════════════════════════════════════════════════════
// الوظائف الأساسية
// ═══════════════════════════════════════════════════════════════════════════

gboolean bluetooth_init(void);
void bluetooth_cleanup(void);

// التحقق من التوفر
gboolean bluetooth_is_available(void);
gboolean bluetooth_has_adapter(void);

// الحالة
BluetoothStatus* bluetooth_get_status(void);
gboolean bluetooth_set_powered(gboolean powered);
gboolean bluetooth_set_discoverable(gboolean discoverable);
gboolean bluetooth_set_pairable(gboolean pairable);

// الأجهزة
GList* bluetooth_get_devices(void);
gboolean bluetooth_start_scan(void);
gboolean bluetooth_stop_scan(void);

// الإقران والاتصال
BluetoothResult bluetooth_pair(const gchar *address);
BluetoothResult bluetooth_connect(const gchar *address);
BluetoothResult bluetooth_disconnect(const gchar *address);
gboolean bluetooth_remove(const gchar *address);
gboolean bluetooth_trust(const gchar *address, gboolean trusted);

// ═══════════════════════════════════════════════════════════════════════════
// الميزات المتقدمة
// ═══════════════════════════════════════════════════════════════════════════

// Battery
BluetoothBattery* bluetooth_get_battery(const gchar *address);
void bluetooth_battery_free(BluetoothBattery *battery);

// Profiles
BluetoothProfiles* bluetooth_get_profiles(const gchar *address);
void bluetooth_profiles_free(BluetoothProfiles *profiles);

// Pairing Agent
gboolean bluetooth_register_agent(BluetoothAgentCallbacks *callbacks);
void bluetooth_unregister_agent(void);

// ═══════════════════════════════════════════════════════════════════════════
// الإشارات (Signals)
// ═══════════════════════════════════════════════════════════════════════════

// Subscribe to device changes
typedef void (*BluetoothDeviceCallback)(const gchar *address, gboolean added);
void bluetooth_subscribe_device_changes(BluetoothDeviceCallback callback);

typedef void (*BluetoothConnectionCallback)(const gchar *address, gboolean connected);
void bluetooth_subscribe_connection_changes(BluetoothConnectionCallback callback);

typedef void (*BluetoothPropertyCallback)(const gchar *address, const gchar *property, GVariant *value);
void bluetooth_subscribe_property_changes(BluetoothPropertyCallback callback);

// ═══════════════════════════════════════════════════════════════════════════
// تنظيف
// ═══════════════════════════════════════════════════════════════════════════

void bluetooth_device_free(BluetoothDevice *device);
void bluetooth_status_free(BluetoothStatus *status);
void bluetooth_result_free(BluetoothResult *result);

const gchar* bluetooth_error_to_string(BluetoothError error);

#endif
