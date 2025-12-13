#ifndef PAM_AUTH_H
#define PAM_AUTH_H

// Authentication result codes
#define AUTH_SUCCESS  0
#define AUTH_FAILURE -1
#define AUTH_ERROR   -2

// Function prototypes
int pam_authenticate_user(const char *username, const char *password);

#endif // PAM_AUTH_H
