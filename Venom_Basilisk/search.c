/*
 * ═══════════════════════════════════════════════════════════════════════════
 * 🔎 Venom Basilisk - Search Module
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "search.h"
#include "window.h"

extern BasiliskState *state;

// ═══════════════════════════════════════════════════════════════════════════
// App Cache
// ═══════════════════════════════════════════════════════════════════════════

static void free_app_entry(gpointer data) {
    AppEntry *app = (AppEntry *)data;
    if (app) {
        g_free(app->name);
        g_free(app->exec);
        g_free(app->icon);
        g_free(app->desktop_file);
        g_free(app);
    }
}

void search_load_apps(void) {
    if (state->app_cache) return;
    
    const gchar *dirs[] = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        NULL
    };
    
    for (int d = 0; dirs[d] != NULL; d++) {
        GDir *dir = g_dir_open(dirs[d], 0, NULL);
        if (!dir) continue;
        
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            if (!g_str_has_suffix(filename, ".desktop")) continue;
            
            gchar *filepath = g_build_filename(dirs[d], filename, NULL);
            GKeyFile *kf = g_key_file_new();
            
            if (g_key_file_load_from_file(kf, filepath, G_KEY_FILE_NONE, NULL)) {
                gboolean no_display = g_key_file_get_boolean(kf, "Desktop Entry", "NoDisplay", NULL);
                gboolean hidden = g_key_file_get_boolean(kf, "Desktop Entry", "Hidden", NULL);
                
                if (!no_display && !hidden) {
                    gchar *name = g_key_file_get_locale_string(kf, "Desktop Entry", "Name", NULL, NULL);
                    if (name) {
                        AppEntry *app = g_new0(AppEntry, 1);
                        app->name = name;
                        app->exec = g_key_file_get_string(kf, "Desktop Entry", "Exec", NULL);
                        app->icon = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
                        app->desktop_file = g_strdup(filepath);
                        state->app_cache = g_list_append(state->app_cache, app);
                    }
                }
            }
            
            g_key_file_free(kf);
            g_free(filepath);
        }
        g_dir_close(dir);
    }
}

void search_free_apps(void) {
    g_list_free_full(state->app_cache, free_app_entry);
    state->app_cache = NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// Result Click Handler
// ═══════════════════════════════════════════════════════════════════════════

static void on_result_clicked(GtkButton *button, gpointer data) {
    (void)button;
    const gchar *desktop_file = (const gchar *)data;
    
    GDesktopAppInfo *app_info = g_desktop_app_info_new_from_filename(desktop_file);
    if (app_info) {
        g_app_info_launch(G_APP_INFO(app_info), NULL, NULL, NULL);
        g_object_unref(app_info);
    }
    
    window_hide();
}

// ═══════════════════════════════════════════════════════════════════════════
// UI Creation
// ═══════════════════════════════════════════════════════════════════════════

static GtkWidget* create_result_row(AppEntry *app) {
    GtkWidget *button = gtk_button_new();
    gtk_widget_set_name(button, "result-row");
    GtkStyleContext *ctx = gtk_widget_get_style_context(button);
    gtk_style_context_add_class(ctx, "result-row");
    
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_add(GTK_CONTAINER(button), hbox);
    
    // Icon
    GtkWidget *icon = NULL;
    if (app->icon) {
        GtkIconTheme *theme = gtk_icon_theme_get_default();
        if (g_path_is_absolute(app->icon)) {
            GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(app->icon, 32, 32, TRUE, NULL);
            if (pb) {
                icon = gtk_image_new_from_pixbuf(pb);
                g_object_unref(pb);
            }
        } else {
            GdkPixbuf *pb = gtk_icon_theme_load_icon(theme, app->icon, 32, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
            if (pb) {
                icon = gtk_image_new_from_pixbuf(pb);
                g_object_unref(pb);
            }
        }
    }
    if (!icon) {
        icon = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DND);
    }
    gtk_style_context_add_class(gtk_widget_get_style_context(icon), "result-icon");
    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    
    // Text
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    
    GtkWidget *name_label = gtk_label_new(app->name);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(name_label), "result-name");
    gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
    
    // Command hint
    if (app->exec) {
        gchar *hint = g_path_get_basename(app->exec);
        // Remove %u, %U, etc
        gchar *clean = g_strdup(hint);
        gchar *pct = strchr(clean, '%');
        if (pct) *pct = '\0';
        g_strstrip(clean);
        
        GtkWidget *desc_label = gtk_label_new(clean);
        gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
        gtk_style_context_add_class(gtk_widget_get_style_context(desc_label), "result-desc");
        gtk_box_pack_start(GTK_BOX(vbox), desc_label, FALSE, FALSE, 0);
        
        g_free(clean);
        g_free(hint);
    }
    
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    
    g_signal_connect(button, "clicked", G_CALLBACK(on_result_clicked), app->desktop_file);
    
    return button;
}

// ═══════════════════════════════════════════════════════════════════════════
// Search Logic
// ═══════════════════════════════════════════════════════════════════════════

void search_clear_results(void) {
    if (!state->results_box || !GTK_IS_CONTAINER(state->results_box)) return;
    
    GList *children = gtk_container_get_children(GTK_CONTAINER(state->results_box));
    for (GList *l = children; l != NULL; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
}

void search_perform(const gchar *query) {
    search_clear_results();
    
    if (!query || strlen(query) == 0) {
        // dropdown_hide is called in window.c
        return;
    }
    
    // Command hints
    if (g_str_has_prefix(query, "vater:") || 
        g_str_has_prefix(query, "!:") ||
        g_str_has_prefix(query, "vafile:") ||
        g_str_has_prefix(query, "g:") ||
        g_str_has_prefix(query, "s:") ||
        g_str_has_prefix(query, "ai:")) {
        
        const gchar *hint = NULL;
        if (g_str_has_prefix(query, "vater:")) hint = "⌨️ Enter = run command";
        else if (g_str_has_prefix(query, "!:")) hint = "🧮 Enter = calculate";
        else if (g_str_has_prefix(query, "vafile:")) hint = "📁 Enter = search files";
        else if (g_str_has_prefix(query, "g:")) hint = "🐙 Enter = GitHub";
        else if (g_str_has_prefix(query, "s:")) hint = "🔍 Enter = Google";
        else if (g_str_has_prefix(query, "ai:")) hint = "🤖 Enter = AI";
        
        if (state->results_box) {
            GtkWidget *label = gtk_label_new(hint);
            gtk_style_context_add_class(gtk_widget_get_style_context(label), "hint-label");
            gtk_box_pack_start(GTK_BOX(state->results_box), label, FALSE, FALSE, 0);
        }
        
        extern void dropdown_show(void);
        dropdown_show();
        return;
    }
    
    // App search
    gchar *lower_query = g_utf8_strdown(query, -1);
    int count = 0;
    
    for (GList *l = state->app_cache; l != NULL && count < 6; l = l->next) {
        AppEntry *app = (AppEntry *)l->data;
        gchar *lower_name = g_utf8_strdown(app->name, -1);
        
        if (strstr(lower_name, lower_query)) {
            if (state->results_box) {
                GtkWidget *row = create_result_row(app);
                gtk_box_pack_start(GTK_BOX(state->results_box), row, FALSE, FALSE, 0);
            }
            count++;
        }
        
        g_free(lower_name);
    }
    
    g_free(lower_query);
    
    if (count > 0) {
        extern void dropdown_show(void);
        dropdown_show();
    }
}

void search_init(void) {
    search_load_apps();
}
