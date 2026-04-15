#ifndef VENOM_POWER_H
#define VENOM_POWER_H

#include <gio/gio.h>
#include <glib.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🔧 الإعدادات والثوابت
// ═══════════════════════════════════════════════════════════════════════════

#define DAEMON_VERSION "2.1.0"

// المسار الافتراضي لأمر قفل الشاشة (يمكن تجاوزه من ملف الإعدادات)
#define DEFAULT_LOCK_SCREEN_CMD   "loginctl lock-session"
#define LOCK_SCREEN_CMD           DEFAULT_LOCK_SCREEN_CMD
#define BACKLIGHT_PATH "/sys/class/backlight"

// أوقات الخمول (بالثواني)
#define DEFAULT_DIM_TIMEOUT      120   // 2 دقيقة
#define DEFAULT_BLANK_TIMEOUT    300   // 5 دقائق
#define DEFAULT_SUSPEND_TIMEOUT  900   // 15 دقيقة (على البطارية فقط)

// مستويات البطارية
#define BATTERY_LOW      10
#define BATTERY_CRITICAL  5
#define BATTERY_DANGER    2

// ═══════════════════════════════════════════════════════════════════════════
// 📦 أنواع البيانات
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    INHIBIT_IDLE     = 1 << 0,
    INHIBIT_SUSPEND  = 1 << 1,
    INHIBIT_SHUTDOWN = 1 << 2
} InhibitType;

typedef struct {
    guint id;
    gchar *app_name;
    gchar *reason;
    InhibitType type;
} Inhibitor;

typedef struct {
    // إعدادات الخمول
    guint dim_timeout;
    guint blank_timeout;
    guint suspend_timeout;
    
    // الحالة الحالية
    gboolean is_idle;
    gboolean screen_dimmed;
    gboolean screen_blanked;
    gint original_brightness;
    
    // (المؤقتات تُدار داخلياً في idle.c)
    
    // حالة البطارية
    gdouble battery_percentage;
    gboolean on_battery;
    gboolean lid_closed;
    
    // المانعات
    GList *inhibitors;
    guint next_inhibitor_id;
    
    // الاتصالات
    GDBusConnection *session_conn;
    GDBusConnection *system_conn;
} PowerState;

// المتغير العام للحالة
extern PowerState power_state;
extern GMainLoop *main_loop;

#endif // VENOM_POWER_H
