#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/inotify.h>
#include <math.h>

// PulseAudio
#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

// إعدادات المظهر
#define OSD_WIDTH 200
#define OSD_HEIGHT 200
#define FONT_SIZE 40.0
#define SHOW_DURATION_MS 1500
#define CONFIG_FILE "/etc/venom/input.json"
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

// أنواع OSD
typedef enum {
    OSD_KEYBOARD,
    OSD_VOLUME,
    OSD_BRIGHTNESS
} OsdType;

// المتغيرات العامة
GtkWidget *window = NULL;
GtkWidget *drawing_area = NULL;
guint hide_timer_id = 0;
Display *xdisplay = NULL;
int xkb_event_type = 0;
int current_group_index = -1;

// حالة OSD الحالية
OsdType current_osd_type = OSD_KEYBOARD;
char current_text[10] = "";
int current_volume = -1;
int is_muted = -1;
int current_brightness = 0;

// PulseAudio
pa_glib_mainloop *my_pa_mainloop = NULL;
pa_context *pa_ctx = NULL;
uint32_t default_sink_index = PA_INVALID_INDEX;

// XRandR المتغيرات
int xrandr_event_base = 0;

// مصفوفة لتخزين اللغات
char loaded_layouts[4][10];
int loaded_layouts_count = 0;

// تعريف الدوال المسبق
void show_osd();
void to_upper_str(char *str);
void load_layouts_from_json();

// ----------------------------------------------------------------------
// PulseAudio Handlers
// ----------------------------------------------------------------------
void sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;

    pa_cvolume cv = i->volume;
    int vol = (pa_cvolume_avg(&cv) * 100) / PA_VOLUME_NORM;
    if (vol > 100) vol = 100;
    
    int changed = 0;
    if (current_volume != -1 && (current_volume != vol || is_muted != i->mute)) {
        changed = 1;
    }
    
    current_volume = vol;
    is_muted = i->mute;
    
    if (changed) {
        current_osd_type = OSD_VOLUME;
        printf("[PA] Sink volume updated: %d%%, muted: %d\n", vol, is_muted);
        show_osd();
    }
}

void server_info_cb(pa_context *c, const pa_server_info *i, void *userdata) {
    if (!i) return;
    printf("[PA] Default sink is %s\n", i->default_sink_name);
    pa_context_get_sink_info_by_name(c, i->default_sink_name, sink_info_cb, NULL);
}

void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_CHANGE &&
        (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK) {
        printf("[PA] Sink event received, fetching info...\n");
        pa_context_get_server_info(c, server_info_cb, NULL);
    }
}

void context_state_cb(pa_context *c, void *userdata) {
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
            printf("[PA] Context Ready\n");
            pa_context_get_server_info(c, server_info_cb, NULL);
            pa_context_set_subscribe_callback(c, subscribe_cb, NULL);
            pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK, NULL, NULL);
            break;
        case PA_CONTEXT_FAILED:
            printf("[PA] Context Failed\n");
            break;
        case PA_CONTEXT_TERMINATED:
            printf("[PA] Context Terminated\n");
            break;
        default:
            printf("[PA] Context State Changed: %d\n", pa_context_get_state(c));
            break;
    }
}

void setup_pulseaudio() {
    my_pa_mainloop = pa_glib_mainloop_new(NULL);
    pa_mainloop_api *api = pa_glib_mainloop_get_api(my_pa_mainloop);
    pa_ctx = pa_context_new(api, "Venom OSD");
    
    pa_context_set_state_callback(pa_ctx, context_state_cb, NULL);
    pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
}

