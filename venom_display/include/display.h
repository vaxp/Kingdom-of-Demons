#ifndef DISPLAY_H
#define DISPLAY_H

#include "venom_display.h"

// ═══════════════════════════════════════════════════════════════════════════
// 🖥️ معلومات الشاشة
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    ROTATION_NORMAL = 0,
    ROTATION_LEFT = 90,
    ROTATION_INVERTED = 180,
    ROTATION_RIGHT = 270
} RotationType;

typedef struct {
    char *name;
    int width;
    int height;
    double refresh_rate;
    int x;
    int y;
    gboolean is_primary;
    gboolean is_connected;
    gboolean is_enabled;
    RotationType rotation;
    double scale;
    RROutput output_id;
    RRCrtc crtc_id;
} DisplayInfo;

typedef struct {
    int width;
    int height;
    double *rates;
    int rate_count;
} DisplayMode;

typedef struct {
    gboolean enabled;
    int temperature;  // 1000K - 10000K, default 6500K
} NightLightSettings;

// ═══════════════════════════════════════════════════════════════════════════
// الوظائف الأساسية
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_init(void);
void display_cleanup(void);

GList* display_get_all(void);
DisplayInfo* display_get_by_name(const gchar *name);
DisplayInfo* display_get_primary(void);

GList* display_get_modes(const gchar *name);
gboolean display_set_resolution(const gchar *name, int width, int height);
gboolean display_set_refresh_rate(const gchar *name, double rate);
gboolean display_set_mode(const gchar *name, int width, int height, double rate);
gboolean display_set_position(const gchar *name, int x, int y);
gboolean display_set_primary(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// 🔄 التدوير
// ═══════════════════════════════════════════════════════════════════════════

RotationType display_get_rotation(const gchar *name);
gboolean display_set_rotation(const gchar *name, RotationType rotation);

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 تشغيل/إيقاف
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_enable(const gchar *name);
gboolean display_disable(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// 🪞 المطابقة
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_set_mirror(const gchar *source, const gchar *target);
gboolean display_disable_mirror(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// 🔍 التكبير
// ═══════════════════════════════════════════════════════════════════════════

double display_get_scale(const gchar *name);
gboolean display_set_scale(const gchar *name, double scale);

// ═══════════════════════════════════════════════════════════════════════════
// 🌙 Night Light
// ═══════════════════════════════════════════════════════════════════════════

NightLightSettings display_get_night_light(void);
gboolean display_set_night_light(gboolean enabled, int temperature);

// ═══════════════════════════════════════════════════════════════════════════
// 💾 البروفايلات
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_save_profile(const gchar *profile_name);
gboolean display_load_profile(const gchar *profile_name);
GList* display_get_profiles(void);
gboolean display_delete_profile(const gchar *profile_name);

// ═══════════════════════════════════════════════════════════════════════════
// 🧹 تنظيف
// ═══════════════════════════════════════════════════════════════════════════

void display_info_free(DisplayInfo *info);
void display_mode_free(DisplayMode *mode);

#endif
