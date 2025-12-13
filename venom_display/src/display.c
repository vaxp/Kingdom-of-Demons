#include "display.h"
#include "venom_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🖥️ إدارة الشاشات باستخدام XRandR
// ═══════════════════════════════════════════════════════════════════════════

static XRRScreenResources *screen_res = NULL;

gboolean display_init(void) {
    display_state.x_display = XOpenDisplay(NULL);
    if (!display_state.x_display) {
        fprintf(stderr, "❌ Cannot open X display\n");
        return FALSE;
    }
    
    display_state.screen = DefaultScreen(display_state.x_display);
    display_state.root_window = RootWindow(display_state.x_display, display_state.screen);
    
    screen_res = XRRGetScreenResources(display_state.x_display, display_state.root_window);
    if (!screen_res) {
        fprintf(stderr, "❌ Cannot get screen resources\n");
        return FALSE;
    }
    
    printf("🖥️ XRandR initialized (%d outputs detected)\n", screen_res->noutput);
    return TRUE;
}

void display_cleanup(void) {
    if (screen_res) {
        XRRFreeScreenResources(screen_res);
        screen_res = NULL;
    }
    if (display_state.x_display) {
        XCloseDisplay(display_state.x_display);
        display_state.x_display = NULL;
    }
}

void display_info_free(DisplayInfo *info) {
    if (info) {
        g_free(info->name);
        g_free(info);
    }
}

void display_mode_free(DisplayMode *mode) {
    if (mode) {
        g_free(mode->rates);
        g_free(mode);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 📺 جلب معلومات الشاشات
// ═══════════════════════════════════════════════════════════════════════════

static void refresh_screen_resources(void) {
    if (screen_res) {
        XRRFreeScreenResources(screen_res);
    }
    screen_res = XRRGetScreenResources(display_state.x_display, display_state.root_window);
}

static XRRModeInfo* find_mode_by_id(RRMode mode_id) {
    for (int i = 0; i < screen_res->nmode; i++) {
        if (screen_res->modes[i].id == mode_id) {
            return &screen_res->modes[i];
        }
    }
    return NULL;
}

static double get_mode_refresh_rate(XRRModeInfo *mode) {
    if (mode->hTotal && mode->vTotal) {
        return (double)mode->dotClock / ((double)mode->hTotal * (double)mode->vTotal);
    }
    return 0.0;
}

GList* display_get_all(void) {
    GList *displays = NULL;
    
    // تحديث البيانات دائماً للحصول على أحدث حالة
    refresh_screen_resources();
    
    RROutput primary = XRRGetOutputPrimary(display_state.x_display, display_state.root_window);
    
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        DisplayInfo *info = g_new0(DisplayInfo, 1);
        info->name = g_strdup(output->name);
        info->output_id = screen_res->outputs[i];
        info->is_connected = (output->connection == RR_Connected);
        info->is_primary = (screen_res->outputs[i] == primary);
        info->crtc_id = output->crtc;
        
        // الحصول على الدقة الحالية
        if (output->crtc) {
            XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, output->crtc);
            if (crtc) {
                info->width = crtc->width;
                info->height = crtc->height;
                info->x = crtc->x;
                info->y = crtc->y;
                
                XRRModeInfo *mode = find_mode_by_id(crtc->mode);
                if (mode) {
                    info->refresh_rate = get_mode_refresh_rate(mode);
                }
                XRRFreeCrtcInfo(crtc);
            }
        }
        
        displays = g_list_append(displays, info);
        XRRFreeOutputInfo(output);
    }
    
    return displays;
}

DisplayInfo* display_get_by_name(const gchar *name) {
    GList *displays = display_get_all();
    DisplayInfo *result = NULL;
    
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *info = l->data;
        if (g_strcmp0(info->name, name) == 0) {
            result = info;
        } else {
            display_info_free(info);
        }
    }
    g_list_free(displays);
    
    return result;
}

DisplayInfo* display_get_primary(void) {
    GList *displays = display_get_all();
    DisplayInfo *result = NULL;
    
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *info = l->data;
        if (info->is_primary && info->is_connected) {
            result = info;
        } else {
            display_info_free(info);
        }
    }
    g_list_free(displays);
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🎛️ الأوضاع المتاحة
// ═══════════════════════════════════════════════════════════════════════════

GList* display_get_modes(const gchar *name) {
    GList *modes = NULL;
    
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, name) == 0) {
            for (int j = 0; j < output->nmode; j++) {
                XRRModeInfo *mode_info = find_mode_by_id(output->modes[j]);
                if (mode_info) {
                    DisplayMode *mode = g_new0(DisplayMode, 1);
                    mode->width = mode_info->width;
                    mode->height = mode_info->height;
                    mode->rates = g_new(double, 1);
                    mode->rates[0] = get_mode_refresh_rate(mode_info);
                    mode->rate_count = 1;
                    modes = g_list_append(modes, mode);
                }
            }
        }
        XRRFreeOutputInfo(output);
    }
    
    return modes;
}

