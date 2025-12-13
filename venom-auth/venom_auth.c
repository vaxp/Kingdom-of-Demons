#define _GNU_SOURCE
#define POLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <polkitagent/polkitagent.h>

#include "x11_ui.h"
#include "pam_auth.h"

// ------------------------------------------------------------------------------------------------
// D-Bus Service for Authentication
// ------------------------------------------------------------------------------------------------

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.venom.Auth'>"
    "    <method name='Authenticate'>"
    "      <arg name='username' type='s' direction='in'/>"
    "      <arg name='password' type='s' direction='in'/>"
    "      <arg name='success' type='b' direction='out'/>"
    "      <arg name='message' type='s' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static GDBusNodeInfo *introspection_data = NULL;

static void handle_method_call(GDBusConnection       *connection,
                               const gchar           *sender,
                               const gchar           *object_path,
                               const gchar           *interface_name,
                               const gchar           *method_name,
                               GVariant              *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer               user_data) {
    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)user_data;
    
    if (g_strcmp0(method_name, "Authenticate") == 0) {
        const gchar *username;
        const gchar *password;
        
        g_variant_get(parameters, "(&s&s)", &username, &password);
        
        // Authenticate using PAM
        int result = pam_authenticate_user(username, password);
        
        gboolean success = (result == AUTH_SUCCESS);
        const gchar *message = success ? "Authentication successful" : "Authentication failed";
        
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(bs)", success, message));
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    NULL,  // get_property
    NULL   // set_property
};

static guint dbus_owner_id = 0;

static void on_bus_acquired(GDBusConnection *connection,
                            const gchar     *name,
                            gpointer         user_data) {
    (void)name;
    (void)user_data;
    
    GError *error = NULL;
    
    guint registration_id = g_dbus_connection_register_object(
        connection,
        "/org/venom/Auth",
        introspection_data->interfaces[0],
        &interface_vtable,
        NULL,   // user_data
        NULL,   // user_data_free_func
        &error);
    
    if (registration_id == 0) {
        fprintf(stderr, "Failed to register D-Bus object: %s\n", error->message);
        g_error_free(error);
    } else {
        fprintf(stdout, "D-Bus object registered at /org/venom/Auth\n");
    }
}

static void on_name_acquired(GDBusConnection *connection,
                             const gchar     *name,
                             gpointer         user_data) {
    (void)connection;
    (void)user_data;
    fprintf(stdout, "D-Bus name acquired: %s\n", name);
}

static void on_name_lost(GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         user_data) {
    (void)connection;
    (void)user_data;
    fprintf(stderr, "D-Bus name lost: %s\n", name);
}

// ------------------------------------------------------------------------------------------------
// VenomAgent Class Definition (Polkit)
// ------------------------------------------------------------------------------------------------

#define VENOM_TYPE_AGENT (venom_agent_get_type())
G_DECLARE_FINAL_TYPE(VenomAgent, venom_agent, VENOM, AGENT, PolkitAgentListener)

struct _VenomAgent {
    PolkitAgentListener parent_instance;
};

G_DEFINE_TYPE(VenomAgent, venom_agent, POLKIT_AGENT_TYPE_LISTENER)

static void venom_agent_init(VenomAgent *agent) {
    (void)agent;
}

static void on_completed(PolkitAgentSession *session,
                         gboolean            gained_authorization,
                         gpointer            user_data) {
    GTask *task = G_TASK(user_data);
    
    if (gained_authorization) {
        g_task_return_boolean(task, TRUE);
    } else {
        g_task_return_new_error(task, POLKIT_ERROR, POLKIT_ERROR_FAILED, "Authentication failed");
    }
    
    g_object_unref(session);
    g_object_unref(task);
}

static void on_request(PolkitAgentSession *session,
                       gchar              *request,
                       gboolean            echo_on,
                       gpointer            user_data) {
    (void)echo_on;
    (void)request;
    char *password = (char *)user_data;
    polkit_agent_session_response(session, password);
}

static void on_show_error(PolkitAgentSession *session,
                          gchar              *text,
                          gpointer            user_data) {
    (void)session;
    (void)user_data;
    (void)text;
}

static void on_show_info(PolkitAgentSession *session,
                         gchar              *text,
                         gpointer            user_data) {
    (void)session;
    (void)user_data;
    (void)text;
}

