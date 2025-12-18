#ifndef DISPLAY_H
#define DISPLAY_H

#include "venom_display.h"
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// الأنواع الأساسية
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
    int temperature;
} NightLightSettings;

// ═══════════════════════════════════════════════════════════════════════════
// DPMS
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    DPMS_MODE_ON,
    DPMS_MODE_STANDBY,
    DPMS_MODE_SUSPEND,
    DPMS_MODE_OFF
} DPMSMode;

gboolean display_dpms_is_supported(void);
DPMSMode display_dpms_get_mode(void);
gboolean display_dpms_set_mode(DPMSMode mode);
gboolean display_dpms_set_timeouts(int standby, int suspend, int off);
void display_dpms_get_timeouts(int *standby, int *suspend, int *off);
gboolean display_dpms_enable(void);
gboolean display_dpms_disable(void);

// ═══════════════════════════════════════════════════════════════════════════
// EDID
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    char manufacturer[4];
    char model[64];
    char serial[16];
    unsigned int product_code;
    unsigned int serial_number;
    int year;
    int max_width;
    int max_height;
    gboolean hdr_supported;
    gboolean vrr_supported;
} EDIDInfo;

EDIDInfo* display_get_edid(const gchar *name);
void display_edid_free(EDIDInfo *info);
gchar* display_get_edid_hash(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// VRR
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    gboolean capable;
    gboolean enabled;
    int min_rate;
    int max_rate;
} VRRInfo;

gboolean display_vrr_capable(const gchar *name);
gboolean display_vrr_enabled(const gchar *name);
gboolean display_vrr_set(const gchar *name, gboolean enable);
VRRInfo display_vrr_get_info(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// Auto Profile
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    gchar *edid_match;
    gchar *profile_name;
} AutoProfileRule;

gboolean display_auto_set_rule(const gchar *edid_match, const gchar *profile);
gboolean display_auto_remove_rule(const gchar *edid_match);
GList* display_auto_get_rules(void);
void display_auto_rule_free(AutoProfileRule *rule);
gboolean display_auto_apply(void);
gboolean display_auto_save_current_as_rule(const gchar *profile_name);

// ═══════════════════════════════════════════════════════════════════════════
// Arrangement
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    ARRANGE_LEFT_OF,
    ARRANGE_RIGHT_OF,
    ARRANGE_ABOVE,
    ARRANGE_BELOW,
    ARRANGE_SAME_AS
} ArrangePosition;

gboolean display_arrange(const gchar *name, ArrangePosition pos, const gchar *relative_to);
gboolean display_auto_arrange(void);
ArrangePosition display_get_arrangement(const gchar *name, const gchar *relative_to);
gboolean display_extend_right(const gchar *name);
gboolean display_extend_left(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// Core Functions
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_init(void);
void display_cleanup(void);

GList* display_get_all(void);
DisplayInfo* display_get_by_name(const gchar *name);
DisplayInfo* display_get_primary(void);

void display_info_free(DisplayInfo *info);
void display_mode_free(DisplayMode *mode);

// Internal helpers (used across modules)
void display_refresh_resources(void);
XRRModeInfo* display_find_mode_by_id(RRMode mode_id);
double display_get_mode_refresh_rate(XRRModeInfo *mode);

// ═══════════════════════════════════════════════════════════════════════════
// Mode Functions
// ═══════════════════════════════════════════════════════════════════════════

GList* display_get_modes(const gchar *name);
gboolean display_set_resolution(const gchar *name, int width, int height);
gboolean display_set_refresh_rate(const gchar *name, double rate);
gboolean display_set_mode(const gchar *name, int width, int height, double rate);

// ═══════════════════════════════════════════════════════════════════════════
// Layout Functions
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_set_position(const gchar *name, int x, int y);
gboolean display_set_primary(const gchar *name);

RotationType display_get_rotation(const gchar *name);
gboolean display_set_rotation(const gchar *name, RotationType rotation);

gboolean display_enable(const gchar *name);
gboolean display_disable(const gchar *name);

gboolean display_set_mirror(const gchar *source, const gchar *target);
gboolean display_disable_mirror(const gchar *name);

double display_get_scale(const gchar *name);
gboolean display_set_scale(const gchar *name, double scale);

// ═══════════════════════════════════════════════════════════════════════════
// Night Light
// ═══════════════════════════════════════════════════════════════════════════

NightLightSettings display_get_night_light(void);
gboolean display_set_night_light(gboolean enabled, int temperature);

// ═══════════════════════════════════════════════════════════════════════════
// Profiles
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_save_profile(const gchar *profile_name);
gboolean display_load_profile(const gchar *profile_name);
GList* display_get_profiles(void);
gboolean display_delete_profile(const gchar *profile_name);

// ═══════════════════════════════════════════════════════════════════════════
// Backlight
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_backlight_available(void);
int display_backlight_get(void);
gboolean display_backlight_set(int level);
int display_backlight_get_max(void);
gchar* display_backlight_get_device(void);

// ═══════════════════════════════════════════════════════════════════════════
// DDC/CI
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_ddc_available(const gchar *name);
int display_ddc_get_brightness(const gchar *name);
gboolean display_ddc_set_brightness(const gchar *name, int level);
int display_ddc_get_contrast(const gchar *name);
gboolean display_ddc_set_contrast(const gchar *name, int level);
gboolean display_ddc_power_off(const gchar *name);
gboolean display_ddc_power_on(const gchar *name);

// ═══════════════════════════════════════════════════════════════════════════
// Persistence - الحفظ التلقائي
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_persist_save(void);
gboolean display_persist_load(void);
gboolean display_persist_save_night_light(void);
gboolean display_persist_load_night_light(void);
gboolean display_persist_restore_all(void);

#endif
