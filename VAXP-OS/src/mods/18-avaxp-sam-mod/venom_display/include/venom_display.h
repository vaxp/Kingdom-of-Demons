#ifndef VENOM_DISPLAY_H
#define VENOM_DISPLAY_H

#include <glib.h>
#include <gio/gio.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🖥️ Venom Display Daemon - الحالة العامة
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    Display *x_display;
    Window root_window;
    int screen;
    GDBusConnection *session_conn;
} DisplayState;

extern DisplayState display_state;
extern GMainLoop *main_loop;

#endif
