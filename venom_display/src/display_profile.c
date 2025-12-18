/*
 * display_profile.c - Venom Display Profiles
 * حفظ/تحميل البروفايلات
 */

#include "display.h"
#include "venom_display.h"
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// ثوابت
// ═══════════════════════════════════════════════════════════════════════════

#define PROFILE_DIR "/.config/venom/display-profiles"

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

static gchar* get_profile_path(const gchar *name) {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s%s/%s.conf", home, PROFILE_DIR, name);
}

// ═══════════════════════════════════════════════════════════════════════════
// حفظ البروفايل
// ═══════════════════════════════════════════════════════════════════════════

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
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// تحميل البروفايل
// ═══════════════════════════════════════════════════════════════════════════

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
    return TRUE;
}

// ═══════════════════════════════════════════════════════════════════════════
// قائمة البروفايلات
// ═══════════════════════════════════════════════════════════════════════════

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
    return result;
}