// ----------------------------------------------------------------------
// Utils
// ----------------------------------------------------------------------
void to_upper_str(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

void load_layouts_from_json() {
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) return;

    char buffer[1024];
    size_t len = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[len] = '\0';
    fclose(f);

    memset(loaded_layouts, 0, sizeof(loaded_layouts));
    loaded_layouts_count = 0;

    char *key = strstr(buffer, "\"layouts\":");
    if (key) {
        char *start = strchr(key, ':');
        if (start) {
            start = strchr(start, '"');
            if (start) {
                start++;
                char *end = strchr(start, '"');
                if (end) {
                    *end = '\0';
                    char *token = strtok(start, ",");
                    while (token != NULL && loaded_layouts_count < 4) {
                        strncpy(loaded_layouts[loaded_layouts_count], token, 9);
                        to_upper_str(loaded_layouts[loaded_layouts_count]);
                        
                        if (strcmp(loaded_layouts[loaded_layouts_count], "ARA") == 0)
                            strcpy(loaded_layouts[loaded_layouts_count], "AR");
                        if (strcmp(loaded_layouts[loaded_layouts_count], "USA") == 0)
                            strcpy(loaded_layouts[loaded_layouts_count], "US");

                        loaded_layouts_count++;
                        token = strtok(NULL, ",");
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------
// Drawing
// ----------------------------------------------------------------------
void draw_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double radius) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius, radius, -M_PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI/2);
    cairo_arc(cr, x + radius, y + h - radius, radius, M_PI/2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
}

void draw_icon_speaker(cairo_t *cr, double cx, double cy, double size, int muted) {
    // رسم مكبر صوت بسيط
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 4.0);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    
    // هيكل المكبر
    cairo_move_to(cr, cx - size*0.4, cy - size*0.2);
    cairo_line_to(cr, cx - size*0.1, cy - size*0.2);
    cairo_line_to(cr, cx + size*0.2, cy - size*0.4);
    cairo_line_to(cr, cx + size*0.2, cy + size*0.4);
    cairo_line_to(cr, cx - size*0.1, cy + size*0.2);
    cairo_line_to(cr, cx - size*0.4, cy + size*0.2);
    cairo_close_path(cr);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);

    if (muted) {
        cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);
        cairo_move_to(cr, cx + size*0.1, cy - size*0.2);
        cairo_line_to(cr, cx + size*0.5, cy + size*0.2);
        cairo_move_to(cr, cx + size*0.5, cy - size*0.2);
        cairo_line_to(cr, cx + size*0.1, cy + size*0.2);
        cairo_stroke(cr);
    } else {
        // رسم أمواج الصوت
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_arc(cr, cx + size*0.2, cy, size*0.2, -M_PI/4, M_PI/4);
        cairo_stroke(cr);
        cairo_arc(cr, cx + size*0.2, cy, size*0.4, -M_PI/4, M_PI/4);
        cairo_stroke(cr);
    }
}

void draw_icon_sun(cairo_t *cr, double cx, double cy, double size) {
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 4.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    // دائرة الشمس
    cairo_arc(cr, cx, cy, size*0.25, 0, 2*M_PI);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
    
    // أشعة الشمس
    for (int i = 0; i < 8; ++i) {
        double angle = i * M_PI / 4.0;
        cairo_move_to(cr, cx + cos(angle) * size * 0.35, cy + sin(angle) * size * 0.35);
        cairo_line_to(cr, cx + cos(angle) * size * 0.5, cy + sin(angle) * size * 0.5);
        cairo_stroke(cr);
    }
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    (void)widget; (void)data;
    
    // خلفية شفافة
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    double x = 10, y = 10, w = OSD_WIDTH - 20, h = OSD_HEIGHT - 20;
    double radius = 20.0;

    // رسم الخلفية
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.0);
    draw_rounded_rect(cr, x, y, w, h, radius);
    cairo_fill(cr);

    if (current_osd_type == OSD_KEYBOARD) {
        cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, FONT_SIZE);

        cairo_text_extents_t extents;
        cairo_text_extents(cr, current_text, &extents);

        double text_x = (OSD_WIDTH - extents.width) / 2.0 - extents.x_bearing;
        double text_y = (OSD_HEIGHT - extents.height) / 2.0 - extents.y_bearing;

        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, current_text);
    } 
    else if (current_osd_type == OSD_VOLUME || current_osd_type == OSD_BRIGHTNESS) {
        int percentage = (current_osd_type == OSD_VOLUME) ? current_volume : current_brightness;
        
        // رسم الأيقونة في النصف العلوي
        double icon_cx = OSD_WIDTH / 2.0;
        double icon_cy = OSD_HEIGHT / 2.0 - 20;
        
        if (current_osd_type == OSD_VOLUME) {
            draw_icon_speaker(cr, icon_cx, icon_cy, 60.0, is_muted);
        } else {
            draw_icon_sun(cr, icon_cx, icon_cy, 60.0);
        }

        // رسم شريط التقدم في النصف السفلي
        double bar_w = w * 0.8;
        double bar_h = 10.0;
        double bar_x = (OSD_WIDTH - bar_w) / 2.0;
        double bar_y = y + h - 30.0;
        
        // خلفية الشريط
        draw_rounded_rect(cr, bar_x, bar_y, bar_w, bar_h, bar_h/2.0);
        cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
        cairo_fill(cr);
        
        // مستوى الشريط
        if (percentage > 0) {
            double fill_w = bar_w * (percentage / 100.0);
            draw_rounded_rect(cr, bar_x, bar_y, fill_w, bar_h, bar_h/2.0);
            if (current_osd_type == OSD_VOLUME && is_muted) {
                cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
            } else {
                cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
            }
            cairo_fill(cr);
        }
        
        // رسم النسبة المئوية
        char val_str[16];
        snprintf(val_str, sizeof(val_str), "%d%%", percentage);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 16.0);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, val_str, &extents);
        cairo_move_to(cr, (OSD_WIDTH - extents.width)/2.0 - extents.x_bearing, bar_y - 10.0);
        cairo_show_text(cr, val_str);
    }
    
    return FALSE;
}