static void venom_agent_initiate_authentication_real(PolkitAgentListener  *listener,
                                                     const gchar          *action_id,
                                                     const gchar          *message,
                                                     const gchar          *icon_name,
                                                     PolkitDetails        *details,
                                                     const gchar          *cookie,
                                                     GList                *identities,
                                                     GCancellable         *cancellable,
                                                     GAsyncReadyCallback   callback,
                                                     gpointer              user_data) {
    (void)action_id;
    (void)icon_name;
    (void)details;

    VenomAgent *agent = VENOM_AGENT(listener);
    
    if (identities == NULL) {
        g_task_report_new_error(G_OBJECT(agent), callback, user_data, venom_agent_initiate_authentication_real,
                                POLKIT_ERROR, POLKIT_ERROR_FAILED, "No identities");
        return;
    }

    PolkitIdentity *identity = POLKIT_IDENTITY(identities->data);
    
    char *password = malloc(256);
    int ui_result = x11_ui_get_password(
        message ? message : "Authentication Required",
        NULL,
        password,
        256
    );

    if (ui_result != UI_RESULT_SUCCESS) {
        free(password);
        g_task_report_new_error(G_OBJECT(agent), callback, user_data, venom_agent_initiate_authentication_real,
                                POLKIT_ERROR, POLKIT_ERROR_CANCELLED, "Cancelled");
        return;
    }

    PolkitAgentSession *session = polkit_agent_session_new(identity, cookie);
    GTask *task = g_task_new(agent, cancellable, callback, user_data);
    
    g_signal_connect(session, "completed", G_CALLBACK(on_completed), task);
    g_signal_connect(session, "request", G_CALLBACK(on_request), password);
    g_signal_connect(session, "show-error", G_CALLBACK(on_show_error), NULL);
    g_signal_connect(session, "show-info", G_CALLBACK(on_show_info), NULL);
    
    g_object_set_data_full(G_OBJECT(session), "password", password, free);

    polkit_agent_session_initiate(session);
}

static gboolean venom_agent_initiate_authentication_finish_real(PolkitAgentListener  *listener,
                                                                GAsyncResult         *res,
                                                                GError              **error) {
    (void)listener;
    return g_task_propagate_boolean(G_TASK(res), error);
}

static void venom_agent_class_init(VenomAgentClass *klass) {
    PolkitAgentListenerClass *listener_class = POLKIT_AGENT_LISTENER_CLASS(klass);
    
    listener_class->initiate_authentication = venom_agent_initiate_authentication_real;
    listener_class->initiate_authentication_finish = venom_agent_initiate_authentication_finish_real;
}

// ------------------------------------------------------------------------------------------------
// Main
// ------------------------------------------------------------------------------------------------

static GMainLoop *loop;

static void signal_handler(int signum) {
    (void)signum;
    if (loop) {
        g_main_loop_quit(loop);
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    // Initialize X11
    if (x11_ui_init() < 0) {
        fprintf(stderr, "Failed to initialize X11\n");
        return 1;
    }
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse D-Bus introspection data
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    if (!introspection_data) {
        fprintf(stderr, "Failed to parse D-Bus introspection XML\n");
        x11_ui_cleanup();
        return 1;
    }
    
    // Register D-Bus service on System Bus
    dbus_owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                   "org.venom.Auth",
                                   G_BUS_NAME_OWNER_FLAGS_NONE,
                                   on_bus_acquired,
                                   on_name_acquired,
                                   on_name_lost,
                                   NULL,
                                   NULL);
    
    // Create Polkit agent
    VenomAgent *agent = g_object_new(VENOM_TYPE_AGENT, NULL);
    
    // Register Polkit agent
    gpointer subject = polkit_unix_session_new_for_process_sync(getpid(), NULL, NULL);
    if (!subject) {
        fprintf(stderr, "Warning: Could not get unix session, trying generic registration...\n");
    }
    
    GError *error = NULL;
    if (!polkit_agent_listener_register(POLKIT_AGENT_LISTENER(agent), 
                                        POLKIT_AGENT_REGISTER_FLAGS_NONE,
                                        subject, 
                                        NULL, 
                                        NULL, 
                                        &error)) {
        fprintf(stderr, "Failed to register Polkit agent: %s\n", error->message);
        g_error_free(error);
        // Continue anyway - D-Bus service still works
    }
    
    fprintf(stdout, "Venom Auth started - D-Bus: org.venom.Auth, Polkit Agent: Active\n");
    
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    
    // Cleanup
    if (dbus_owner_id > 0) {
        g_bus_unown_name(dbus_owner_id);
    }
    g_dbus_node_info_unref(introspection_data);
    g_object_unref(agent);
    if (subject) g_object_unref(subject);
    g_main_loop_unref(loop);
    x11_ui_cleanup();
    
    return 0;
}