// ═══════════════════════════════════════════════════════════════════════════
// ⚙️ تغيير الإعدادات
// ═══════════════════════════════════════════════════════════════════════════

static RRMode find_mode(const gchar *output_name, int width, int height, double rate) {
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, output_name) == 0) {
            for (int j = 0; j < output->nmode; j++) {
                XRRModeInfo *mode = find_mode_by_id(output->modes[j]);
                if (mode && mode->width == (unsigned)width && mode->height == (unsigned)height) {
                    double mode_rate = get_mode_refresh_rate(mode);
                    if (rate <= 0 || fabs(mode_rate - rate) < 1.0) {
                        RRMode result = mode->id;
                        XRRFreeOutputInfo(output);
                        return result;
                    }
                }
            }
        }
        XRRFreeOutputInfo(output);
    }
    return None;
}

gboolean display_set_mode(const gchar *name, int width, int height, double rate) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    RRMode mode = find_mode(name, width, height, rate);
    if (mode == None) {
        display_info_free(info);
        return FALSE;
    }
    
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, info->x, info->y, mode,
                                      RR_Rotate_0, &info->output_id, 1);
    
    display_info_free(info);
    
    if (status == RRSetConfigSuccess) {
        printf("🖥️ Display %s set to %dx%d@%.1fHz\n", name, width, height, rate);
        return TRUE;
    }
    
    return FALSE;
}

gboolean display_set_resolution(const gchar *name, int width, int height) {
    return display_set_mode(name, width, height, -1);
}

gboolean display_set_refresh_rate(const gchar *name, double rate) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info) return FALSE;
    
    gboolean result = display_set_mode(name, info->width, info->height, rate);
    display_info_free(info);
    return result;
}

gboolean display_set_position(const gchar *name, int x, int y) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, info->crtc_id);
    if (!crtc) {
        display_info_free(info);
        return FALSE;
    }
    
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, x, y, crtc->mode,
                                      crtc->rotation, crtc->outputs, crtc->noutput);
    
    XRRFreeCrtcInfo(crtc);
    display_info_free(info);
    
    if (status == RRSetConfigSuccess) {
        printf("🖥️ Display %s moved to (%d, %d)\n", name, x, y);
        return TRUE;
    }
    
    return FALSE;
}

gboolean display_set_primary(const gchar *name) {
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, name) == 0) {
            XRRSetOutputPrimary(display_state.x_display, display_state.root_window, 
                               screen_res->outputs[i]);
            XRRFreeOutputInfo(output);
            printf("🖥️ %s set as primary display\n", name);
            return TRUE;
        }
        XRRFreeOutputInfo(output);
    }
    return FALSE;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔄 التدوير
// ═══════════════════════════════════════════════════════════════════════════

static Rotation rotation_type_to_xrandr(RotationType type) {
    switch (type) {
        case ROTATION_NORMAL: return RR_Rotate_0;
        case ROTATION_LEFT: return RR_Rotate_90;
        case ROTATION_INVERTED: return RR_Rotate_180;
        case ROTATION_RIGHT: return RR_Rotate_270;
        default: return RR_Rotate_0;
    }
}

static RotationType xrandr_to_rotation_type(Rotation rot) {
    if (rot & RR_Rotate_90) return ROTATION_LEFT;
    if (rot & RR_Rotate_180) return ROTATION_INVERTED;
    if (rot & RR_Rotate_270) return ROTATION_RIGHT;
    return ROTATION_NORMAL;
}

RotationType display_get_rotation(const gchar *name) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return ROTATION_NORMAL;
    }
    
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, info->crtc_id);
    if (!crtc) {
        display_info_free(info);
        return ROTATION_NORMAL;
    }
    
    RotationType result = xrandr_to_rotation_type(crtc->rotation);
    XRRFreeCrtcInfo(crtc);
    display_info_free(info);
    return result;
}

