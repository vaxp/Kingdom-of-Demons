/*
 * ═══════════════════════════════════════════════════════════════════════════
 * 🖼️ Venom Basilisk - Window Module (With Dropdown Results)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include "window.h"
#include "search.h"
#include "commands.h"

extern BasiliskState *state;

static GtkWidget *dropdown_window = NULL;

// ═══════════════════════════════════════════════════════════════════════════
// CSS
// ═══════════════════════════════════════════════════════════════════════════

static const gchar *css_data = 
    "* { padding: 0; margin: 0; }"
    "window, .background { background-color: transparent; }"
    "#basilisk-main { background: rgba(30,30,35,0.95); border-radius: 8px; padding: 4px; }"
    "#search-entry { background: transparent; border: none; color: white; font-size: 14px; }"
    "#dropdown { background: rgba(25,25,30,0.98); border-radius: 8px; border: 1px solid rgba(255,255,255,0.1); }"
    ".result-row { padding: 8px 12px; background: transparent; border: none; }"
    ".result-row:hover { background: rgba(0,212,255,0.2); }"
    ".result-name { font-size: 13px; color: white; }"
    ".hint-label { color: #888; font-size: 12px; padding: 8px; }";

void window_apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

// ═══════════════════════════════════════════════════════════════════════════
// Dropdown Window
// ═══════════════════════════════════════════════════════════════════════════

void dropdown_show(void) {
    if (!dropdown_window) {
        dropdown_window = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_widget_set_name(dropdown_window, "dropdown");
        gtk_window_set_decorated(GTK_WINDOW(dropdown_window), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dropdown_window), TRUE);
        gtk_window_set_keep_above(GTK_WINDOW(dropdown_window), TRUE);
        
        GdkScreen *screen = gtk_widget_get_screen(dropdown_window);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual) gtk_widget_set_visual(dropdown_window, visual);
        gtk_widget_set_app_paintable(dropdown_window, TRUE);
        
        // Results box directly (no scroll) - fixed width 300px
        state->results_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(state->results_box, 300, -1);
        gtk_container_add(GTK_CONTAINER(dropdown_window), state->results_box);
        gtk_widget_set_size_request(dropdown_window, 300, -1);
    }
    
    // Position below search entry - same width as search (300px)
    gint x, y, h;
    gtk_window_get_position(GTK_WINDOW(state->window), &x, &y);
    gtk_window_get_size(GTK_WINDOW(state->window), NULL, &h);
    gtk_window_move(GTK_WINDOW(dropdown_window), x, y + h + 4);
    gtk_widget_set_size_request(dropdown_window, 300, -1);
    
    gtk_widget_show_all(dropdown_window);
}

void dropdown_hide(void) {
    if (dropdown_window) {
        gtk_widget_hide(dropdown_window);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Event Handlers
// ═══════════════════════════════════════════════════════════════════════════

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    (void)widget; (void)data;
    
    if (event->keyval == GDK_KEY_Escape) {
        dropdown_hide();
        window_hide();
        return TRUE;
    }
    
    if (event->keyval == GDK_KEY_Return) {
        const gchar *text = gtk_entry_get_text(GTK_ENTRY(state->search_entry));
        if (commands_check_prefix(text)) {
            commands_execute(text);
            return TRUE;
        }
    }
    
    return FALSE;
}

static void on_search_changed(GtkEditable *editable, gpointer data) {
    (void)data;
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(editable));
    
    if (text && strlen(text) >= 1) {
        search_perform(text);
    } else {
        search_clear_results();
        dropdown_hide();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Window Creation
// ═══════════════════════════════════════════════════════════════════════════

void window_init(void) {
    window_apply_css();
    
    // Main Window - Just the search bar
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "Basilisk");
    gtk_window_set_decorated(GTK_WINDOW(state->window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(state->window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(state->window), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(state->window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_keep_above(GTK_WINDOW(state->window), TRUE);  // Always on top
    gtk_widget_set_app_paintable(state->window, TRUE);
    
    GdkScreen *screen = gtk_widget_get_screen(state->window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) gtk_widget_set_visual(state->window, visual);
    
    // Main Box
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(main_box, "basilisk-main");
    gtk_container_add(GTK_CONTAINER(state->window), main_box);
    
    // Search Entry ONLY - compact size
    state->search_entry = gtk_entry_new();
    gtk_widget_set_name(state->search_entry, "search-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->search_entry), "Search...");
    gtk_widget_set_size_request(state->search_entry, 335, 28);
    gtk_box_pack_start(GTK_BOX(main_box), state->search_entry, FALSE, FALSE, 0);
    
    // Set window size to match entry only
    gtk_window_set_default_size(GTK_WINDOW(state->window), 300, -1);
    gtk_window_set_resizable(GTK_WINDOW(state->window), FALSE);
    
    // Signals
    g_signal_connect(state->window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(state->search_entry, "changed", G_CALLBACK(on_search_changed), NULL);
    
    // Prevent window from being destroyed - just hide it
    g_signal_connect(state->window, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    
    // IMPORTANT: Realize the window to keep GTK running
    gtk_widget_realize(state->window);
    
    state->visible = FALSE;
    state->results_scroll = NULL;
    state->results_box = NULL;
}

void window_show(void) {
    if (!state->visible) {
        gtk_entry_set_text(GTK_ENTRY(state->search_entry), "");
        search_clear_results();
        dropdown_hide();
        
        // Center on screen
        GdkScreen *screen = gdk_screen_get_default();
        gint sw = gdk_screen_get_width(screen);
        gint sh = gdk_screen_get_height(screen);
        gint x = (sw - 300) / 2;
        gint y = sh / 3;
        
        gtk_window_move(GTK_WINDOW(state->window), x, y);
        gtk_widget_show_all(state->window);
        gtk_window_present(GTK_WINDOW(state->window));
        gtk_widget_grab_focus(state->search_entry);
        state->visible = TRUE;
    }
}

void window_hide(void) {
    if (state->visible) {
        dropdown_hide();
        gtk_widget_hide(state->window);
        state->visible = FALSE;
    }
}

void window_toggle(void) {
    if (state->visible) {
        window_hide();
    } else {
        window_show();
    }
}
