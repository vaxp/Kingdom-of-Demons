#ifndef X11_UI_H
#define X11_UI_H

// UI result codes
#define UI_RESULT_SUCCESS  1
#define UI_RESULT_CANCEL   0
#define UI_RESULT_ERROR   -1

// Function prototypes
int x11_ui_init(void);
void x11_ui_cleanup(void);

int x11_ui_get_password(const char *message, 
                        const char *icon_name,
                        char *password_out,
                        int max_length);

#endif // X11_UI_H