gboolean display_set_rotation(const gchar *name, RotationType rotation) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, info->crtc_id);
    if (!crtc) {
        display_info_free(info);
        return FALSE;
    }
    
    Rotation xrot = rotation_type_to_xrandr(rotation);
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, crtc->x, crtc->y, crtc->mode,
                                      xrot, crtc->outputs, crtc->noutput);
    
    XRRFreeCrtcInfo(crtc);
    display_info_free(info);
    
    if (status == RRSetConfigSuccess) {
        printf("🔄 Display %s rotated to %d°\n", name, (int)rotation);
        return TRUE;
    }
    return FALSE;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 تشغيل/إيقاف
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_enable(const gchar *name) {
    // للتفعيل: نستخدم الدقة الافتراضية
    for (int i = 0; i < screen_res->noutput; i++) {
        XRROutputInfo *output = XRRGetOutputInfo(display_state.x_display, screen_res, 
                                                  screen_res->outputs[i]);
        if (!output) continue;
        
        if (g_strcmp0(output->name, name) == 0 && output->nmode > 0) {
            // استخدام أول وضع متاح
            XRRModeInfo *mode = find_mode_by_id(output->modes[0]);
            if (mode) {
                // البحث عن CRTC متاح
                for (int j = 0; j < output->ncrtc; j++) {
                    XRRCrtcInfo *crtc = XRRGetCrtcInfo(display_state.x_display, screen_res, 
                                                        output->crtcs[j]);
                    if (crtc && crtc->noutput == 0) {
                        Status status = XRRSetCrtcConfig(display_state.x_display, screen_res,
                            output->crtcs[j], CurrentTime, 0, 0, mode->id, RR_Rotate_0,
                            &screen_res->outputs[i], 1);
                        XRRFreeCrtcInfo(crtc);
                        XRRFreeOutputInfo(output);
                        if (status == RRSetConfigSuccess) {
                            printf("🔌 Display %s enabled\n", name);
                            return TRUE;
                        }
                        return FALSE;
                    }
                    if (crtc) XRRFreeCrtcInfo(crtc);
                }
            }
        }
        XRRFreeOutputInfo(output);
    }
    return FALSE;
}

gboolean display_disable(const gchar *name) {
    DisplayInfo *info = display_get_by_name(name);
    if (!info || !info->crtc_id) {
        display_info_free(info);
        return FALSE;
    }
    
    Status status = XRRSetCrtcConfig(display_state.x_display, screen_res, info->crtc_id,
                                      CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
    display_info_free(info);
    
    if (status == RRSetConfigSuccess) {
        printf("🔌 Display %s disabled\n", name);
        return TRUE;
    }
    return FALSE;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🪞 المطابقة (Mirror)
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_set_mirror(const gchar *source, const gchar *target) {
    DisplayInfo *src = display_get_by_name(source);
    DisplayInfo *tgt = display_get_by_name(target);
    
    if (!src || !tgt) {
        display_info_free(src);
        display_info_free(tgt);
        return FALSE;
    }
    
    // ضبط الهدف على نفس دقة المصدر ونفس الموقع
    gboolean result = display_set_mode(target, src->width, src->height, -1);
    if (result) {
        result = display_set_position(target, src->x, src->y);
    }
    
    display_info_free(src);
    display_info_free(tgt);
    
    if (result) {
        printf("🪞 %s mirrored to %s\n", source, target);
    }
    return result;
}

gboolean display_disable_mirror(const gchar *name) {
    // إزاحة الشاشة لإلغاء المطابقة
    DisplayInfo *info = display_get_by_name(name);
    if (!info) return FALSE;
    
    // البحث عن شاشة في نفس الموقع ووضع هذه الشاشة بجانبها
    GList *displays = display_get_all();
    int new_x = 0;
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *other = l->data;
        if (g_strcmp0(other->name, name) != 0 && other->is_connected) {
            new_x = other->x + other->width;
        }
        display_info_free(other);
    }
    g_list_free(displays);
    
    gboolean result = display_set_position(name, new_x, 0);
    display_info_free(info);
    
    if (result) {
        printf("🪞 Mirror disabled for %s\n", name);
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔍 التكبير (Scale) - باستخدام xrandr transform
// ═══════════════════════════════════════════════════════════════════════════

double display_get_scale(const gchar *name) {
    (void)name;
    // XRandR لا يوفر scaling مباشر، نرجع 1.0 افتراضياً
    // يمكن استخدام Xft.dpi أو GNOME settings لاحقاً
    return 1.0;
}

gboolean display_set_scale(const gchar *name, double scale) {
    if (scale <= 0 || scale > 4.0) return FALSE;
    
    // استخدام xrandr transform
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xrandr --output %s --scale %.2fx%.2f 2>/dev/null", 
             name, 1.0/scale, 1.0/scale);
    int result = system(cmd);
    
    if (result == 0) {
        printf("🔍 Display %s scaled to %.1fx\n", name, scale);
        return TRUE;
    }
    return FALSE;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🌙 Night Light - باستخدام xrandr gamma
// ═══════════════════════════════════════════════════════════════════════════

static NightLightSettings night_light = {FALSE, 6500};

NightLightSettings display_get_night_light(void) {
    return night_light;
}

static void apply_gamma(const gchar *output, double r, double g, double b) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xrandr --output %s --gamma %.2f:%.2f:%.2f 2>/dev/null",
             output, r, g, b);
    system(cmd);
}

gboolean display_set_night_light(gboolean enabled, int temperature) {
    if (temperature < 1000) temperature = 1000;
    if (temperature > 10000) temperature = 10000;
    
    night_light.enabled = enabled;
    night_light.temperature = temperature;
    
    // حساب الـ gamma بناءً على درجة الحرارة
    double temp = temperature / 100.0;
    double r = 1.0, g = 1.0, b = 1.0;
    
    if (enabled && temperature < 6500) {
        // تقليل الأزرق وزيادة الدفء
        if (temp <= 66) {
            r = 1.0;
            g = 0.39 * log(temp) - 0.63;
            if (g < 0) g = 0; if (g > 1) g = 1;
        }
        if (temp <= 65) {
            b = 0.0;
            if (temp > 19) {
                b = 0.54 * log(temp - 10) - 1.0;
                if (b < 0) b = 0; if (b > 1) b = 1;
            }
        }
    }
    
    // تطبيق على جميع الشاشات المتصلة
    GList *displays = display_get_all();
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected && d->crtc_id) {
            apply_gamma(d->name, r, g, b);
        }
        display_info_free(d);
    }
    g_list_free(displays);
    
    printf("🌙 Night Light: %s (temperature: %dK)\n", enabled ? "ON" : "OFF", temperature);
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// 💾 البروفايلات
// ═══════════════════════════════════════════════════════════════════════════

