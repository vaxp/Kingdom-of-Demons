#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/inotify.h>
#include <math.h>

// إعدادات المظهر
#define OSD_WIDTH 200
#define OSD_HEIGHT 100
#define FONT_SIZE 40.0
#define SHOW_DURATION_MS 1500
#define CONFIG_FILE "/etc/venom/input.json"
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

// المتغيرات العامة
GtkWidget *window = NULL;
GtkWidget *drawing_area = NULL;
guint hide_timer_id = 0;
Display *xdisplay = NULL;
int xkb_event_type = 0;
int current_group_index = -1;

// مصفوفة لتخزين اللغات
char loaded_layouts[4][10];
int loaded_layouts_count = 0;
char current_text[10] = "";

// دالة تحويل النص لأحرف كبيرة
void to_upper_str(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

// دالة قراءة اللغات من ملف JSON
void load_layouts_from_json() {
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        printf("⚠️ Warning: Could not open config file: %s\n", CONFIG_FILE);
        return;
    }

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
    printf("🔄 Config Reloaded: %d layouts found.\n", loaded_layouts_count);
}

// رسم OSD
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
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.85);
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius, radius, -M_PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI/2);
    cairo_arc(cr, x + radius, y + h - radius, radius, M_PI/2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);

    // رسم النص
    cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, FONT_SIZE);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, current_text, &extents);

    double text_x = (OSD_WIDTH - extents.width) / 2.0 - extents.x_bearing;
    double text_y = (OSD_HEIGHT - extents.height) / 2.0 - extents.y_bearing;

    cairo_move_to(cr, text_x, text_y);
    cairo_show_text(cr, current_text);
    
    return FALSE;
}

// إخفاء OSD
gboolean hide_osd(gpointer data) {
    (void)data;
    gtk_widget_hide(window);
    hide_timer_id = 0;
    return G_SOURCE_REMOVE;
}

// إظهار OSD
void show_osd(const char *text) {
    strncpy(current_text, text, sizeof(current_text) - 1);
    current_text[sizeof(current_text) - 1] = '\0';
    
    if (hide_timer_id) {
        g_source_remove(hide_timer_id);
    }
    
    gtk_widget_queue_draw(drawing_area);
    gtk_widget_show_all(window);
    
    hide_timer_id = g_timeout_add(SHOW_DURATION_MS, hide_osd, NULL);
}

// معالجة أحداث XKB
GdkFilterReturn xkb_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data) {
    (void)event; (void)data;
    XEvent *xev = (XEvent *)xevent;
    
    if (xev->type == xkb_event_type) {
        XkbEvent *xkb = (XkbEvent *)xev;
        if (xkb->any.xkb_type == XkbStateNotify) {
            if (!(xkb->state.changed & XkbGroupStateMask)) return GDK_FILTER_CONTINUE;
            if (xkb->state.group == current_group_index) return GDK_FILTER_CONTINUE;
            
            current_group_index = xkb->state.group;
            
            if (loaded_layouts_count > 0) {
                int idx = current_group_index;
                if (idx >= 0 && idx < loaded_layouts_count) {
                    show_osd(loaded_layouts[idx]);
                }
            } else {
                load_layouts_from_json();
            }
        }
    }
    
    return GDK_FILTER_CONTINUE;
}

// معالجة inotify
gboolean on_inotify(GIOChannel *source, GIOCondition cond, gpointer data) {
    (void)cond; (void)data;
    char buf[BUF_LEN];
    gsize bytes_read;
    g_io_channel_read_chars(source, buf, BUF_LEN, &bytes_read, NULL);
    if (bytes_read > 0) {
        load_layouts_from_json();
    }
    return TRUE;
}

// إعداد XKB
void setup_xkb() {
    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    
    int opcode, error, major, minor;
    if (!XkbQueryExtension(xdisplay, &opcode, &xkb_event_type, &error, &major, &minor)) {
        printf("❌ XKB not available\n");
        return;
    }
    
    XkbSelectEvents(xdisplay, XkbUseCoreKbd, XkbStateNotifyMask, XkbStateNotifyMask);
    
    XkbStateRec state;
    XkbGetState(xdisplay, XkbUseCoreKbd, &state);
    current_group_index = state.group;
    
    gdk_window_add_filter(NULL, xkb_filter, NULL);
    printf("🌐 XKB: Initial group = %d\n", current_group_index);
}

// إعداد النافذة
void setup_window() {
    window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_default_size(GTK_WINDOW(window), OSD_WIDTH, OSD_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_widget_set_app_paintable(window, TRUE);
    
    // الشفافية
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(window, visual);
    }
    
    // منطقة الرسم
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, OSD_WIDTH, OSD_HEIGHT);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_container_add(GTK_CONTAINER(window), drawing_area);
    
    // جعل النافذة click-through
    gtk_widget_realize(window);
    GdkWindow *gdk_win = gtk_widget_get_window(window);
    cairo_region_t *empty = cairo_region_create();
    gdk_window_input_shape_combine_region(gdk_win, empty, 0, 0);
    cairo_region_destroy(empty);
}

int main(int argc, char *argv[]) {
    printf("🚀 VENOM OSD: GTK Version\n");
    
    gtk_init(&argc, &argv);
    
    load_layouts_from_json();
    setup_window();
    setup_xkb();
    
    // إعداد inotify
    int inotifyFd = inotify_init();
    if (inotifyFd >= 0) {
        inotify_add_watch(inotifyFd, CONFIG_FILE, IN_CLOSE_WRITE);
        GIOChannel *channel = g_io_channel_unix_new(inotifyFd);
        g_io_add_watch(channel, G_IO_IN, on_inotify, NULL);
    }
    
    printf("✅ OSD Ready. Waiting for events...\n");
    
    gtk_main();
    
    return 0;
}