#include "pam_auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>

// Global password storage for PAM conversation
static const char *g_password = NULL;

// PAM conversation function
static int pam_conversation(int num_msg, const struct pam_message **msg,
                            struct pam_response **resp, void *appdata_ptr) {
    (void)appdata_ptr;
    
    struct pam_response *reply = calloc(num_msg, sizeof(struct pam_response));
    if (!reply) {
        return PAM_CONV_ERR;
    }
    
    for (int i = 0; i < num_msg; i++) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
            case PAM_PROMPT_ECHO_ON:
                // Return the password
                reply[i].resp = strdup(g_password ? g_password : "");
                reply[i].resp_retcode = 0;
                break;
            case PAM_ERROR_MSG:
            case PAM_TEXT_INFO:
                // Just acknowledge info/error messages
                reply[i].resp = NULL;
                reply[i].resp_retcode = 0;
                break;
            default:
                free(reply);
                return PAM_CONV_ERR;
        }
    }
    
    *resp = reply;
    return PAM_SUCCESS;
}

// Authenticate user using PAM
int pam_authenticate_user(const char *username, const char *password) {
    if (!username || !password) {
        return AUTH_ERROR;
    }
    
    // Set global password for conversation function
    g_password = password;
    
    // Setup PAM conversation
    struct pam_conv conv = {
        .conv = pam_conversation,
        .appdata_ptr = NULL
    };
    
    pam_handle_t *pamh = NULL;
    
    // Start PAM session with our service name (matches /etc/pam.d/venom_auth)
    int ret = pam_start("venom_auth", username, &conv, &pamh);
    if (ret != PAM_SUCCESS) {
        fprintf(stderr, "pam_start failed: %s\n", pam_strerror(pamh, ret));
        g_password = NULL;
        return AUTH_ERROR;
    }
    
    // Authenticate the user
    ret = pam_authenticate(pamh, 0);
    
    // Also validate the account
    int acct_ret = PAM_SUCCESS;
    if (ret == PAM_SUCCESS) {
        acct_ret = pam_acct_mgmt(pamh, 0);
    }
    
    // End PAM session
    pam_end(pamh, ret);
    
    // Clear password reference
    g_password = NULL;
    
    // Return result
    if (ret == PAM_SUCCESS && acct_ret == PAM_SUCCESS) {
        return AUTH_SUCCESS;
    }
    
    return AUTH_FAILURE;
}
