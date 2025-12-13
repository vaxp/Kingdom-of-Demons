/*
 * ═══════════════════════════════════════════════════════════════════════════
 * Venom Shortcuts Daemon v5.0 - MPRIS Edition
 * ═══════════════════════════════════════════════════════════════════════════
 * Features:
 * - Hot Reload + CSV Config + Error Handling
 * - MPRIS D-Bus Integration for Media Control
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <gio/gio.h>

#define X_GrabKey 33
#define MAX_SHORTCUTS 200
#define MAX_CMD_LEN 512

// ═══════════════════════════════════════════════════════════════════════════
// هياكل البيانات
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    KeyCode key_code;
    unsigned int modifiers;
    char command[MAX_CMD_LEN];
    char debug_name[50];
} Shortcut;

Shortcut shortcuts[MAX_SHORTCUTS];
int shortcut_count = 0;

// D-Bus connection للـ MPRIS
static GDBusConnection *session_bus = NULL;

// ═══════════════════════════════════════════════════════════════════════════
// MPRIS D-Bus Functions
// ═══════════════════════════════════════════════════════════════════════════

// البحث عن مشغل MPRIS نشط
static gchar* find_active_mpris_player() {
    if (!session_bus) return NULL;
    
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        session_bus,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus",
        "ListNames",
        NULL,
        G_VARIANT_TYPE("(as)"),
        G_DBUS_CALL_FLAGS_NONE,
        1000,
        NULL,
        &error
    );
    
    if (error) {
        g_error_free(error);
        return NULL;
    }
    
    GVariantIter *iter;
    gchar *name;
    gchar *player = NULL;
    
    g_variant_get(result, "(as)", &iter);
    while (g_variant_iter_loop(iter, "s", &name)) {
        if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
            player = g_strdup(name);
            break;
        }
    }
    
    g_variant_iter_free(iter);
    g_variant_unref(result);
    
    return player;
}

// استدعاء method على MPRIS Player
static void mpris_call(const gchar *method) {
    gchar *player = find_active_mpris_player();
    if (!player) {
        return;
    }
    
    
    GError *error = NULL;
    g_dbus_connection_call_sync(
        session_bus,
        player,
        "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player",
        method,
        NULL,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        1000,
        NULL,
        &error
    );
    
    if (error) {
        g_error_free(error);
    }
    
    g_free(player);
}

// دوال MPRIS المباشرة
void mpris_play_pause() { mpris_call("PlayPause"); }
void mpris_next()       { mpris_call("Next"); }
void mpris_previous()   { mpris_call("Previous"); }
void mpris_stop()       { mpris_call("Stop"); }

// ═══════════════════════════════════════════════════════════════════════════
// معالج الأخطاء
// ═══════════════════════════════════════════════════════════════════════════

int XVenomErrorHandler(Display *d, XErrorEvent *e) {
    if (e->error_code == BadAccess && e->request_code == X_GrabKey) {
        return 0;
    }
    char buffer[1024];
    XGetErrorText(d, e->error_code, buffer, 1024);
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

void trim(char *s) {
    char *p = s;
    int l = strlen(p);
    if (l == 0) return;
    while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
    while(*p && isspace(*p)) p++;
    memmove(s, p, l + 1);
}

const char* resolve_x11_name(char *input_name) {
    if (strcasecmp(input_name, "VolUp") == 0) return "XF86AudioRaiseVolume";
    if (strcasecmp(input_name, "VolDown") == 0) return "XF86AudioLowerVolume";
    if (strcasecmp(input_name, "Mute") == 0) return "XF86AudioMute";
    if (strcasecmp(input_name, "BriUp") == 0) return "XF86MonBrightnessUp";
    if (strcasecmp(input_name, "BriDown") == 0) return "XF86MonBrightnessDown";
    if (strcasecmp(input_name, "Play") == 0) return "XF86AudioPlay";
    if (strcasecmp(input_name, "Pause") == 0) return "XF86AudioPause";
    if (strcasecmp(input_name, "Next") == 0) return "XF86AudioNext";
    if (strcasecmp(input_name, "Prev") == 0) return "XF86AudioPrev";
    if (strcasecmp(input_name, "Stop") == 0) return "XF86AudioStop";
    if (strcasecmp(input_name, "Enter") == 0) return "Return";
    if (strcasecmp(input_name, "Esc") == 0) return "Escape";
    if (strcasecmp(input_name, "Del") == 0) return "Delete";
    if (strcasecmp(input_name, "Ins") == 0) return "Insert";
    if (strcasecmp(input_name, "PgUp") == 0) return "Page_Up";
    if (strcasecmp(input_name, "PgDn") == 0) return "Page_Down";
    if (strcasecmp(input_name, "Space") == 0) return "space";
    return input_name;
}

unsigned int parse_modifier(char *mod_str) {
    trim(mod_str);
    unsigned int mask = 0;
    if (strcasestr(mod_str, "Ctrl")) mask |= ControlMask;
    if (strcasestr(mod_str, "Alt")) mask |= Mod1Mask;
    if (strcasestr(mod_str, "Shift")) mask |= ShiftMask;
    if (strcasestr(mod_str, "Super")) mask |= Mod4Mask;
    return mask;
}

// ═══════════════════════════════════════════════════════════════════════════
// تحميل الإعدادات
// ═══════════════════════════════════════════════════════════════════════════

void load_config(Display *display) {
    FILE *file = fopen("/etc/venom/shortcuts.csv", "r");
    if (!file) {
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file) && shortcut_count < MAX_SHORTCUTS) {
        if (strlen(line) < 3 || line[0] == '#') continue;
        
        char *mod_str = strtok(line, ",");
        char *key_raw = strtok(NULL, ",");
        char *cmd_str = strtok(NULL, ",");
        
        if (mod_str && key_raw && cmd_str) {
            trim(key_raw); trim(cmd_str);
            cmd_str[strcspn(cmd_str, "\r\n")] = 0;
            
            Shortcut *s = &shortcuts[shortcut_count];
            strcpy(s->command, cmd_str);
            strcpy(s->debug_name, key_raw);
            s->modifiers = parse_modifier(mod_str);
            
            const char *x11_name = resolve_x11_name(key_raw);
            KeySym sym = XStringToKeysym(x11_name);
            
            if (sym != NoSymbol) {
                s->key_code = XKeysymToKeycode(display, sym);
                if (s->key_code != 0) {
                    shortcut_count++;
                }
            }
        }
    }
    fclose(file);
}

void reload_signal_handler(int signum) {
    if (signum == SIGHUP) {
        extern char **environ;
        execve("/proc/self/exe", (char *[]){"venom_shortcuts", NULL}, environ);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// تنفيذ الأوامر مع دعم MPRIS
// ═══════════════════════════════════════════════════════════════════════════

void execute_command(const char *command) {
    // أوامر MPRIS المباشرة
    if (strcasecmp(command, "@mpris:playpause") == 0) {
        mpris_play_pause();
        return;
    }
    if (strcasecmp(command, "@mpris:next") == 0) {
        mpris_next();
        return;
    }
    if (strcasecmp(command, "@mpris:previous") == 0 || 
        strcasecmp(command, "@mpris:prev") == 0) {
        mpris_previous();
        return;
    }
    if (strcasecmp(command, "@mpris:stop") == 0) {
        mpris_stop();
        return;
    }
    
    // أمر shell عادي
    if (fork() == 0) {
        execl("/bin/sh", "sh", "-c", command, (char *)0);
        exit(0);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    
    signal(SIGHUP, reload_signal_handler);
    
    // Initialize D-Bus
    GError *error = NULL;
    session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_error_free(error);
    } else {
    }
    
    // Initialize X11
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        return 1;
    }
    
    XSetErrorHandler(XVenomErrorHandler);
    Window root = DefaultRootWindow(display);
    
    load_config(display);
    
    // Grab keys
    unsigned int locks[] = {0, Mod2Mask, LockMask, Mod2Mask | LockMask};
    
    
    for (int i = 0; i < shortcut_count; i++) {
        for (int j = 0; j < 4; j++) {
            XGrabKey(display, shortcuts[i].key_code, 
                     shortcuts[i].modifiers | locks[j], 
                     root, True, GrabModeAsync, GrabModeAsync);
        }
    }
    
    XSync(display, False);
    
    // Event loop
    XEvent ev;
    while (1) {
        XNextEvent(display, &ev);
        if (ev.type == KeyPress) {
            for (int i = 0; i < shortcut_count; i++) {
                if (ev.xkey.keycode == shortcuts[i].key_code) {
                    unsigned int clean_mask = ev.xkey.state & 
                        (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
                    if (clean_mask == shortcuts[i].modifiers) {
                        execute_command(shortcuts[i].command);
                        break;
                    }
                }
            }
        }
    }
    
    if (session_bus) g_object_unref(session_bus);
    XCloseDisplay(display);
    return 0;
}