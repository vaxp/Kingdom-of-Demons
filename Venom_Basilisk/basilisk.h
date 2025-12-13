/*
 * ═══════════════════════════════════════════════════════════════════════════
 * 🔍 Venom Basilisk - Global Header
 * ═══════════════════════════════════════════════════════════════════════════
 */

#ifndef BASILISK_H
#define BASILISK_H

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

// D-Bus
#define DBUS_NAME "org.venom.Basilisk"
#define DBUS_PATH "/org/venom/Basilisk"
#define DBUS_INTERFACE "org.venom.Basilisk"

// Window
#define WINDOW_WIDTH 350
#define WINDOW_HEIGHT -1
#define RESULTS_HEIGHT 200

// App Entry
typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
    gchar *desktop_file;
} AppEntry;

// Global state
typedef struct {
    GtkWidget *window;
    GtkWidget *search_entry;
    GtkWidget *results_box;
    GtkWidget *results_scroll;
    GDBusConnection *dbus_conn;
    guint dbus_owner_id;
    GList *app_cache;
    gboolean visible;
} BasiliskState;

extern BasiliskState *state;

#endif
