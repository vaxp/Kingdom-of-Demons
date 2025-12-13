#include "dbus_service.h"
#include "display.h"
#include "venom_display.h"
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// 📄 D-Bus Introspection XML
// ═══════════════════════════════════════════════════════════════════════════

const gchar* dbus_get_introspection_xml(void) {
    return
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' "
    "'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    "  <interface name='org.venom.Display'>"
    // معلومات الشاشات
    "    <method name='GetDisplays'>"
    "      <arg type='a(siidbbiix)' name='displays' direction='out'/>"
    "    </method>"
    "    <method name='GetPrimaryDisplay'>"
    "      <arg type='(siidbbiix)' name='display' direction='out'/>"
    "    </method>"
    "    <method name='GetDisplayInfo'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='(siidbbiix)' name='display' direction='out'/>"
    "    </method>"
    "    <method name='GetModes'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='a(iid)' name='modes' direction='out'/>"
    "    </method>"
    // تغيير الإعدادات الأساسية
    "    <method name='SetResolution'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='width' direction='in'/>"
    "      <arg type='i' name='height' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetRefreshRate'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='d' name='rate' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetMode'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='width' direction='in'/>"
    "      <arg type='i' name='height' direction='in'/>"
    "      <arg type='d' name='rate' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetPosition'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='SetPrimary'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // التدوير
    "    <method name='GetRotation'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='rotation' direction='out'/>"
    "    </method>"
    "    <method name='SetRotation'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='i' name='rotation' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // تشغيل/إيقاف
    "    <method name='EnableOutput'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='DisableOutput'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // المطابقة
    "    <method name='SetMirror'>"
    "      <arg type='s' name='source' direction='in'/>"
    "      <arg type='s' name='target' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='DisableMirror'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // التكبير
    "    <method name='GetScale'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='d' name='scale' direction='out'/>"
    "    </method>"
    "    <method name='SetScale'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='d' name='scale' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // Night Light
    "    <method name='GetNightLight'>"
    "      <arg type='(bi)' name='settings' direction='out'/>"
    "    </method>"
    "    <method name='SetNightLight'>"
    "      <arg type='b' name='enabled' direction='in'/>"
    "      <arg type='i' name='temperature' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // البروفايلات
    "    <method name='SaveProfile'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='LoadProfile'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='GetProfiles'>"
    "      <arg type='as' name='profiles' direction='out'/>"
    "    </method>"
    "    <method name='DeleteProfile'>"
    "      <arg type='s' name='name' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    // الإشارات
    "    <signal name='DisplayChanged'>"
    "      <arg type='s' name='name'/>"
    "    </signal>"
    "    <signal name='NightLightChanged'>"
    "      <arg type='b' name='enabled'/>"
    "      <arg type='i' name='temperature'/>"
    "    </signal>"
    "  </interface>"
    "</node>";
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 معالج طرق D-Bus
// ═══════════════════════════════════════════════════════════════════════════

static GVariant* display_info_to_variant(DisplayInfo *info) {
    return g_variant_new("(siidbbiix)",
        info->name,
        info->width,
        info->height,
        info->refresh_rate,
        info->is_connected,
        info->is_primary,
        info->x,
        info->y,
        (gint64)info->output_id);
}

void dbus_handle_method_call(GDBusConnection *connection,
                             const gchar *sender,
                             const gchar *object_path,
                             const gchar *interface_name,
                             const gchar *method_name,
                             GVariant *parameters,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data) {
    (void)connection; (void)sender; (void)object_path; (void)interface_name; (void)user_data;
    
    printf("📨 D-Bus: %s received\n", method_name);
    
    // ═══════════════════════════════════════════════════════════════════════
    // GetDisplays
    // ═══════════════════════════════════════════════════════════════════════
    if (g_strcmp0(method_name, "GetDisplays") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(siidbbiix)"));
        GList *displays = display_get_all();
        for (GList *l = displays; l; l = l->next) {
            DisplayInfo *info = l->data;
            g_variant_builder_add_value(&builder, display_info_to_variant(info));
            display_info_free(info);
        }
        g_list_free(displays);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(siidbbiix))", &builder));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetPrimaryDisplay
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetPrimaryDisplay") == 0) {
        DisplayInfo *info = display_get_primary();
        if (info) {
            g_dbus_method_invocation_return_value(invocation, 
                g_variant_new("((siidbbiix))", info->name, info->width, info->height, 
                    info->refresh_rate, info->is_connected, info->is_primary, 
                    info->x, info->y, (gint64)info->output_id));
            display_info_free(info);
        } else {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "No primary display");
        }
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetDisplayInfo
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetDisplayInfo") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        DisplayInfo *info = display_get_by_name(name);
        if (info) {
            g_dbus_method_invocation_return_value(invocation, 
                g_variant_new("((siidbbiix))", info->name, info->width, info->height, 
                    info->refresh_rate, info->is_connected, info->is_primary, 
                    info->x, info->y, (gint64)info->output_id));
            display_info_free(info);
        } else {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Display not found");
        }
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetModes
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetModes") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a(iid)"));
        GList *modes = display_get_modes(name);
        for (GList *l = modes; l; l = l->next) {
            DisplayMode *mode = l->data;
            g_variant_builder_add(&builder, "(iid)", mode->width, mode->height, mode->rates[0]);
            display_mode_free(mode);
        }
        g_list_free(modes);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a(iid))", &builder));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetResolution
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetResolution") == 0) {
        const gchar *name; gint32 w, h;
        g_variant_get(parameters, "(&sii)", &name, &w, &h);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_resolution(name, w, h)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetRefreshRate
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetRefreshRate") == 0) {
        const gchar *name; gdouble rate;
        g_variant_get(parameters, "(&sd)", &name, &rate);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_refresh_rate(name, rate)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetMode
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetMode") == 0) {
        const gchar *name; gint32 w, h; gdouble rate;
        g_variant_get(parameters, "(&siid)", &name, &w, &h, &rate);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_mode(name, w, h, rate)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetPosition
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetPosition") == 0) {
        const gchar *name; gint32 x, y;
        g_variant_get(parameters, "(&sii)", &name, &x, &y);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_position(name, x, y)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetPrimary
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetPrimary") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_primary(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetRotation
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetRotation") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(i)", (gint32)display_get_rotation(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetRotation
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetRotation") == 0) {
        const gchar *name; gint32 rotation;
        g_variant_get(parameters, "(&si)", &name, &rotation);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_rotation(name, (RotationType)rotation)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // EnableOutput
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "EnableOutput") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_enable(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // DisableOutput
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "DisableOutput") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_disable(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetMirror
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetMirror") == 0) {
        const gchar *source, *target;
        g_variant_get(parameters, "(&s&s)", &source, &target);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_mirror(source, target)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // DisableMirror
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "DisableMirror") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_disable_mirror(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetScale
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetScale") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(d)", display_get_scale(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetScale
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetScale") == 0) {
        const gchar *name; gdouble scale;
        g_variant_get(parameters, "(&sd)", &name, &scale);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_set_scale(name, scale)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetNightLight
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetNightLight") == 0) {
        NightLightSettings nl = display_get_night_light();
        g_dbus_method_invocation_return_value(invocation, g_variant_new("((bi))", nl.enabled, nl.temperature));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SetNightLight
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SetNightLight") == 0) {
        gboolean enabled; gint32 temp;
        g_variant_get(parameters, "(bi)", &enabled, &temp);
        gboolean success = display_set_night_light(enabled, temp);
        dbus_emit_signal("NightLightChanged", g_variant_new("(bi)", enabled, temp));
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", success));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // SaveProfile
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "SaveProfile") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_save_profile(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // LoadProfile
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "LoadProfile") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_load_profile(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // GetProfiles
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "GetProfiles") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        GList *profiles = display_get_profiles();
        for (GList *l = profiles; l; l = l->next) {
            g_variant_builder_add(&builder, "s", (gchar*)l->data);
            g_free(l->data);
        }
        g_list_free(profiles);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // DeleteProfile
    // ═══════════════════════════════════════════════════════════════════════
    else if (g_strcmp0(method_name, "DeleteProfile") == 0) {
        const gchar *name;
        g_variant_get(parameters, "(&s)", &name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", display_delete_profile(name)));
    }
    // ═══════════════════════════════════════════════════════════════════════
    // Unknown
    // ═══════════════════════════════════════════════════════════════════════
    else {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown: %s", method_name);
    }
}

void dbus_emit_signal(const gchar *signal_name, GVariant *parameters) {
    if (!display_state.session_conn) return;
    GError *error = NULL;
    g_dbus_connection_emit_signal(display_state.session_conn, NULL, DBUS_OBJECT_PATH, 
        DBUS_INTERFACE_NAME, signal_name, parameters, &error);
    if (error) { fprintf(stderr, "Signal error: %s\n", error->message); g_error_free(error); }
}

const GDBusInterfaceVTable dbus_interface_vtable = { dbus_handle_method_call, NULL, NULL };
