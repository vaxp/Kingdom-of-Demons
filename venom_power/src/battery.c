#include "battery.h"
#include "venom_power.h"
#include "dbus_service.h"
#include "logind.h"
#include "idle.h"
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🔋 مراقبة البطارية
// ═══════════════════════════════════════════════════════════════════════════

static gboolean warned_low = FALSE;
static gboolean warned_critical = FALSE;

void battery_get_info(gdouble *percentage, gboolean *charging, gint64 *time_to_empty) {
    *percentage = 0;
    *charging = FALSE;
    *time_to_empty = 0;
    
    if (!power_state.system_conn) return;
    
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        power_state.system_conn,
        "org.freedesktop.UPower",
        "/org/freedesktop/UPower/devices/DisplayDevice",
        "org.freedesktop.DBus.Properties",
        "GetAll",
        g_variant_new("(s)", "org.freedesktop.UPower.Device"),
        G_VARIANT_TYPE("(a{sv})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error
    );
    
    if (error) {
        g_error_free(error);
        return;
    }
    
    GVariant *props = NULL;
    g_variant_get(result, "(@a{sv})", &props);
    
    if (props) {
        GVariant *v;
        
        if ((v = g_variant_lookup_value(props, "Percentage", G_VARIANT_TYPE_DOUBLE))) {
            *percentage = g_variant_get_double(v);
            g_variant_unref(v);
        }
        if ((v = g_variant_lookup_value(props, "State", G_VARIANT_TYPE_UINT32))) {
            *charging = (g_variant_get_uint32(v) == 1);
            g_variant_unref(v);
        }
        if ((v = g_variant_lookup_value(props, "TimeToEmpty", G_VARIANT_TYPE_INT64))) {
            *time_to_empty = g_variant_get_int64(v);
            g_variant_unref(v);
        }
        g_variant_unref(props);
    }
    g_variant_unref(result);
}

void battery_check_level(gdouble percentage, gboolean charging) {
    if (charging) {
        warned_low = FALSE;
        warned_critical = FALSE;
        return;
    }
    
    if (percentage <= BATTERY_DANGER) {
        printf("🚨 CRITICAL: Battery at %.0f%%! Hibernating to save data!\n", percentage);
        dbus_send_notification("⚠️ بطارية حرجة!", "سيتم إيقاف الجهاز لحماية البيانات", "critical");
        dbus_emit_signal("BatteryCritical", g_variant_new("(d)", percentage));
        
        if (!logind_call("Hibernate", g_variant_new("(b)", TRUE))) {
            logind_call("PowerOff", g_variant_new("(b)", TRUE));
        }
    }
    else if (percentage <= BATTERY_CRITICAL && !warned_critical) {
        printf("⚠️ Battery critical: %.0f%%\n", percentage);
        dbus_send_notification("⚠️ البطارية 5%", "يرجى شبك الشاحن فوراً!", "critical");
        dbus_emit_signal("BatteryWarning", g_variant_new("(d)", percentage));
        warned_critical = TRUE;
    }
    else if (percentage <= BATTERY_LOW && !warned_low) {
        printf("🔋 Battery low: %.0f%%\n", percentage);
        dbus_send_notification("🔋 البطارية منخفضة", "10% متبقية", "normal");
        dbus_emit_signal("BatteryWarning", g_variant_new("(d)", percentage));
        warned_low = TRUE;
    }
}

void battery_on_properties_changed(GDBusConnection *connection,
                                   const gchar *sender_name,
                                   const gchar *object_path,
                                   const gchar *interface_name,
                                   const gchar *signal_name,
                                   GVariant *parameters,
                                   gpointer user_data) {
    (void)connection; (void)sender_name; (void)object_path;
    (void)interface_name; (void)signal_name; (void)user_data;
    GVariant *changed_props = NULL;
    const gchar *iface = NULL;
    
    g_variant_get(parameters, "(&s@a{sv}as)", &iface, &changed_props, NULL);
    
    if (changed_props) {
        GVariant *percentage_v = g_variant_lookup_value(changed_props, "Percentage", G_VARIANT_TYPE_DOUBLE);
        GVariant *state_v = g_variant_lookup_value(changed_props, "State", G_VARIANT_TYPE_UINT32);
        
        // تحديث حالة الشحن إذا تغيرت
        if (state_v) {
            guint32 bat_state = g_variant_get_uint32(state_v);
            // State: 1 = Charging, 2 = Discharging, 4 = Full
            gboolean new_on_battery = (bat_state == 2);
            
            // إذا تغيرت حالة الشحن، أرسل إشارة فوراً
            if (new_on_battery != power_state.on_battery) {
                power_state.on_battery = new_on_battery;
                gboolean charging = !new_on_battery;
                printf("🔋 Battery: %.0f%% (%s)\n", power_state.battery_percentage, 
                       charging ? "charging" : "discharging");
                dbus_emit_signal("BatteryChanged", 
                    g_variant_new("(db)", power_state.battery_percentage, charging));
            }
            g_variant_unref(state_v);
        }
        
        if (percentage_v) {
            gdouble pct = g_variant_get_double(percentage_v);
            gboolean charging = !power_state.on_battery;
            
            // إرسال إشارة تغير البطارية عند تغير النسبة
            if (pct != power_state.battery_percentage) {
                power_state.battery_percentage = pct;
                printf("🔋 Battery: %.0f%% (%s)\n", pct, charging ? "charging" : "discharging");
                dbus_emit_signal("BatteryChanged", g_variant_new("(db)", pct, charging));
            }
            
            battery_check_level(pct, charging);
            
            g_variant_unref(percentage_v);
        }
        g_variant_unref(changed_props);
    }
}

void battery_on_power_source_changed(GDBusConnection *connection,
                                     const gchar *sender_name,
                                     const gchar *object_path,
                                     const gchar *interface_name,
                                     const gchar *signal_name,
                                     GVariant *parameters,
                                     gpointer user_data) {
    (void)connection; (void)sender_name; (void)object_path;
    (void)interface_name; (void)signal_name; (void)user_data;
    GVariant *changed_props = NULL;
    const gchar *iface = NULL;
    
    g_variant_get(parameters, "(&s@a{sv}as)", &iface, &changed_props, NULL);
    
    if (changed_props) {
        GVariant *on_battery_v = g_variant_lookup_value(changed_props, "OnBattery", G_VARIANT_TYPE_BOOLEAN);
        
        if (on_battery_v) {
            gboolean on_battery = g_variant_get_boolean(on_battery_v);
            
            if (on_battery != power_state.on_battery) {
                power_state.on_battery = on_battery;
                printf("🔌 Power source: %s\n", on_battery ? "Battery" : "AC");
                dbus_emit_signal("PowerSourceChanged", g_variant_new("(b)", on_battery));
                idle_reset_timers();
            }
            g_variant_unref(on_battery_v);
        }
        g_variant_unref(changed_props);
    }
}

void battery_setup_monitoring(void) {
    if (!power_state.system_conn) return;
    
    // مراقبة بطارية العرض
    g_dbus_connection_signal_subscribe(
        power_state.system_conn,
        "org.freedesktop.UPower",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "/org/freedesktop/UPower/devices/DisplayDevice",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        battery_on_properties_changed,
        NULL, NULL
    );
    
    // مراقبة تغيير مصدر الطاقة
    g_dbus_connection_signal_subscribe(
        power_state.system_conn,
        "org.freedesktop.UPower",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "/org/freedesktop/UPower",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        battery_on_power_source_changed,
        NULL, NULL
    );
    
    printf("🔋 UPower monitoring active\n");
}