gboolean hide_osd_cb(gpointer data) {
    (void)data;
    gtk_widget_hide(window);
    hide_timer_id = 0;
    return G_SOURCE_REMOVE;
}

void show_osd() {
    printf("[OSD] Showing OSD Type: %d\n", current_osd_type);
    if (hide_timer_id) {
        g_source_remove(hide_timer_id);
    }
    
    // Resize for volume/brightness since they need more height
    if (current_osd_type == OSD_KEYBOARD) {
        gtk_window_resize(GTK_WINDOW(window), 200, 100);
        gtk_widget_set_size_request(drawing_area, 200, 100);
    } else {
        gtk_window_resize(GTK_WINDOW(window), 200, 200);
        gtk_widget_set_size_request(drawing_area, 200, 200);
    }
    
    gtk_widget_queue_draw(drawing_area);
    gtk_widget_show_all(window);
    
    hide_timer_id = g_timeout_add(SHOW_DURATION_MS, hide_osd_cb, NULL);
}

// ----------------------------------------------------------------------
// Events & Hooks
// ----------------------------------------------------------------------
GdkFilterReturn x_event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data) {
    (void)event; (void)data;
    XEvent *xev = (XEvent *)xevent;
    
    // XKB Keyboard Layout Event
    if (xev->type == xkb_event_type) {
        XkbEvent *xkb = (XkbEvent *)xev;
        if (xkb->any.xkb_type == XkbStateNotify) {
            if (!(xkb->state.changed & XkbGroupStateMask)) return GDK_FILTER_CONTINUE;
            if (xkb->state.group == current_group_index) return GDK_FILTER_CONTINUE;
            
            current_group_index = xkb->state.group;
            
            if (loaded_layouts_count > 0) {
                int idx = current_group_index;
                if (idx >= 0 && idx < loaded_layouts_count) {
                    strncpy(current_text, loaded_layouts[idx], sizeof(current_text)-1);
                    current_osd_type = OSD_KEYBOARD;
                    show_osd();
                }
            } else {
                load_layouts_from_json();
            }
        }
    }
    // XRandR Brightness Event (Property Change)
    else if (xev->type == xrandr_event_base + RRNotify) {
        XRRNotifyEvent *rrn = (XRRNotifyEvent *)xev;
        if (rrn->subtype == RRNotify_OutputProperty) {
            // تجاهل التحديثات التي لم تتغير (بحاجة للقراءة، سنستخدم inotify للسطوع ليكون أكثر موثوقية)
        }
    }
    
    return GDK_FILTER_CONTINUE;
}

