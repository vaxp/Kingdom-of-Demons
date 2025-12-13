#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define CONFIG_DIR "/etc/venom"
#define CONFIG_FILE "/etc/venom/input.json"

// متغير عالمي لحفظ اللغات الحالية
char current_layouts[256] = "us"; 

// --- دوال مساعدة للتعامل مع النظام والملفات ---

// دالة لتطبيق الكيبورد في X11
void apply_keyboard_settings(const char *layouts) {
    char command[512];
    snprintf(command, sizeof(command), 
        "setxkbmap -rules evdev -model pc105 -layout '%s' -option grp:shift_alt_toggle,grp:win_space_toggle", 
        layouts);
    system(command);
}

// دالة حفظ الإعدادات في ملف JSON
void save_config(const char *layouts) {
    // التأكد من وجود المجلد
    struct stat st = {0};
    if (stat(CONFIG_DIR, &st) == -1) {
        system("mkdir -p " CONFIG_DIR);
    }

    FILE *f = fopen(CONFIG_FILE, "w");
    if (f == NULL) {
        return;
    }
    
    // كتابة JSON بسيط يدوياً للحفاظ على نظافة الكود دون مكاتب خارجية
    fprintf(f, "{\n  \"model\": \"pc105\",\n  \"layouts\": \"%s\",\n  \"options\": \"grp:shift_alt_toggle,grp:win_space_toggle\"\n}\n", layouts);
    fclose(f);
}

// دالة تحميل الإعدادات (Parser بسيط جداً)
void load_config() {
    FILE *f = fopen(CONFIG_FILE, "r");
    if (f == NULL) {
        apply_keyboard_settings("us");
        return;
    }

    char buffer[1024];
    size_t len = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[len] = '\0';
    fclose(f);

    // استخراج قيمة layouts يدوياً (بدل تعقيد المكتبات)
    char *key = strstr(buffer, "\"layouts\":");
    if (key) {
        char *start = strchr(key, ':');
        if (start) {
            start = strchr(start, '"'); // بداية القيمة
            if (start) {
                start++; // تخطي علامة التنصيص
                char *end = strchr(start, '"'); // نهاية القيمة
                if (end) {
                    *end = '\0'; // قطع النص
                    strncpy(current_layouts, start, sizeof(current_layouts));
                    apply_keyboard_settings(current_layouts);
                    return;
                }
            }
        }
    }
    // في حال فشل القراءة
    apply_keyboard_settings("us");
}

// --- التعامل مع D-Bus ---

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.venom.Input'>"
  "    <method name='SetLayouts'>"
  "      <arg type='s' name='layouts' direction='in'/>"
  "      <arg type='b' name='success' direction='out'/>"
  "    </method>"
  "    <method name='GetLayouts'>"
  "      <arg type='s' name='layouts' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static void handle_method_call(GDBusConnection       *connection,
                               const gchar           *sender,
                               const gchar           *object_path,
                               const gchar           *interface_name,
                               const gchar           *method_name,
                               GVariant              *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer               user_data) {
    
    if (g_strcmp0(method_name, "SetLayouts") == 0) {
        const gchar *layouts_string;
        g_variant_get(parameters, "(&s)", &layouts_string);

        // 1. تحديث المتغير
        strncpy(current_layouts, layouts_string, sizeof(current_layouts));
        
        // 2. تطبيق الإعدادات فوراً
        apply_keyboard_settings(current_layouts);
        
        // 3. حفظ في ملف JSON
        save_config(current_layouts);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", TRUE));
    } 
    else if (g_strcmp0(method_name, "GetLayouts") == 0) {
        // فلاتر يطلب معرفة اللغات الحالية
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", current_layouts));
    }
}

static const GDBusInterfaceVTable interface_vtable = {
  handle_method_call, NULL, NULL
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    GError *error = NULL;
    GDBusNodeInfo *node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    g_dbus_connection_register_object(connection, "/org/venom/Input", node_info->interfaces[0], &interface_vtable, NULL, NULL, &error);
    load_config();
}

int main(int argc, char *argv[]) {
    // بما أننا نكتب في /etc يجب تشغيل العفريت كـ Root أو إعطاء صلاحيات للمجلد
    GMainLoop *loop;
    g_bus_own_name(G_BUS_TYPE_SESSION, "org.venom.Input", G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, NULL, NULL, NULL, NULL);
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    return 0;
}