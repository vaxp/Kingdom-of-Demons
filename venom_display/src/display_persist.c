/*
 * display_persist.c - Venom Display Auto Persistence
 * حفظ وتحميل الإعدادات تلقائياً
 */

#include "display.h"
#include "venom_display.h"
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// ثوابت
// ═══════════════════════════════════════════════════════════════════════════

#define PERSIST_CONFIG_PATH "/.config/venom/display-current.conf"
#define NIGHT_LIGHT_PATH "/.config/venom/night-light.conf"

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

static gchar* get_persist_path(void) {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s%s", home, PERSIST_CONFIG_PATH);
}

static gchar* get_night_light_path(void) {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s%s", home, NIGHT_LIGHT_PATH);
}

static void ensure_config_dir(void) {
    const gchar *home = g_get_home_dir();
    gchar *dir = g_strdup_printf("%s/.config/venom", home);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);
}

// ═══════════════════════════════════════════════════════════════════════════
// حفظ الإعدادات الحالية
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_persist_save(void) {
    ensure_config_dir();
    
    gchar *path = get_persist_path();
    FILE *f = fopen(path, "w");
    if (!f) {
        g_free(path);
        return FALSE;
    }
    
    GList *displays = display_get_all();
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected && d->crtc_id) {
            fprintf(f, "[%s]\n", d->name);
            fprintf(f, "width=%d\n", d->width);
            fprintf(f, "height=%d\n", d->height);
            fprintf(f, "rate=%.2f\n", d->refresh_rate);
            fprintf(f, "x=%d\n", d->x);
            fprintf(f, "y=%d\n", d->y);
            fprintf(f, "primary=%d\n", d->is_primary);
            fprintf(f, "rotation=%d\n", (int)display_get_rotation(d->name));
            fprintf(f, "enabled=1\n");
            fprintf(f, "\n");
        }
        display_info_free(d);
    }
    g_list_free(displays);
    
    fclose(f);
    g_free(path);
    return TRUE;
}

gboolean display_persist_save_night_light(void) {
    ensure_config_dir();
    
    gchar *path = get_night_light_path();
    FILE *f = fopen(path, "w");
    if (!f) {
        g_free(path);
        return FALSE;
    }
    
    NightLightSettings nl = display_get_night_light();
    fprintf(f, "[NightLight]\n");
    fprintf(f, "enabled=%d\n", nl.enabled);
    fprintf(f, "temperature=%d\n", nl.temperature);
    
    fclose(f);
    g_free(path);
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// تحميل وتطبيق الإعدادات المحفوظة
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_persist_load(void) {
    gchar *path = get_persist_path();
    GKeyFile *kf = g_key_file_new();
    
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return FALSE;
    }
    
    gchar **groups = g_key_file_get_groups(kf, NULL);
    for (int i = 0; groups[i]; i++) {
        const gchar *name = groups[i];
        
        // تحقق من وجود الشاشة
        DisplayInfo *current = display_get_by_name(name);
        if (!current || !current->is_connected) {
            display_info_free(current);
            continue;
        }
        display_info_free(current);
        
        int w = g_key_file_get_integer(kf, name, "width", NULL);
        int h = g_key_file_get_integer(kf, name, "height", NULL);
        double rate = g_key_file_get_double(kf, name, "rate", NULL);
        int x = g_key_file_get_integer(kf, name, "x", NULL);
        int y = g_key_file_get_integer(kf, name, "y", NULL);
        gboolean primary = g_key_file_get_boolean(kf, name, "primary", NULL);
        int rotation = g_key_file_get_integer(kf, name, "rotation", NULL);
        gboolean enabled = g_key_file_get_boolean(kf, name, "enabled", NULL);
        
        if (enabled && w > 0 && h > 0) {
            display_set_mode(name, w, h, rate);
            display_set_position(name, x, y);
            display_set_rotation(name, (RotationType)rotation);
            if (primary) display_set_primary(name);
        } else if (!enabled) {
            display_disable(name);
        }
    }
    
    g_strfreev(groups);
    g_key_file_free(kf);
    g_free(path);
    return TRUE;
}

gboolean display_persist_load_night_light(void) {
    gchar *path = get_night_light_path();
    GKeyFile *kf = g_key_file_new();
    
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return FALSE;
    }
    
    gboolean enabled = g_key_file_get_boolean(kf, "NightLight", "enabled", NULL);
    int temp = g_key_file_get_integer(kf, "NightLight", "temperature", NULL);
    
    if (temp > 0) {
        display_set_night_light(enabled, temp);
    }
    
    g_key_file_free(kf);
    g_free(path);
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// تحميل جميع الإعدادات عند بدء التشغيل
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_persist_restore_all(void) {
    gboolean result = TRUE;
    
    // أولاً: محاولة تطبيق Auto Profiles
    if (!display_auto_apply()) {
        // إذا لم توجد قواعد auto، تحميل الإعدادات المحفوظة
        result = display_persist_load();
    }
    
    // تحميل إعدادات Night Light
    display_persist_load_night_light();
    
    return result;
}
