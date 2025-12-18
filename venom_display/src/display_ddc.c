/*
 * display_ddc.c - Venom Display DDC/CI Control
 * التحكم بالشاشة عبر DDC/CI
 */

#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// ═══════════════════════════════════════════════════════════════════════════
// ثوابت DDC/CI
// ═══════════════════════════════════════════════════════════════════════════

#define DDC_ADDR 0x37
#define DDC_BRIGHTNESS 0x10
#define DDC_CONTRAST 0x12
#define DDC_POWER 0xD6

// ═══════════════════════════════════════════════════════════════════════════
// دوال مساعدة
// ═══════════════════════════════════════════════════════════════════════════

static int find_i2c_bus(const gchar *display_name) {
    (void)display_name;  // Reserved for future display-specific bus mapping
    // Try common i2c buses
    for (int i = 0; i < 20; i++) {
        gchar *path = g_strdup_printf("/dev/i2c-%d", i);
        int fd = open(path, O_RDWR);
        g_free(path);
        
        if (fd >= 0) {
            if (ioctl(fd, I2C_SLAVE, DDC_ADDR) >= 0) {
                // Try to read from device
                unsigned char buf[2];
                if (read(fd, buf, 1) >= 0) {
                    return fd;  // Found working bus
                }
            }
            close(fd);
        }
    }
    return -1;
}

static gboolean ddc_write_command(int fd, unsigned char vcp, unsigned char value) {
    unsigned char buf[7];
    
    buf[0] = 0x51;  // Source address
    buf[1] = 0x84;  // Length
    buf[2] = 0x03;  // Set VCP feature
    buf[3] = vcp;   // VCP code
    buf[4] = value >> 8;   // Value high byte
    buf[5] = value & 0xFF; // Value low byte
    
    // Checksum
    buf[6] = buf[0];
    for (int i = 1; i < 6; i++) {
        buf[6] ^= buf[i];
    }
    
    if (write(fd, buf, 7) != 7) {
        return FALSE;
    }
    
    usleep(50000);  // Wait 50ms
    return TRUE;
}

static int ddc_read_vcp(int fd, unsigned char vcp) {
    // Request VCP value
    unsigned char req[5];
    req[0] = 0x51;  // Source
    req[1] = 0x82;  // Length
    req[2] = 0x01;  // Get VCP feature
    req[3] = vcp;
    req[4] = req[0] ^ req[1] ^ req[2] ^ req[3];
    
    if (write(fd, req, 5) != 5) {
        return -1;
    }
    
    usleep(50000);
    
    // Read response
    unsigned char resp[12];
    if (read(fd, resp, 12) < 8) {
        return -1;
    }
    
    if (resp[2] != 0x02) {
        return -1;  // Not a VCP reply
    }
    
    return (resp[8] << 8) | resp[9];
}

// ═══════════════════════════════════════════════════════════════════════════
// DDC/CI API
// ═══════════════════════════════════════════════════════════════════════════

gboolean display_ddc_available(const gchar *name) {
    int fd = find_i2c_bus(name);
    if (fd < 0) return FALSE;
    close(fd);
    return TRUE;
}

int display_ddc_get_brightness(const gchar *name) {
    int fd = find_i2c_bus(name);
    if (fd < 0) return -1;
    
    int value = ddc_read_vcp(fd, DDC_BRIGHTNESS);
    close(fd);
    
    if (value < 0) return -1;
    return value & 0xFF;
}

gboolean display_ddc_set_brightness(const gchar *name, int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    
    int fd = find_i2c_bus(name);
    if (fd < 0) return FALSE;
    
    gboolean result = ddc_write_command(fd, DDC_BRIGHTNESS, level);
    close(fd);
    return result;
}

int display_ddc_get_contrast(const gchar *name) {
    int fd = find_i2c_bus(name);
    if (fd < 0) return -1;
    
    int value = ddc_read_vcp(fd, DDC_CONTRAST);
    close(fd);
    
    if (value < 0) return -1;
    return value & 0xFF;
}

gboolean display_ddc_set_contrast(const gchar *name, int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    
    int fd = find_i2c_bus(name);
    if (fd < 0) return FALSE;
    
    gboolean result = ddc_write_command(fd, DDC_CONTRAST, level);
    close(fd);
    return result;
}

gboolean display_ddc_power_off(const gchar *name) {
    int fd = find_i2c_bus(name);
    if (fd < 0) return FALSE;
    
    // Power mode: 4 = off, 1 = on
    gboolean result = ddc_write_command(fd, DDC_POWER, 4);
    close(fd);
    return result;
}

gboolean display_ddc_power_on(const gchar *name) {
    int fd = find_i2c_bus(name);
    if (fd < 0) return FALSE;
    
    gboolean result = ddc_write_command(fd, DDC_POWER, 1);
    close(fd);
    return result;
}