#define PROFILE_DIR "/.config/venom/display-profiles"

static gchar* get_profile_path(const gchar *name) {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s%s/%s.conf", home, PROFILE_DIR, name);
}

gboolean display_save_profile(const gchar *profile_name) {
    const gchar *home = g_get_home_dir();
    gchar *dir = g_strdup_printf("%s%s", home, PROFILE_DIR);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);
    
    gchar *path = get_profile_path(profile_name);
    FILE *f = fopen(path, "w");
    if (!f) {
        g_free(path);
        return FALSE;
    }
    
    GList *displays = display_get_all();
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected) {
            fprintf(f, "[%s]\n", d->name);
            fprintf(f, "width=%d\n", d->width);
            fprintf(f, "height=%d\n", d->height);
            fprintf(f, "rate=%.2f\n", d->refresh_rate);
            fprintf(f, "x=%d\n", d->x);
            fprintf(f, "y=%d\n", d->y);
            fprintf(f, "primary=%d\n", d->is_primary);
            fprintf(f, "enabled=%d\n", d->crtc_id != 0);
            fprintf(f, "\n");
        }
        display_info_free(d);
    }
    g_list_free(displays);
    
    fclose(f);
    g_free(path);
    printf("💾 Profile saved: %s\n", profile_name);
    return TRUE;
}

gboolean display_load_profile(const gchar *profile_name) {
    gchar *path = get_profile_path(profile_name);
    GKeyFile *kf = g_key_file_new();
    
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return FALSE;
    }
    
    gchar **groups = g_key_file_get_groups(kf, NULL);
    for (int i = 0; groups[i]; i++) {
        const gchar *name = groups[i];
        int w = g_key_file_get_integer(kf, name, "width", NULL);
        int h = g_key_file_get_integer(kf, name, "height", NULL);
        double rate = g_key_file_get_double(kf, name, "rate", NULL);
        int x = g_key_file_get_integer(kf, name, "x", NULL);
        int y = g_key_file_get_integer(kf, name, "y", NULL);
        gboolean primary = g_key_file_get_boolean(kf, name, "primary", NULL);
        gboolean enabled = g_key_file_get_boolean(kf, name, "enabled", NULL);
        
        if (enabled) {
            display_set_mode(name, w, h, rate);
            display_set_position(name, x, y);
            if (primary) display_set_primary(name);
        } else {
            display_disable(name);
        }
    }
    
    g_strfreev(groups);
    g_key_file_free(kf);
    g_free(path);
    printf("💾 Profile loaded: %s\n", profile_name);
    return TRUE;
}

GList* display_get_profiles(void) {
    GList *profiles = NULL;
    const gchar *home = g_get_home_dir();
    gchar *dir = g_strdup_printf("%s%s", home, PROFILE_DIR);
    
    GDir *d = g_dir_open(dir, 0, NULL);
    if (d) {
        const gchar *filename;
        while ((filename = g_dir_read_name(d))) {
            if (g_str_has_suffix(filename, ".conf")) {
                gchar *name = g_strndup(filename, strlen(filename) - 5);
                profiles = g_list_append(profiles, name);
            }
        }
        g_dir_close(d);
    }
    
    g_free(dir);
    return profiles;
}

gboolean display_delete_profile(const gchar *profile_name) {
    gchar *path = get_profile_path(profile_name);
    gboolean result = (g_remove(path) == 0);
    g_free(path);
    if (result) {
        printf("🗑️ Profile deleted: %s\n", profile_name);
    }
    return result;
}
