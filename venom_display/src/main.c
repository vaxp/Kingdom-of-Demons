#include "venom_display.h"
#include "display.h"
#include "dbus_service.h"
#include <signal.h>

// ═══════════════════════════════════════════════════════════════════════════
// 🖥️ Venom Display Daemon - نقطة الدخول
// ═══════════════════════════════════════════════════════════════════════════

DisplayState display_state = {0};
GMainLoop *main_loop = NULL;

static void on_signal(int sig) {
    (void)sig;
    if (main_loop) g_main_loop_quit(main_loop);
}

static void on_bus_acquired(GDBusConnection *connection,
                            const gchar *name,
                            gpointer user_data) {
    (void)name;
    (void)user_data;
    
    display_state.session_conn = connection;
    
    GDBusNodeInfo *introspection = g_dbus_node_info_new_for_xml(
        dbus_get_introspection_xml(), NULL);
    
    GError *error = NULL;
    g_dbus_connection_register_object(connection,
                                       DBUS_OBJECT_PATH,
                                       introspection->interfaces[0],
                                       &dbus_interface_vtable,
                                       NULL, NULL, &error);
    
    if (error) {
        g_error_free(error);
    }
    
    g_dbus_node_info_unref(introspection);
}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar *name,
                             gpointer user_data) {
    (void)connection;
    (void)name;
    (void)user_data;
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar *name,
                         gpointer user_data) {
    (void)connection;
    (void)name;
    (void)user_data;
    if (main_loop) g_main_loop_quit(main_loop);
}

int main(void) {
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    
    // تهيئة XRandR
    if (!display_init()) {
        return 1;
    }
    
    // تنظيف قائمة الشاشات
    GList *displays = display_get_all();
    for (GList *l = displays; l; l = l->next) {
        display_info_free(l->data);
    }
    g_list_free(displays);
    
    // استعادة الإعدادات المحفوظة
    display_persist_restore_all();
    
    // تسجيل خدمة D-Bus
    guint owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                                     DBUS_SERVICE_NAME,
                                     G_BUS_NAME_OWNER_FLAGS_NONE,
                                     on_bus_acquired,
                                     on_name_acquired,
                                     on_name_lost,
                                     NULL, NULL);
    
    // تشغيل الحلقة الرئيسية
    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);
    
    // تنظيف
    g_bus_unown_name(owner_id);
    g_main_loop_unref(main_loop);
    display_cleanup();
    
    return 0;
}
