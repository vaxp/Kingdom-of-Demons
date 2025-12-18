/*
 * display_auto.c - Venom Display Auto Profile
 * تطبيق بروفايل تلقائي حسب الشاشة المتصلة
 */

#include "display.h"
#include <stdio.h>
#include <string.h>

// ═══════════════════════════════════════════════════════════════════════════
// ثوابت
// ═══════════════════════════════════════════════════════════════════════════

#define AUTO_CONFIG_PATH "/.config/venom/display-auto.conf"

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

static gchar* get_auto_config_path(void) {
    const gchar *home = g_get_home_dir();
    return g_strdup_printf("%s%s", home, AUTO_CONFIG_PATH);
}

// ═══════════════════════════════════════════════════════════════════════════
// Auto Profile API
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_auto_set_rule(const gchar *edid_match, const gchar *profile) {
    gchar *path = get_auto_config_path();
    
    // Ensure directory exists
    gchar *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0755);
    g_free(dir);
    
    GKeyFile *kf = g_key_file_new();
    g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL);
    
    g_key_file_set_string(kf, "AutoProfiles", edid_match, profile);
    
    gboolean result = g_key_file_save_to_file(kf, path, NULL);
    
    g_key_file_free(kf);
    g_free(path);
    return result;
}

gboolean display_auto_remove_rule(const gchar *edid_match) {
    gchar *path = get_auto_config_path();
    GKeyFile *kf = g_key_file_new();
    
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return FALSE;
    }
    
    gboolean result = g_key_file_remove_key(kf, "AutoProfiles", edid_match, NULL);
    if (result) {
        result = g_key_file_save_to_file(kf, path, NULL);
    }
    
    g_key_file_free(kf);
    g_free(path);
    return result;
}

GList* display_auto_get_rules(void) {
    GList *rules = NULL;
    gchar *path = get_auto_config_path();
    GKeyFile *kf = g_key_file_new();
    
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return NULL;
    }
    
    gchar **keys = g_key_file_get_keys(kf, "AutoProfiles", NULL, NULL);
    if (keys) {
        for (int i = 0; keys[i]; i++) {
            gchar *profile = g_key_file_get_string(kf, "AutoProfiles", keys[i], NULL);
            if (profile) {
                AutoProfileRule *rule = g_new0(AutoProfileRule, 1);
                rule->edid_match = g_strdup(keys[i]);
                rule->profile_name = profile;
                rules = g_list_append(rules, rule);
            }
        }
        g_strfreev(keys);
    }
    
    g_key_file_free(kf);
    g_free(path);
    return rules;
}

void display_auto_rule_free(AutoProfileRule *rule) {
    if (rule) {
        g_free(rule->edid_match);
        g_free(rule->profile_name);
        g_free(rule);
    }
}

gboolean display_auto_apply(void) {
    gchar *path = get_auto_config_path();
    GKeyFile *kf = g_key_file_new();
    
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return FALSE;
    }
    
    gboolean applied = FALSE;
    GList *displays = display_get_all();
    
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected) {
            gchar *hash = display_get_edid_hash(d->name);
            if (hash) {
                gchar *profile = g_key_file_get_string(kf, "AutoProfiles", hash, NULL);
                if (profile) {
                    display_load_profile(profile);
                    g_free(profile);
                    applied = TRUE;
                }
                g_free(hash);
            }
        }
        display_info_free(d);
    }
    g_list_free(displays);
    
    g_key_file_free(kf);
    g_free(path);
    return applied;
}

gboolean display_auto_save_current_as_rule(const gchar *profile_name) {
    // Save current config as profile
    if (!display_save_profile(profile_name)) {
        return FALSE;
    }
    
    // Create auto rules for all connected displays
    GList *displays = display_get_all();
    gboolean success = TRUE;
    
    for (GList *l = displays; l; l = l->next) {
        DisplayInfo *d = l->data;
        if (d->is_connected) {
            gchar *hash = display_get_edid_hash(d->name);
            if (hash) {
                if (!display_auto_set_rule(hash, profile_name)) {
                    success = FALSE;
                }
                g_free(hash);
            }
        }
        display_info_free(d);
    }
    g_list_free(displays);
    
    return success;
}