gboolean on_inotify(GIOChannel *source, GIOCondition cond, gpointer data) {
    (void)cond; (void)data;
    char buf[BUF_LEN];
    gsize bytes_read;
    g_io_channel_read_chars(source, buf, BUF_LEN, &bytes_read, NULL);
    
    if (bytes_read > 0) {
        struct inotify_event *event = (struct inotify_event *)buf;
        if (event->mask & IN_CLOSE_WRITE) {
            if (strcmp(event->name, "input.json") == 0 || event->len == 0) {
                load_layouts_from_json();
            }
        }
        // Check for brightness independently of IN_CLOSE_WRITE because sysfs sends IN_MODIFY
        if ((event->mask & IN_MODIFY) && (strcmp(event->name, "brightness") == 0 || event->len == 0)) {
            // Read brightness
            printf("[Brightness] Modified event received\n");
            FILE *f = fopen("/sys/class/backlight/amd_backlight/brightness", "r");
            FILE *fm = fopen("/sys/class/backlight/amd_backlight/max_brightness", "r");
            if (!f) f = fopen("/sys/class/backlight/intel_backlight/brightness", "r");
            if (!fm) fm = fopen("/sys/class/backlight/intel_backlight/max_brightness", "r");
            
            if (f && fm) {
                int b = 0, max_b = 1;
                if (fscanf(f, "%d", &b) == 1 && fscanf(fm, "%d", &max_b) == 1) {
                    current_brightness = (b * 100) / max_b;
                    current_osd_type = OSD_BRIGHTNESS;
                    printf("[Brightness] Level %d%%\n", current_brightness);
                    show_osd();
                }
                if (f) fclose(f);
                if (fm) fclose(fm);
            } else {
                printf("[Brightness] Could not open sysfs brightness files\n");
            }
        }
    }
    return TRUE;
}

// ----------------------------------------------------------------------
// Setups
// ----------------------------------------------------------------------
void setup_xkb_and_xrandr() {
    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    
    int opcode, error, major, minor;
    if (XkbQueryExtension(xdisplay, &opcode, &xkb_event_type, &error, &major, &minor)) {
        XkbSelectEvents(xdisplay, XkbUseCoreKbd, XkbStateNotifyMask, XkbStateNotifyMask);
        XkbStateRec state;
        XkbGetState(xdisplay, XkbUseCoreKbd, &state);
        current_group_index = state.group;
    }

    // XRandr setup (optional, if we use inotify for backlight, this is just secondary)
    int rr_error_base;
    if (XRRQueryExtension(xdisplay, &xrandr_event_base, &rr_error_base)) {
        Window root = DefaultRootWindow(xdisplay);
        XRRSelectInput(xdisplay, root, RROutputPropertyNotifyMask);
    }
    
    gdk_window_add_filter(NULL, x_event_filter, NULL);
}

void setup_window() {
    window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_default_size(GTK_WINDOW(window), OSD_WIDTH, OSD_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_widget_set_app_paintable(window, TRUE);
    
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(window, visual);
    }
    
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, OSD_WIDTH, OSD_HEIGHT);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_container_add(GTK_CONTAINER(window), drawing_area);
    
    gtk_widget_realize(window);
    GdkWindow *gdk_win = gtk_widget_get_window(window);
    cairo_region_t *empty = cairo_region_create();
    gdk_window_input_shape_combine_region(gdk_win, empty, 0, 0);
    cairo_region_destroy(empty);
}

// ----------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    load_layouts_from_json();
    setup_window();
    setup_pulseaudio();
    setup_xkb_and_xrandr();
    
    int inotifyFd = inotify_init();
    if (inotifyFd >= 0) {
        // Watch Keyboard layouts config
        inotify_add_watch(inotifyFd, "/etc/venom", IN_CLOSE_WRITE | IN_MOVED_TO);
        
        // Find backlight dir and watch it (amd_backlight, amdgpu_bl0, etc)
        // For simplicity, we assume amd_backlight or similar. A real implementation
        // might loop through /sys/class/backlight/*/.
        inotify_add_watch(inotifyFd, "/sys/class/backlight/amd_backlight", IN_MODIFY);
        inotify_add_watch(inotifyFd, "/sys/class/backlight/amdgpu_bl0", IN_MODIFY);
        inotify_add_watch(inotifyFd, "/sys/class/backlight/amdgpu_bl1", IN_MODIFY);
        inotify_add_watch(inotifyFd, "/sys/class/backlight/acpi_video0", IN_MODIFY);

        GIOChannel *channel = g_io_channel_unix_new(inotifyFd);
        g_io_add_watch(channel, G_IO_IN, on_inotify, NULL);
    }
    
    gtk_main();
    
    if (pa_ctx) {
        pa_context_disconnect(pa_ctx);
        pa_context_unref(pa_ctx);
    }
    if (my_pa_mainloop) {
        pa_glib_mainloop_free(my_pa_mainloop);
    }

    return 0;
}
