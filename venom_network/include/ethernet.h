#ifndef ETHERNET_H
#define ETHERNET_H

#include "venom_network.h"

// ═══════════════════════════════════════════════════════════════════════════
// 🔌 Ethernet - معلومات الواجهة
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    char *name;
    char *mac_address;
    char *ip_address;
    char *gateway;
    int speed;
    gboolean connected;
    gboolean enabled;
} EthernetInterface;

// ═══════════════════════════════════════════════════════════════════════════
// الوظائف
// ═══════════════════════════════════════════════════════════════════════════

gboolean ethernet_init(void);
void ethernet_cleanup(void);

GList* ethernet_get_interfaces(void);
EthernetInterface* ethernet_get_status(const gchar *name);
gboolean ethernet_enable(const gchar *name);
gboolean ethernet_disable(const gchar *name);

void ethernet_interface_free(EthernetInterface *iface);

#endif
